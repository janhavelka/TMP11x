#include "DeviceEvidence.h"

#include "TMP11x/CommandTable.h"
#include "TMP11x/Temperature.h"

namespace TMP11x::detail {
namespace {

constexpr auto eepromBusy = Status::Error(Err::MEASUREMENT_NOT_READY, "EEPROM busy");

std::optional<SettingsField> fieldFor(std::uint8_t pointer) noexcept {
  switch (pointer) {
    case command::config: return SettingsField::CONFIG;
    case command::lowLimit: return SettingsField::LOW_LIMIT;
    case command::highLimit: return SettingsField::HIGH_LIMIT;
    case command::offset: return SettingsField::OFFSET;
    default: return std::nullopt;
  }
}

}  // namespace

Status DeviceEvidence::observeGuard(std::uint16_t raw, std::optional<std::uint32_t> tick) noexcept {
  if (raw & command::unlock) device._eepromUnsafe = true;
  const bool busy = (raw & command::unlockBusy) != 0;
  if (busy) observeBusy(tick);
  if (device._eepromUnsafe) return Status::Error(Err::EEPROM_UNSAFE, "Observed unlocked EEPROM");
  return busy ? eepromBusy : Status::Ok();
}

void DeviceEvidence::observeBusy(std::optional<std::uint32_t> tick) noexcept {
  auto& live = device._live;
  if (live.guardInterruption.ok()) {
    live.guardInterruption = eepromBusy;
  }
  live.guardBusyMs = tick;
  live.readyPending = false;
}

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
    device._live.guardInterruption = transfer.status;
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
                                   ConfigReadSource source, bool timely) noexcept {
  auto& live = device._live;
  auto& alerts = device._alerts;
  auto config = decodeConfiguration(raw);
  config.timestampValid = tick.has_value();
  config.observedAtMs = tick.value_or(0);
  live.config = config;
  alerts.lastConfigSource = source;
  alerts.lastConfigAtMs = tick;
  alerts.readClearMask = config.readClearMask;
  // A matching preflight still observes the current generation until the first write.
  if (config.eepromBusy) {
    observeBusy(tick);
  } else if (timely && live.trust == ConfigTrust::VERIFIED) {
    if (config.mode == ObservedMode::CONTINUOUS) {
      if (live.baselineMs) live.readyPending |= config.dataReady;
      else live.baselineMs = tick;  // The first observation only establishes a boundary.
    } else if (live.trigger == OwnedTrigger::CONFIRMED) {
      // A new explicit shot always clears residual evidence at its SD boundary.
      live.readyPending |= config.dataReady;
    }
  }
  if (source == ConfigReadSource::ACQUISITION && live.baselineMs &&
      config.mode == ObservedMode::CONTINUOUS) {
    live.baselineMs = tick;  // Pace subsequent continuous polls from this callback.
  }

  if (config.alertMode == AlertMode::THERM) {
    alerts.thermHigh = config.high;
    return;
  }
  alerts.thermHigh.reset();
  if (!config.high && !config.low) return;

  // Keep the first retained event's attribution when merging later events.
  if (!alerts.firstEvent) {
    alerts.firstEvent = AlertEventOrigin{source, config.mode, live.profileGeneration, tick};
  } else if (alerts.firstEvent->profileGeneration != live.profileGeneration ||
             alerts.firstEvent->source != source || alerts.firstEvent->mode != config.mode) {
    alerts.mixedOrUnknown = true;
  }
  if (live.trust != ConfigTrust::VERIFIED) alerts.mixedOrUnknown = true;
  alerts.pendingHigh |= config.high;
  alerts.pendingLow |= config.low;
}

Status DeviceEvidence::observeRegister(std::uint8_t pointer, std::uint16_t raw,
                                       const TransferResult& transfer, const SensorSettings* expected,
                                       ConfigReadSource source, bool timely) noexcept {
  if (!transfer.status.ok()) {
    if (transfer.attempted) device._live.guardInterruption = transfer.status;
    if (transfer.attempted && pointer == command::config) {
      loseReadiness();
      device._alerts.possibleLoss = true;
      device._alerts.thermHigh.reset();
    }
    return transfer.status;
  }
  auto& live = device._live;
  if (pointer == command::offset) {
    if (command::accessFor(device._config.model, pointer) != command::Access::GUARDED_OFFSET) {
      return Status::Error(Err::UNSUPPORTED, "Selected model has no temperature offset");
    }
    const bool managed = expected && expected->managedOffsetCounts.has_value();
    const bool drift = !managed && live.offset && !live.offsetContinuityLost && live.offset->raw != raw;
    // An unexplained calibration change also breaks continuity of the profile
    // and its ready evidence. Keep the word that exposed the change.
    if (drift) {
      const auto completedReads = live.verificationReads;
      device.clearLiveState();
      if (source == ConfigReadSource::VERIFICATION) live.verificationReads = completedReads;
    }
    live.offset = OffsetObservation{raw, temperature::decodeCounts(raw), transfer.completedMs};
    live.offsetContinuityLost = drift;
    if (source == ConfigReadSource::VERIFICATION) {
      live.verificationReads |= static_cast<std::uint8_t>(SettingsField::OFFSET);
    }
    if (drift) return Status::Error(Err::CONFIG_MISMATCH, "Unmanaged temperature offset changed", pointer);
    if (managed) {
      const auto match = compareExpectedRegister(device._config.model, *expected, pointer, raw);
      if (match.is(Err::CONFIG_MISMATCH)) markDirty(DirtyReason::READBACK_MISMATCH, SettingsField::OFFSET);
      return match;
    }
    return Status::Ok();
  }
  const auto field = fieldFor(pointer);
  if (!field) return Status::Error(Err::INVALID_PARAM, "Expected an owned settings register", pointer);
  const auto match = expected ? compareExpectedRegister(device._config.model, *expected, pointer, raw, live.trigger)
                              : Status::Ok();
  if (match.is(Err::CONFIG_MISMATCH)) markDirty(DirtyReason::READBACK_MISMATCH, *field);
  if (pointer == command::config) {
    captureConfig(raw, transfer.completedMs, source, timely);
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
  } else if (pointer == command::offset) {
    live.offset.reset();
  } else {
    (pointer == command::lowLimit ? live.low : live.high).reset();
  }
}

}  // namespace TMP11x::detail
