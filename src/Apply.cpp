#include "Apply.h"

#include "DeviceEvidence.h"
#include "Identity.h"
#include "TMP11x/CommandTable.h"
#include "TMP11x/Configuration.h"
#include "TMP11x/Temperature.h"

namespace TMP11x::detail {
namespace {

constexpr auto pending = Status::Error(Err::IN_PROGRESS, "Apply pending");
constexpr auto unsafe = Status::Error(Err::EEPROM_UNSAFE, "Observed unlocked EEPROM");

}  // namespace

Status Apply::validateTiming(const Config& binding) noexcept {
  if (!binding.nowMs) return Status::Error(Err::CLOCK_REQUIRED, "Apply requires a clock");
  if (!isValidInterval(binding.operationTimeoutMs) || !isValidInterval(binding.clockResolutionMs) ||
      binding.operationTimeoutMs < binding.clockResolutionMs) {
    return Status::Error(Err::INVALID_CONFIG, "Apply budget must cover at least one clock tick");
  }
  return Status::Ok();
}

Status Apply::start() {
  if (_started) return _status;
  _started = true;
  if (!_device.isBound()) return _status = Status::Error(Err::NOT_BOUND, "No sensor binding");
  const auto& binding = _device._config;
  _status = validateSettings(binding.model, _candidate);
  if (!_status.ok()) return _status;
  _status = validateTiming(binding);
  if (!_status.ok()) return _status;
  if (_device._eepromUnsafe) return _status = unsafe;

  _shutdown.mode = Mode::SHUTDOWN;
  _budget.start(binding, binding.operationTimeoutMs);
  return _status = pending;
}

Status Apply::run() {
  auto status = start();
  while (status.inProgress()) status = step();
  return status;
}

Status Apply::step() {
  if (!_status.inProgress()) return _status;
  const auto& binding = _device._config;
  _status = _budget.beforeDispatch(binding.nowMs(binding.timeUser));
  if (_status.ok()) _status = _budget.afterDispatch(dispatch());
  auto& live = _device._live;
  if (!_status.inProgress() && live.trust == ConfigTrust::DIRTY && live.dirtyReason == DirtyReason::NONE) {
    live.dirtyReason = DirtyReason::INCOMPLETE_APPLY;
  }
  return _status;
}

Status Apply::readIdentityStep() {
  const auto result = detail::readIdentity(RegisterTransport::tracked(_device), _device._identity,
                                            _budget.remainingMs);
  if (result.status.is(Err::IDENTITY_MISMATCH)) _device.clearLiveState();
  const auto status = _budget.finishTransfer(result.transfer);
  if (!status.ok()) return status;
  if (!result.status.ok()) return result.status;

  _phase = Phase::GUARD;
  // Until the operation-local wait elapses, the instance holds no proof.
  return _device._live.startupReady ? pending : _budget.waitForUs(3000);
}

Status Apply::readGuard() {
  _device._live.startupReady = true;  // step() has completed any required FIRST_ACK wait.
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::eepromUnlock, raw, _budget.remainingMs);
  const auto guard = result.status.ok() ? DeviceEvidence{_device}.observeGuard(raw) : result.status;
  const auto status = _budget.finishTransfer(result);
  if (!status.ok()) return status;
  if (guard.is(Err::MEASUREMENT_NOT_READY)) return _budget.waitForUs(1000);
  if (!guard.ok()) return guard;
  _lockedIdle = true;
  _phase = _afterGuard;
  return pending;
}

Status Apply::readConfig() {
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::config, raw, _budget.remainingMs);
  const auto status = _budget.finishTransfer(result);
  auto& live = _device._live;
  // Preflight can observe any startup profile. Once an owned write was
  // attempted, every CONFIG read compares against that phase's target.
  const SensorSettings* expected = nullptr;
  if (_phase == Phase::FINAL_CONFIG) expected = &_candidate;
  else if (_phase != Phase::CURRENT_CONFIG) expected = &_shutdown;
  else if (live.initialized) expected = &_device._config.settings;
  const auto match = DeviceEvidence{_device}.observeRegister(command::config, raw, result, expected,
                                                            ConfigReadSource::APPLY, status.ok());
  if (result.status.ok() && (raw & command::configBusy)) _lockedIdle = false;
  if (!status.ok()) return status;
  // Explicit begin/recover repairs the existing target in this operation.
  // Its initial reads establish the starting values, even after drift.
  if (!match.ok() && !(_replayingTarget && _phase == Phase::CURRENT_CONFIG)) return match;
  if (!_lockedIdle) {
    _afterGuard = _phase;
    _phase = Phase::GUARD;
    return _budget.waitForUs(1000);
  }

  switch (_phase) {
    case Phase::CURRENT_CONFIG: _phase = Phase::CURRENT_LOW; break;
    case Phase::VERIFY_SHUTDOWN: _phase = Phase::WRITE_FIRST_LIMIT; break;
    case Phase::BASELINE: _phase = Phase::WRITE_MODE; break;
    case Phase::FINAL_CONFIG:
      ++live.profileGeneration;
      live.baselineMs = result.completedMs;
      live.verifiedAtMs = result.completedMs;
      live.readyHistoryUncertain = false;
      live.readyPending = false;
      live.trust = ConfigTrust::VERIFIED;
      live.dirtyReason = DirtyReason::NONE;
      live.affectedFields = 0;
      live.initialized = true;
      return Status::Ok();
    default: return Status::Error(Err::INVALID_PARAM, "Unexpected CONFIG phase");
  }
  return pending;
}

Status Apply::readLimit(std::uint8_t pointer) {
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), pointer, raw, _budget.remainingMs);
  const auto status = _budget.finishTransfer(result);
  auto& live = _device._live;
  const bool verifying = _phase == Phase::VERIFY_FIRST_LIMIT || _phase == Phase::VERIFY_SECOND_LIMIT;
  const SensorSettings* expected = verifying ? &_candidate : nullptr;
  if (!verifying && live.initialized) expected = &_device._config.settings;
  const auto match = DeviceEvidence{_device}.observeRegister(pointer, raw, result, expected, ConfigReadSource::APPLY);
  if (!status.ok()) return status;
  if (!match.ok() && (verifying || !_replayingTarget)) return match;
  switch (_phase) {
    case Phase::CURRENT_LOW: _phase = Phase::CURRENT_HIGH; break;
    case Phase::CURRENT_HIGH: {
      const auto oldHigh = temperature::decodeCounts(raw);
      const auto oldLow = live.low->counts;
      if (_candidate.limits.lowCounts > oldHigh) {
        _firstLimit = command::highLimit;
        _secondLimit = command::lowLimit;
      } else {
        _firstLimit = command::lowLimit;
        _secondLimit = command::highLimit;
      }
      // Never enter THERM with an inverted or unverified pair. An
      // existing THERM comparator can stay in THERM across ordered writes.
      if (oldLow > oldHigh || live.config->alertMode != AlertMode::THERM) {
        _shutdown.alertMode = AlertMode::ALERT;
      }
      // A settings change checks the previously owned profile before replacing it.
      // Explicit replay may repair a drifted managed offset in shutdown.
      _phase = !_replayingTarget && _device._config.settings.managedOffsetCounts
                   ? Phase::CURRENT_OFFSET : Phase::WRITE_SHUTDOWN;
      break;
    }
    // An ACK can leave the old limit in place. Confirm the first write
    // before the second can rely on its effect to keep the pair ordered.
    case Phase::VERIFY_FIRST_LIMIT: _phase = Phase::WRITE_SECOND_LIMIT; break;
    case Phase::VERIFY_SECOND_LIMIT:
      if (_candidate.managedOffsetCounts) {
        _phase = Phase::WRITE_OFFSET;
      } else if (command::accessFor(_device._config.model, command::offset) == command::Access::GUARDED_OFFSET) {
        _phase = Phase::READ_OFFSET;
      } else {
        _phase = beforeBaseline();
      }
      break;
    default: return Status::Error(Err::INVALID_PARAM, "Unexpected limit phase");
  }
  return pending;
}

Apply::Phase Apply::beforeBaseline() const noexcept {
  return _shutdown.alertMode == _candidate.alertMode ? Phase::BASELINE : Phase::ENABLE_THERM;
}

Status Apply::readOffset() {
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::offset, raw, _budget.remainingMs);
  const auto status = _budget.finishTransfer(result);
  const bool preflight = _phase == Phase::CURRENT_OFFSET;
  const auto& expected = preflight ? _device._config.settings : _candidate;
  const auto match = DeviceEvidence{_device}.observeRegister(command::offset, raw, result, &expected,
                                                            ConfigReadSource::APPLY);
  if (!status.ok()) return status;
  if (!match.ok()) return match;
  _phase = preflight ? Phase::WRITE_SHUTDOWN : beforeBaseline();
  return pending;
}

Status Apply::writeConfig(const SensorSettings& settings, Phase next) {
  const auto transport = RegisterTransport::tracked(_device);
  std::uint16_t raw = 0;
  const auto encoding = encodeConfiguration(transport.binding.model, settings, raw);
  if (!encoding.ok()) return encoding;
  auto& live = _device._live;
  if (_phase == Phase::WRITE_SHUTDOWN) {
    // All local checks passed. Commit intent before the first callback,
    // including when the adapter will report an ambiguous failure.
    _device._config.settings = _candidate;
    live.trust = ConfigTrust::DIRTY;
    live.baselineMs.reset();
    live.readyPending = false;
    live.verificationReads = 0;
  }
  const auto result = writeWord(transport, command::config, raw,
                                {WritePurpose::STABLE_CONFIG, live.startupReady, _lockedIdle}, _budget.remainingMs);
  DeviceEvidence{_device}.observeWrite(command::config, raw, result);
  const auto status = _budget.finishTransfer(result);
  if (!status.ok()) return status;
  _phase = next;
  return pending;
}

Status Apply::writeSettingWord(std::uint8_t pointer, Phase next) {
  auto purpose = WritePurpose::LIMIT;
  auto counts = _candidate.limits.highCounts;
  if (pointer == command::offset) {
    purpose = WritePurpose::MANAGED_OFFSET;
    counts = *_candidate.managedOffsetCounts;
  } else if (pointer == command::lowLimit) {
    counts = _candidate.limits.lowCounts;
  }
  const auto raw = static_cast<std::uint16_t>(counts);
  auto& live = _device._live;
  const auto result = writeWord(RegisterTransport::tracked(_device), pointer, raw,
                                {purpose, live.startupReady, _lockedIdle}, _budget.remainingMs);
  DeviceEvidence{_device}.observeWrite(pointer, raw, result);
  const auto status = _budget.finishTransfer(result);
  if (!status.ok()) return status;
  _phase = next;
  return pending;
}

Status Apply::dispatch() {
  switch (_phase) {
    case Phase::IDENTITY: return readIdentityStep();
    case Phase::GUARD: return readGuard();
    case Phase::CURRENT_CONFIG:
    case Phase::VERIFY_SHUTDOWN:
    case Phase::BASELINE:
    case Phase::FINAL_CONFIG: return readConfig();
    case Phase::CURRENT_LOW: return readLimit(command::lowLimit);
    case Phase::CURRENT_HIGH: return readLimit(command::highLimit);
    case Phase::WRITE_SHUTDOWN: return writeConfig(_shutdown, Phase::VERIFY_SHUTDOWN);
    case Phase::WRITE_FIRST_LIMIT: return writeSettingWord(_firstLimit, Phase::VERIFY_FIRST_LIMIT);
    case Phase::VERIFY_FIRST_LIMIT: return readLimit(_firstLimit);
    case Phase::WRITE_SECOND_LIMIT: return writeSettingWord(_secondLimit, Phase::VERIFY_SECOND_LIMIT);
    case Phase::VERIFY_SECOND_LIMIT: return readLimit(_secondLimit);
    case Phase::WRITE_OFFSET: return writeSettingWord(command::offset, Phase::READ_OFFSET);
    case Phase::CURRENT_OFFSET:
    case Phase::READ_OFFSET: return readOffset();
    case Phase::ENABLE_THERM:
      _shutdown.alertMode = AlertMode::THERM;
      return writeConfig(_shutdown, Phase::BASELINE);
    case Phase::WRITE_MODE: return writeConfig(_candidate, Phase::FINAL_CONFIG);
  }
  return Status::Error(Err::INVALID_PARAM, "Unknown apply phase");
}

}  // namespace TMP11x::detail
