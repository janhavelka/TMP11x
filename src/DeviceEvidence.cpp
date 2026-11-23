#include "DeviceEvidence.h"

#include "TMP11x/CommandTable.h"
#include "TMP11x/Temperature.h"

namespace TMP11x::detail {
namespace {

std::optional<SettingsField> fieldFor(std::uint8_t pointer) noexcept {
  switch (pointer) {
    case command::config: return SettingsField::CONFIG;
    case command::lowLimit: return SettingsField::LOW_LIMIT;
    case command::highLimit: return SettingsField::HIGH_LIMIT;
    default: return std::nullopt;
  }
}

}  // namespace

void DeviceEvidence::loseReadiness() noexcept {
  auto& live = device._live;
  live.readyPending = false;
  live.baselineMs.reset();
  live.readyHistoryUncertain = true;
}

void DeviceEvidence::markDirty(DirtyReason reason, SettingsField field) noexcept {
  auto& live = device._live;
  live.trust = ConfigTrust::DIRTY;
  if (live.dirtyReason == DirtyReason::NONE) live.dirtyReason = reason;
  live.affectedFields |= static_cast<std::uint8_t>(field);
  live.readyPending = false;
  live.baselineMs.reset();
}

void DeviceEvidence::observeTemperature(const TransferResult& transfer) noexcept {
  if (!transfer.attempted) return;
  if (!transfer.status.ok()) {
    loseReadiness();
    return;
  }
  auto& live = device._live;
  live.readyPending = false;
  // This TEMP read acknowledged DATA_READY. A later indication belongs after it.
  if (!live.baselineMs && live.trust == ConfigTrust::VERIFIED &&
      device._config.settings.mode == Mode::CONTINUOUS) {
    live.baselineMs = transfer.completedMs;
  }
}

void DeviceEvidence::captureConfig(std::uint16_t raw, std::optional<std::uint32_t> tick,
                                   ConfigReadSource source) noexcept {
  auto& live = device._live;
  auto& alerts = device._alerts;
  auto config = decodeConfiguration(raw);
  config.timestampValid = tick.has_value();
  config.observedAtMs = tick.value_or(0);
  live.config = config;
  if (config.eepromBusy) live.readyPending = false;
  else if (source != ConfigReadSource::APPLY && live.trust == ConfigTrust::VERIFIED &&
           config.mode == ObservedMode::CONTINUOUS) {
    if (live.baselineMs) live.readyPending |= config.dataReady;
    else live.baselineMs = tick;  // The first observation only establishes a boundary.
  }

  if (config.alertMode == AlertMode::THERM) {
    alerts.thermHigh = config.high;
    return;
  }
  alerts.thermHigh.reset();
  if (!config.high && !config.low) return;

  // Keep the first retained event's attribution when merging later events.
  if (!alerts.pendingHigh && !alerts.pendingLow) {
    alerts.profileGeneration = live.profileGeneration;
    alerts.source = source;
    alerts.mode = config.mode;
    alerts.observedAtMs = tick;
  } else if (alerts.profileGeneration != live.profileGeneration || alerts.source != source ||
             alerts.mode != config.mode) {
    alerts.mixedOrUnknown = true;
  }
  if (live.trust != ConfigTrust::VERIFIED) alerts.mixedOrUnknown = true;
  alerts.pendingHigh |= config.high;
  alerts.pendingLow |= config.low;
}

Status DeviceEvidence::observeRegister(std::uint8_t pointer, std::uint16_t raw,
                                       const TransferResult& transfer, const SensorSettings* expected,
                                       ConfigReadSource source) noexcept {
  if (!transfer.status.ok()) {
    if (transfer.attempted && pointer == command::config) {
      loseReadiness();
      device._alerts.possibleLoss = true;
      device._alerts.thermHigh.reset();
    }
    return transfer.status;
  }
  auto& live = device._live;
  const auto field = fieldFor(pointer);
  if (!field) return Status::Error(Err::INVALID_PARAM, "Expected an owned settings register", pointer);
  const auto match = expected ? compareExpectedRegister(device._config.model, *expected, pointer, raw, live.trigger)
                              : Status::Ok();
  if (match.is(Err::CONFIG_MISMATCH)) markDirty(DirtyReason::READBACK_MISMATCH, *field);
  if (pointer == command::config) {
    captureConfig(raw, transfer.completedMs, source);
  } else {
    auto& limit = pointer == command::lowLimit ? live.low : live.high;
    limit = LimitObservation{raw, temperature::decodeCounts(raw), transfer.completedMs};
  }
  if (source == ConfigReadSource::VERIFICATION) live.verificationReads |= static_cast<std::uint8_t>(*field);
  return match;
}

void DeviceEvidence::observeWrite(std::uint8_t pointer, std::uint16_t raw,
                                  const TransferResult& transfer) noexcept {
  if (!transfer.attempted) return;
  auto& live = device._live;
  const auto field = fieldFor(pointer);
  if (!field) return;
  if (!transfer.status.ok()) markDirty(DirtyReason::AMBIGUOUS_WRITE, *field);
  if (live.trust == ConfigTrust::DIRTY) live.affectedFields |= static_cast<std::uint8_t>(*field);
  if (pointer == command::config) {
    live.config.reset();
    device._alerts.thermHigh.reset();
    live.readyPending = false;
    if (transfer.status.ok()) {
      const auto mod = raw & command::modeMask;
      if (mod == command::modeMask) live.trigger = OwnedTrigger::CONFIRMED;
      else if (mod == 0x0400) live.trigger = OwnedTrigger::NONE;
    }
  } else {
    (pointer == command::lowLimit ? live.low : live.high).reset();
  }
}

}  // namespace TMP11x::detail
