#include "Apply.h"

#include "DeviceEvidence.h"
#include "Identity.h"
#include "TMP11x/CommandTable.h"
#include "TMP11x/Configuration.h"
#include "TMP11x/Temperature.h"

namespace TMP11x::detail {
namespace {

constexpr auto pending = Status::Error(Err::IN_PROGRESS, "Apply pending");
constexpr auto timeout = Status::Error(Err::OPERATION_TIMEOUT, "Apply deadline reached");
constexpr auto workLimit = Status::Error(Err::WORK_LIMIT, "Apply work allowance exhausted");
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
  _remainingMs = (binding.operationTimeoutMs / binding.clockResolutionMs) * binding.clockResolutionMs;
  _previousMs = binding.nowMs(binding.timeUser);
  return _status = pending;
}

Status Apply::run() {
  auto status = start();
  while (status.inProgress()) status = step();
  return status;
}

void Apply::advanceClock(std::uint32_t now) noexcept {
  const std::uint32_t elapsed = now - _previousMs;
  _previousMs = now;
  _remainingMs = elapsed < _remainingMs ? _remainingMs - elapsed : 0;
  _waitMs = elapsed < _waitMs ? _waitMs - elapsed : 0;
  if (elapsed != 0) _sameTickChecks = 0;
}

Status Apply::waitFor(std::uint32_t minimumMs) noexcept {
  // Completion may have been just before the next floor tick. Compare in
  // wide arithmetic before narrowing the relative interval.
  const std::uint64_t q = _device._config.clockResolutionMs;
  const auto delay = q + ((minimumMs + q - 1) / q) * q;
  if (delay >= _remainingMs) return timeout;
  _waitMs = static_cast<std::uint32_t>(delay);
  return pending;
}

Status Apply::step() {
  if (!_status.inProgress()) return _status;
  const auto& binding = _device._config;
  const auto now = binding.nowMs(binding.timeUser);
  const bool sameTick = now == _previousMs;
  advanceClock(now);
  if (_remainingMs == 0) {
    _status = timeout;
  } else if (_waitMs != 0) {
    if (sameTick && ++_sameTickChecks > 1000000u) _status = workLimit;
  } else {
    _sameTickChecks = 0;
    ++_dispatches;
    _status = dispatch();
    if (_status.inProgress() && _dispatches == 256) _status = workLimit;
  }
  auto& live = _device._live;
  if (!_status.inProgress() && live.trust == ConfigTrust::DIRTY && live.dirtyReason == DirtyReason::NONE) {
    live.dirtyReason = DirtyReason::INCOMPLETE_APPLY;
  }
  return _status;
}

Status Apply::finishTransfer(const TransferResult& result) noexcept {
  // Tracked transport timestamps every actual callback at its return.
  if (result.completedMs) advanceClock(*result.completedMs);
  return _remainingMs == 0 ? timeout : result.status;
}

Status Apply::readIdentityStep() {
  const auto result = detail::readIdentity(RegisterTransport::tracked(_device), _device._identity,
                                            _remainingMs);
  if (result.status.is(Err::IDENTITY_MISMATCH)) _device.clearLiveState();
  const auto status = finishTransfer(result.transfer);
  if (!status.ok()) return status;
  if (!result.status.ok()) return result.status;

  _phase = Phase::GUARD;
  // Until the operation-local wait elapses, the instance holds no proof.
  return _device._live.startupReady ? pending : waitFor(3);
}

Status Apply::readGuard() {
  _device._live.startupReady = true;  // step() has completed any required FIRST_ACK wait.
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::eepromUnlock, raw, _remainingMs);
  if (result.status.ok() && (raw & command::unlock)) _device._eepromUnsafe = true;
  const auto status = finishTransfer(result);
  if (!status.ok()) return status;
  if (_device._eepromUnsafe) return unsafe;
  if (raw & command::unlockBusy) return waitFor(1);
  _lockedIdle = true;
  _phase = _afterGuard;
  return pending;
}

Status Apply::readConfig() {
  std::uint16_t raw = 0;
  const auto result = readWord(RegisterTransport::tracked(_device), command::config, raw, _remainingMs);
  const auto status = finishTransfer(result);
  auto& live = _device._live;
  // Preflight can observe any startup profile. Once an owned write was
  // attempted, every CONFIG read compares against that phase's target.
  const SensorSettings* expected = nullptr;
  if (_phase == Phase::FINAL_CONFIG) expected = &_candidate;
  else if (_phase != Phase::CURRENT_CONFIG) expected = &_shutdown;
  else if (live.initialized) expected = &_device._config.settings;
  const auto match = DeviceEvidence{_device}.observeRegister(command::config, raw, result, expected,
                                                            ConfigReadSource::APPLY);
  if (result.status.ok() && (raw & command::configBusy)) _lockedIdle = false;
  if (!status.ok()) return status;
  // Explicit begin/recover repairs the existing target in this operation.
  // Its initial reads establish the starting values, even after drift.
  if (!match.ok() && !(_replayingTarget && _phase == Phase::CURRENT_CONFIG)) return match;
  if (!_lockedIdle) {
    _afterGuard = _phase;
    _phase = Phase::GUARD;
    return waitFor(1);
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
  const auto result = readWord(RegisterTransport::tracked(_device), pointer, raw, _remainingMs);
  const auto status = finishTransfer(result);
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
      _phase = Phase::WRITE_SHUTDOWN;
      break;
    }
    // An ACK can leave the old limit in place. Confirm the first write
    // before the second can rely on its effect to keep the pair ordered.
    case Phase::VERIFY_FIRST_LIMIT: _phase = Phase::WRITE_SECOND_LIMIT; break;
    case Phase::VERIFY_SECOND_LIMIT:
      _phase = _shutdown.alertMode == _candidate.alertMode ? Phase::BASELINE : Phase::ENABLE_THERM;
      break;
    default: return Status::Error(Err::INVALID_PARAM, "Unexpected limit phase");
  }
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
                                {WritePurpose::STABLE_CONFIG, live.startupReady, _lockedIdle}, _remainingMs);
  DeviceEvidence{_device}.observeWrite(command::config, raw, result);
  const auto status = finishTransfer(result);
  if (!status.ok()) return status;
  _phase = next;
  return pending;
}

Status Apply::writeLimit(std::uint8_t pointer, Phase next) {
  const bool low = pointer == command::lowLimit;
  const auto counts = low ? _candidate.limits.lowCounts : _candidate.limits.highCounts;
  auto& live = _device._live;
  const auto result = writeWord(RegisterTransport::tracked(_device), pointer, static_cast<std::uint16_t>(counts),
                                {WritePurpose::LIMIT, live.startupReady, _lockedIdle}, _remainingMs);
  DeviceEvidence{_device}.observeWrite(pointer, static_cast<std::uint16_t>(counts), result);
  const auto status = finishTransfer(result);
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
    case Phase::WRITE_FIRST_LIMIT: return writeLimit(_firstLimit, Phase::VERIFY_FIRST_LIMIT);
    case Phase::VERIFY_FIRST_LIMIT: return readLimit(_firstLimit);
    case Phase::WRITE_SECOND_LIMIT: return writeLimit(_secondLimit, Phase::VERIFY_SECOND_LIMIT);
    case Phase::VERIFY_SECOND_LIMIT: return readLimit(_secondLimit);
    case Phase::ENABLE_THERM:
      _shutdown.alertMode = AlertMode::THERM;
      return writeConfig(_shutdown, Phase::BASELINE);
    case Phase::WRITE_MODE: return writeConfig(_candidate, Phase::FINAL_CONFIG);
  }
  return Status::Error(Err::INVALID_PARAM, "Unknown apply phase");
}

}  // namespace TMP11x::detail
