#include "Reset.h"

#include "DeviceEvidence.h"
#include "Identity.h"
#include "ModelDescriptor.h"

namespace TMP11x::detail {

Status Reset::awaitDispatch(std::uint32_t delayUs) {
  const auto& binding = _device._config;
  const auto wait = delayUs ? _budget.waitForUs(delayUs) : OperationBudget::pending;
  auto status = _budget.afterDispatch(wait);
  while (status.inProgress()) {
    status = _budget.beforeDispatch(binding.nowMs(binding.timeUser));
  }
  return status;
}

Status Reset::readGuard(std::uint32_t delayUs) {
  auto admission = awaitDispatch(delayUs);
  while (admission.ok()) {
    // Preflight has startup proof. The first post-command guard reaches
    // this point only after the full wait from a new compatible ID.
    _device._live.startupReady = true;
    const auto guard = DeviceEvidence{_device}.readTimedGuard(_budget);
    if (!guard.inProgress()) return guard;
    admission = awaitDispatch();
  }
  return admission;
}

Status Reset::issueCommand() {
  const auto& binding = _device._config;
  if (!_device.isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  if (!findModelDescriptor(binding.model)->capabilities.hasAddressedReset) {
    return Status::Error(Err::UNSUPPORTED, "Model has no addressed reset");
  }
  const auto& live = _device._live;
  if (!live.initialized || !live.startupReady) {
    return Status::Error(Err::NOT_INITIALIZED, "Reset requires initialized startup proof");
  }
  if (live.trust != ConfigTrust::VERIFIED) return Status::Error(Err::CONFIG_DIRTY, "Verified profile required");
  if (_device._eepromUnsafe) return Status::Error(Err::EEPROM_UNSAFE, "Observed unlocked EEPROM");
  const auto timing = OperationBudget::validateTiming(binding);
  if (!timing.ok()) return timing;
  std::uint16_t stableWord = 0;
  const auto encoding = encodeConfiguration(binding.model, binding.settings, stableWord);
  if (!encoding.ok()) return encoding;

  _budget.start(binding, binding.operationTimeoutMs);
  const auto guard = readGuard();
  if (!guard.ok()) return guard;

  const auto admission = awaitDispatch();
  if (!admission.ok()) return admission;
  const WriteAccess access{WritePurpose::ADDRESSED_RESET, live.startupReady, guard.ok()};
  // The callback may apply the command even if it reports an error.
  _device.clearLiveState();
  auto& alerts = _device._alerts;
  if (alerts.firstEvent) alerts.mixedOrUnknown = true;
  const auto result = writeWord(RegisterTransport::tracked(_device), command::config,
                                stableWord | command::softReset, access, _budget.remainingMs);
  _device._resetAttemptMs = result.completedMs;
  return _budget.finishTransfer(result);
}

Status Reset::readProfile() {
  const auto transport = RegisterTransport::tracked(_device);
  Status firstMismatch{};
  // Every model admitted for addressed reset also has OFFSET.
  for (const auto pointer : {command::config, command::lowLimit, command::highLimit, command::offset}) {
    for (;;) {
      const auto admission = awaitDispatch();
      if (!admission.ok()) return admission;
      std::uint16_t raw = 0;
      const auto transfer = readWord(transport, pointer, raw, _budget.remainingMs);
      const auto status = _budget.finishTransfer(transfer);
      const auto match = DeviceEvidence{_device}.observeRegister(
          pointer, raw, transfer, &transport.binding.settings, ConfigReadSource::RESET, status.ok());
      if (!status.ok()) return status;
      if (firstMismatch.ok() && !match.ok()) firstMismatch = match;
      if (pointer != command::config || !(raw & command::configBusy)) break;
      // CONFIG acknowledged flags, but BUSY invalidated the preceding guard.
      const auto guard = readGuard(1000);
      if (!guard.ok()) return guard;
    }
  }
  return firstMismatch;
}

Status Reset::run() {
  const auto issued = issueCommand();
  if (!issued.ok()) return issued;

  const auto admission = awaitDispatch(addressedResetUs);
  if (!admission.ok()) return admission;
  _device._resetAttemptMs.reset();
  const auto id = readIdentity(RegisterTransport::tracked(_device), _device._identity, _budget.remainingMs);
  const auto status = _budget.finishTransfer(id.transfer);
  if (!status.ok()) return status;
  if (!id.status.ok()) return id.status;

  const auto guard = readGuard(startupWaitUs);
  if (!guard.ok()) return guard;
  return readProfile();
}

}  // namespace TMP11x::detail
