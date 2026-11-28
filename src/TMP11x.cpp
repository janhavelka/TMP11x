#include "TMP11x/TMP11x.h"

#include "Acquisition.h"
#include "Apply.h"
#include "DeviceEvidence.h"
#include "Reset.h"
#include "TMP11x/CommandTable.h"
#include "TMP11x/Temperature.h"

namespace TMP11x {

Status TMP11x::bind(const Config& config) noexcept {
  const auto transport = detail::validateTransportBinding(config);
  if (!transport.ok()) return transport;
  if (!detail::isValidInterval(config.operationTimeoutMs) ||
      !detail::isValidInterval(config.clockResolutionMs) || config.offlineThreshold == 0) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid operation timeout, clock resolution or offline threshold");
  }
  const auto profile = validateSettings(config.model, config.settings);
  if (!profile.ok()) return profile;

  _config = config;
  if (_pendingReset) _pendingReset->originMs.reset();  // An accepted binding starts a new clock epoch.
  clearObservations();
  return Status::Ok();
}

Status TMP11x::begin(const Config& config) {
  const auto timing = detail::OperationBudget::validateTiming(config);
  if (!timing.ok()) return timing;
  const auto accepted = bind(config);
  if (!accepted.ok()) return accepted;
  return begin();
}

Status TMP11x::begin() {
  detail::Apply apply{*this};
  return apply.run();
}

Status TMP11x::recover() {
  return begin();
}

Status TMP11x::reset() {
  return detail::Reset{*this}.run();
}

Status TMP11x::applySettings(const SensorSettings& candidate) {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  if (_live.trust != ConfigTrust::VERIFIED) {
    return Status::Error(Err::CONFIG_DIRTY, "Recover the profile before changing settings");
  }
  detail::Apply apply{*this, candidate};
  return apply.run();
}

Status TMP11x::setMode(Mode mode) {
  auto candidate = _config.settings;
  candidate.mode = mode;
  return applySettings(candidate);
}

Status TMP11x::setCycle(Cycle cycle) {
  auto candidate = _config.settings;
  candidate.cycle = cycle;
  return applySettings(candidate);
}

Status TMP11x::setAveraging(Averaging averaging) {
  auto candidate = _config.settings;
  candidate.averaging = averaging;
  return applySettings(candidate);
}

Status TMP11x::setLimits(Limits limits) {
  auto candidate = _config.settings;
  candidate.limits = limits;
  return applySettings(candidate);
}

Status TMP11x::setTemperatureOffset(double celsius) {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  if (command::accessFor(_config.model, command::offset) != command::Access::GUARDED_OFFSET) {
    return Status::Error(Err::UNSUPPORTED, "Selected model has no temperature offset");
  }
  std::uint16_t raw = 0;
  const auto encoded = temperature::encodeCelsius(celsius, raw);
  if (!encoded.ok()) return encoded;
  auto candidate = _config.settings;
  candidate.managedOffsetCounts = temperature::decodeCounts(raw);
  return applySettings(candidate);
}

Status TMP11x::setAlert(AlertMode mode, Polarity polarity, PinFunction pinFunction) {
  auto candidate = _config.settings;
  candidate.alertMode = mode;
  candidate.polarity = polarity;
  candidate.pinFunction = pinFunction;
  return applySettings(candidate);
}

void TMP11x::clearLiveState() noexcept {
  // An identity change can make unread latches on the previous target unreachable.
  if (_live.startupReady) _alerts.possibleLoss = true;
  const auto generation = _live.profileGeneration;
  const auto verifiedAt = _live.verifiedAtMs;
  _live = {};
  _live.profileGeneration = generation;
  _live.verifiedAtMs = verifiedAt;
  _alerts.thermHigh.reset();
}

Status TMP11x::invalidateDeviceState() noexcept {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  clearLiveState();
  _alerts.possibleLoss = true;  // Outside access or loss of power can acknowledge or erase latches.
  _eepromUnsafe = false;  // Recovery must acquire new startup and lock evidence.
  return Status::Ok();
}

SettingsSnapshot TMP11x::getSettingsSnapshot() const noexcept {
  const auto reason = _live.trust == ConfigTrust::DIRTY && _live.dirtyReason == DirtyReason::NONE
                          ? DirtyReason::INCOMPLETE_APPLY : _live.dirtyReason;
  return {_config.settings, _live.config, _live.low, _live.high, _live.trust, reason,
          _live.affectedFields, _live.verificationReads, _live.profileGeneration, _live.verifiedAtMs, _live.offset};
}

Status TMP11x::requireInitialized() const noexcept {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  if (!isInitialized()) return Status::Error(Err::NOT_INITIALIZED, "Managed initialization required");
  return Status::Ok();
}

Status TMP11x::readObservedRegister(std::uint8_t pointer) {
  std::uint16_t raw = 0;
  const auto transfer = detail::readWord(detail::RegisterTransport::tracked(*this), pointer,
                                         raw, _config.i2cTimeoutMs);
  detail::DeviceEvidence{*this}.observeRegister(pointer, raw, transfer, &_config.settings,
                                                ConfigReadSource::OBSERVATION);
  return transfer.status;
}

Status TMP11x::readConfiguration(ConfigurationObservation& out) {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  const auto status = readObservedRegister(command::config);
  if (status.ok()) out = *_live.config;
  return status;
}

Status TMP11x::readLimits(Limits& out) {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  for (const auto pointer : {command::lowLimit, command::highLimit}) {
    const auto status = readObservedRegister(pointer);
    if (!status.ok()) return status;
  }
  out = {_live.low->counts, _live.high->counts};
  return Status::Ok();
}

Status TMP11x::readTemperatureOffset(std::int16_t& out) {
  if (isBound() && command::accessFor(_config.model, command::offset) != command::Access::GUARDED_OFFSET) {
    return Status::Error(Err::UNSUPPORTED, "Selected model has no temperature offset");
  }
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  const auto status = readObservedRegister(command::offset);
  if (status.ok()) out = _live.offset->counts;
  return status;
}

Status TMP11x::readAlertPinLevel(bool& high) const {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  if (!_config.readAlertPin) return Status::Error(Err::UNSUPPORTED, "No ALERT pin reader");
  bool physicalHigh = false;
  const auto status = _config.readAlertPin(physicalHigh, _config.gpioUser);
  if (status.inProgress()) {
    return Status::Error(Err::INVALID_CONFIG, "GPIO reader must finish synchronously", status.detail);
  }
  if (status.ok()) high = physicalHigh;
  return status;
}

Status TMP11x::takeAlertEvents(AlertStatus& out) noexcept {
  out = _alerts;
  _alerts.pendingHigh = false;
  _alerts.pendingLow = false;
  _alerts.possibleLoss = false;
  _alerts.mixedOrUnknown = false;
  _alerts.firstEvent.reset();
  return Status::Ok();
}

Status TMP11x::verifyConfiguration() {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  _live.verificationReads = 0;
  const auto transport = detail::RegisterTransport::tracked(*this);
  Status firstMismatch{};
  std::optional<std::uint32_t> lastReadMs;
  for (const auto pointer : {command::config, command::lowLimit, command::highLimit, command::offset}) {
    if (pointer == command::offset && command::accessFor(_config.model, pointer) != command::Access::GUARDED_OFFSET) {
      continue;
    }
    std::uint16_t raw = 0;
    const auto transfer = detail::readWord(transport, pointer, raw, _config.i2cTimeoutMs);
    const auto match = detail::DeviceEvidence{*this}.observeRegister(pointer, raw, transfer, &_config.settings,
                                                                    ConfigReadSource::VERIFICATION);
    if (!transfer.status.ok()) return transfer.status;
    lastReadMs = transfer.completedMs;
    if (firstMismatch.ok() && !match.ok()) firstMismatch = match;
  }
  if (!firstMismatch.ok()) return firstMismatch;
  if (_live.trust == ConfigTrust::DIRTY) return Status::Error(Err::CONFIG_DIRTY, "Verified replay required");
  _live.verifiedAtMs = lastReadMs;
  return Status::Ok();
}

void TMP11x::clearObservations() noexcept {
  _live = {};
  _alerts = {};
  _lastSample.reset();
  _health = {};
  _identity = {};
  _identity.model = _config.model;
  _identity.i2cAddress = _config.i2cAddress;
}

HealthSnapshot TMP11x::healthSnapshot() const noexcept {
  auto snapshot = _health;
  snapshot.initialized = isInitialized();
  return snapshot;
}

bool TMP11x::isOnline() const noexcept {
  return detail::isOnline(isInitialized(), _health.state);
}

Status TMP11x::end() noexcept {
  clearObservations();
  return Status::Ok();
}

Status TMP11x::unbind() noexcept {
  _config = {};
  clearObservations();
  return Status::Ok();
}

Status TMP11x::readLatest(Sample& out) {
  return detail::Acquisition{*this, ReadKind::LATEST}.run(out);
}

Status TMP11x::tryReadFresh(Sample& out) {
  return detail::Acquisition{*this, ReadKind::CONTINUOUS_FRESH}.run(out);
}

Status TMP11x::readFreshBlocking(Sample& out, std::uint32_t timeoutMs) {
  return detail::Acquisition{*this, ReadKind::CONTINUOUS_FRESH}.run(out, timeoutMs);
}

Status TMP11x::readOneShotBlocking(Sample& out, std::uint32_t timeoutMs) {
  return detail::Acquisition{*this, ReadKind::ONE_SHOT}.run(out, timeoutMs);
}

Status TMP11x::readTemperature(double& out) {
  Sample sample{};
  const auto status = readLatest(sample);
  if (status.ok()) out = sample.celsius;
  return status;
}

}  // namespace TMP11x
