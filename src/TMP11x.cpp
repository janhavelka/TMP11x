#include "TMP11x/TMP11x.h"

#include "Apply.h"
#include "Identity.h"
#include "LatestRead.h"

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

void TMP11x::invalidateIdentity() noexcept {
  // Historical alerts and samples still refer to their original generation.
  const auto generation = _live.profileGeneration;
  _live = {};
  _live.profileGeneration = generation;
  _alerts.thermHigh.reset();
}

void TMP11x::observeApplyConfig(const detail::TransferResult& transfer, std::uint16_t raw) noexcept {
  if (!transfer.attempted) return;
  if (!transfer.status.ok()) {
    _alerts.possibleLoss = true;
    _alerts.thermHigh.reset();
    _live.baselineMs.reset();
    return;
  }
  auto config = decodeConfiguration(raw);
  config.timestampValid = transfer.completedMs.has_value();
  config.observedAtMs = transfer.completedMs.value_or(0);
  _live.config = config;
  if (config.alertMode == AlertMode::THERM) {
    _alerts.thermHigh = config.high;
    return;
  }
  _alerts.thermHigh.reset();
  if (!config.high && !config.low) return;

  const bool pending = _alerts.pendingHigh || _alerts.pendingLow;
  // A returned latch can predate this read. During replay it cannot be
  // attributed to the generation about to be verified.
  if (_live.trust != ConfigTrust::VERIFIED ||
      (pending && _alerts.profileGeneration != _live.profileGeneration)) {
    _alerts.mixedOrUnknown = true;
  }
  _alerts.pendingHigh |= config.high;
  _alerts.pendingLow |= config.low;
  _alerts.observedAtMs = transfer.completedMs;
  _alerts.profileGeneration = _live.profileGeneration;
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
  if (result.status.is(Err::IDENTITY_MISMATCH)) invalidateIdentity();
  return result.status;
}

Status TMP11x::readIdentity(IdentityInfo& out) {
  const auto status = probe();
  if (status.ok() || status.is(Err::IDENTITY_MISMATCH)) out = _identity;
  return status;
}

Status TMP11x::readLatest(Sample& out) {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  if (!isInitialized()) return Status::Error(Err::NOT_INITIALIZED, "Managed initialization required");
  return detail::LatestRead{*this}.read(out).status;
}

Status TMP11x::readTemperature(double& out) {
  Sample sample{};
  const auto status = readLatest(sample);
  if (status.ok()) out = sample.celsius;
  return status;
}

}  // namespace TMP11x
