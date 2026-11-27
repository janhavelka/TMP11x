#include "Reset.h"

#include "DeviceEvidence.h"
#include "ModelDescriptor.h"

namespace TMP11x::detail {

Status Reset::awaitDispatch() {
  const auto& binding = _device._config;
  Status status;
  do {
    status = _budget.beforeDispatch(binding.nowMs(binding.timeUser));
  } while (status.inProgress());
  return status;
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
  Status guard;
  do {
    const auto admission = awaitDispatch();
    if (!admission.ok()) return admission;
    guard = DeviceEvidence{_device}.readTimedGuard(_budget);
    const auto next = _budget.afterDispatch(guard.ok() ? OperationBudget::pending : guard);
    if (!next.inProgress()) return next;
  } while (guard.inProgress());

  const auto admission = awaitDispatch();
  if (!admission.ok()) return admission;
  const WriteAccess access{WritePurpose::ADDRESSED_RESET, live.startupReady, guard.ok()};
  // The callback may apply the command even if it reports an error.
  _device.clearLiveState();
  auto& alerts = _device._alerts;
  if (alerts.firstEvent) alerts.mixedOrUnknown = true;
  const auto result = writeWord(RegisterTransport::tracked(_device), command::config,
                                stableWord | command::softReset, access, _budget.remainingMs);
  return _budget.finishTransfer(result);
}

}  // namespace TMP11x::detail
