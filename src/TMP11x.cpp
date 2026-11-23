#include "TMP11x/TMP11x.h"

#include "Apply.h"
#include "DeviceEvidence.h"
#include "Identity.h"
#include "LatestRead.h"
#include "TMP11x/CommandTable.h"

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
  clearObservations();
  return Status::Ok();
}

Status TMP11x::begin(const Config& config) {
  const auto timing = detail::Apply::validateTiming(config);
  if (!timing.ok()) return timing;
  const auto accepted = bind(config);
  if (!accepted.ok()) return accepted;
  return begin();
}

Status TMP11x::begin() {
  detail::Apply apply{*this};
  auto status = apply.start();
  while (status.inProgress()) status = apply.step();
  return status;
}

Status TMP11x::recover() {
  return begin();
}

void TMP11x::clearLiveState() noexcept {
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
  _eepromUnsafe = false;  // Recovery must acquire new startup and lock evidence.
  return Status::Ok();
}

SettingsSnapshot TMP11x::getSettingsSnapshot() const noexcept {
  const auto reason = _live.trust == ConfigTrust::DIRTY && _live.dirtyReason == DirtyReason::NONE
                          ? DirtyReason::INCOMPLETE_APPLY : _live.dirtyReason;
  return {_config.settings, _live.config, _live.low, _live.high, _live.trust, reason,
          _live.affectedFields, _live.verificationReads, _live.profileGeneration, _live.verifiedAtMs};
}

Status TMP11x::requireInitialized() const noexcept {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  if (!isInitialized()) return Status::Error(Err::NOT_INITIALIZED, "Managed initialization required");
  return Status::Ok();
}

Status TMP11x::readConfiguration(ConfigurationObservation& out) {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  std::uint16_t raw = 0;
  const auto transfer = detail::readWord(detail::RegisterTransport::tracked(*this), command::config,
                                         raw, _config.i2cTimeoutMs);
  detail::DeviceEvidence{*this}.observeRegister(command::config, raw, transfer, &_config.settings,
                                               detail::ConfigReadSource::OBSERVATION);
  if (transfer.status.ok()) out = *_live.config;
  return transfer.status;
}

Status TMP11x::verifyConfiguration() {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  _live.verificationReads = 0;
  const auto transport = detail::RegisterTransport::tracked(*this);
  Status firstMismatch{};
  for (const auto pointer : {command::config, command::lowLimit, command::highLimit}) {
    std::uint16_t raw = 0;
    const auto transfer = detail::readWord(transport, pointer, raw, _config.i2cTimeoutMs);
    const auto match = detail::DeviceEvidence{*this}.observeRegister(pointer, raw, transfer, &_config.settings,
                                                                    detail::ConfigReadSource::VERIFICATION);
    if (!transfer.status.ok()) return transfer.status;
    if (firstMismatch.ok() && !match.ok()) firstMismatch = match;
  }
  if (!firstMismatch.ok()) return firstMismatch;
  if (_live.trust == ConfigTrust::DIRTY) return Status::Error(Err::CONFIG_DIRTY, "Verified replay required");
  _live.verifiedAtMs = _live.high->observedAtMs;
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

Status TMP11x::probe() {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  const auto result = detail::readIdentity(_config, _identity, _config.i2cTimeoutMs);
  if (result.status.is(Err::IDENTITY_MISMATCH)) clearLiveState();
  return result.status;
}

Status TMP11x::readIdentity(IdentityInfo& out) {
  const auto status = probe();
  if (status.ok() || status.is(Err::IDENTITY_MISMATCH)) out = _identity;
  return status;
}

Status TMP11x::readLatest(Sample& out) {
  const auto admission = requireInitialized();
  if (!admission.ok()) return admission;
  return detail::LatestRead{*this}.read(out).status;
}

Status TMP11x::readTemperature(double& out) {
  Sample sample{};
  const auto status = readLatest(sample);
  if (status.ok()) out = sample.celsius;
  return status;
}

}  // namespace TMP11x
