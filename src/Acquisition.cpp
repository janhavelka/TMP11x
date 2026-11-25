#include "Acquisition.h"

#include "DeviceEvidence.h"
#include "LatestRead.h"
#include "TMP11x/CommandTable.h"

namespace TMP11x::detail {
namespace {

constexpr auto pending = OperationBudget::pending;
constexpr auto notReady = Status::Error(Err::MEASUREMENT_NOT_READY, "No eligible conversion result");

}  // namespace

Status Acquisition::start(std::optional<std::uint32_t> timeoutMs) {
  if (!_started) {
    _started = true;
    _status = admit(timeoutMs);
  }
  return _status;
}

Status Acquisition::admit(std::optional<std::uint32_t> timeoutMs) {
  if (!_device.isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  const auto& binding = _device._config;
  if (_kind != ReadKind::LATEST && _kind != ReadKind::CONTINUOUS_FRESH && _kind != ReadKind::ONE_SHOT) {
    return Status::Error(Err::INVALID_PARAM, "Invalid acquisition kind");
  }
  if (_kind == ReadKind::ONE_SHOT && !timeoutMs) {
    return Status::Error(Err::INVALID_PARAM, "One-shot requires a time budget");
  }
  _timed = timeoutMs.has_value();
  if (timeoutMs) {
    if (!isValidInterval(*timeoutMs) || *timeoutMs < binding.clockResolutionMs) {
      return Status::Error(Err::INVALID_PARAM, "Acquisition budget must cover at least one tick");
    }
    if (!binding.nowMs) return Status::Error(Err::CLOCK_REQUIRED, "Timed acquisition requires a clock");
  }
  const auto initialized = _device.requireInitialized();
  if (!initialized.ok()) return initialized;
  if (_kind == ReadKind::LATEST) {
    _phase = Phase::TEMP;
  } else {
    const auto& live = _device._live;
    if (!live.startupReady) return Status::Error(Err::NOT_INITIALIZED, "Missing startup proof");
    if (live.trust != ConfigTrust::VERIFIED) return Status::Error(Err::CONFIG_DIRTY, "Verified profile required");
    const auto& settings = binding.settings;
    const bool shot = _kind == ReadKind::ONE_SHOT;
    if (settings.mode != (shot ? Mode::SHUTDOWN : Mode::CONTINUOUS)) {
      return Status::Error(Err::INVALID_CONFIG, "Desired mode does not support this acquisition");
    }
    if (_device._eepromUnsafe) return Status::Error(Err::EEPROM_UNSAFE, "Observed unlocked EEPROM");
    _generation = live.profileGeneration;
    if (shot) {
      _afterGuard = Phase::PREFLIGHT;
      const auto status = encodeConfiguration(binding.model, settings, _shutdownWord);
      if (!status.ok()) return status;
    }
    if (_timed) {
      const auto status = getConversionTiming(binding.model, settings.cycle, settings.averaging, _timing);
      if (!status.ok()) return status;
    }
  }
  if (timeoutMs) _budget.start(binding, *timeoutMs);
  else _budget.remainingMs = binding.i2cTimeoutMs;
  return pending;
}

Status Acquisition::step(Sample& out) {
  if (!_status.inProgress()) return _status;
  if (_timed) {
    const auto& binding = _device._config;
    const auto due = _budget.beforeDispatch(binding.nowMs(binding.timeUser));
    if (!due.ok()) return _status = due;
  }
  _status = dispatch(out);
  if (_timed) _status = _budget.afterDispatch(_status);
  return _status;
}

Status Acquisition::run(Sample& out, std::optional<std::uint32_t> timeoutMs) {
  auto status = start(timeoutMs);
  while (status.inProgress()) status = step(out);
  return status;
}

Status Acquisition::finish(const TransferResult& result) noexcept {
  return _timed ? _budget.finishTransfer(result) : result.status;
}

Status Acquisition::renewGuard(Phase resume) {
  _lockedIdle = false;
  if (!_timed) return notReady;
  _afterGuard = resume;
  _phase = Phase::GUARD;
  return _budget.waitForUs(1000);
}

Status Acquisition::preparePoll() {
  _phase = Phase::POLL;
  if (_device._live.readyPending) {
    _phase = Phase::TEMP;
    return pending;
  }
  if (!_timed) return pending;
  // Credit time spent renewing the guard after a BUSY poll. In continuous
  // mode the verified generation's baseline can predate the operation.
  const auto origin = _lastPollMs ? _lastPollMs : _device._live.baselineMs;
  if (!origin) return pending;  // Establish a boundary before waiting for a new block.
  const auto intervalUs = _kind == ReadKind::ONE_SHOT ? 5000u : _timing.nominalPeriodUs;
  return _budget.waitForUs(intervalUs, _budget.previousMs - *origin);
}

Status Acquisition::readGuard() {
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::eepromUnlock, raw, _budget.remainingMs);
  const auto guard = result.status.ok() ? DeviceEvidence{_device}.observeGuard(raw) : result.status;
  const auto status = finish(result);
  if (!status.ok()) return status;
  if (guard.is(Err::MEASUREMENT_NOT_READY)) return renewGuard(_afterGuard);
  if (!guard.ok()) return guard;
  _lockedIdle = true;
  _phase = _afterGuard;
  return _phase == Phase::POLL ? preparePoll() : pending;
}

Status Acquisition::readConfig() {
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::config, raw, _budget.remainingMs);
  const auto status = finish(result);
  const auto match = DeviceEvidence{_device}.observeRegister(command::config, raw, result, &_device._config.settings,
                                                            ConfigReadSource::ACQUISITION, status.ok());
  if (!status.ok()) return status;
  if (!match.ok()) return match;
  if (_phase == Phase::POLL) _lastPollMs = result.completedMs;
  if (raw & command::configBusy) return renewGuard(_phase);
  switch (_phase) {
    case Phase::PREFLIGHT: _phase = Phase::SHUTDOWN; return pending;
    case Phase::BASELINE: _phase = Phase::TRIGGER; return pending;
    default:
      if (!_timed && !_device._live.readyPending) return notReady;
      return preparePoll();
  }
}

Status Acquisition::writeConfig() {
  const bool trigger = _phase == Phase::TRIGGER;
  const auto word = static_cast<std::uint16_t>(trigger ? _shutdownWord | command::modeMask : _shutdownWord);
  const auto purpose = trigger ? WritePurpose::ONE_SHOT : WritePurpose::STABLE_CONFIG;
  const auto result = writeWord(RegisterTransport::tracked(_device), command::config, word,
                                {purpose, _device._live.startupReady, _lockedIdle}, _budget.remainingMs);
  DeviceEvidence{_device}.observeWrite(command::config, word, result);
  const auto status = finish(result);
  if (!status.ok()) return status;
  _phase = trigger ? Phase::POLL : Phase::BASELINE;
  return trigger ? _budget.waitForUs(_timing.activeEstimate.maxUs) : pending;
}

Status Acquisition::dispatch(Sample& out) {
  const auto& live = _device._live;
  if (_kind != ReadKind::LATEST && (live.trust != ConfigTrust::VERIFIED || live.profileGeneration != _generation)) {
    return Status::Error(Err::CONFIG_DIRTY, "Acquisition profile is no longer current");
  }
  switch (_phase) {
    case Phase::GUARD: return readGuard();
    case Phase::PREFLIGHT:
    case Phase::BASELINE:
    case Phase::POLL: return readConfig();
    case Phase::SHUTDOWN:
    case Phase::TRIGGER: return writeConfig();
    case Phase::TEMP: {
      if (_kind != ReadKind::LATEST && !live.readyPending) return notReady;
      std::optional<ReadBudget> budget;
      if (_timed) budget = ReadBudget{_budget.previousMs, _budget.remainingMs};
      return LatestRead{_device}.read(out, budget, _kind).status;
    }
  }
  return Status::Error(Err::INVALID_PARAM, "Unknown acquisition phase");
}

}  // namespace TMP11x::detail
