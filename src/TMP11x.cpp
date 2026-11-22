#include "TMP11x/TMP11x.h"

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

  _config = config;
  clearObservations();
  return Status::Ok();
}

void TMP11x::clearObservations() noexcept {
  _live = {};
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
  if (result.status.is(Err::IDENTITY_MISMATCH)) _live = {};
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
