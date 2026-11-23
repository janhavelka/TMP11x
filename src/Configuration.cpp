#include "TMP11x/Configuration.h"

#include "TMP11x/CommandTable.h"
#include "TMP11x/Temperature.h"

namespace TMP11x {
namespace {

std::uint16_t packConfiguration(const SensorSettings& settings) noexcept {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(settings.mode) << 10) |
      (static_cast<std::uint16_t>(settings.cycle) << 7) |
      (static_cast<std::uint16_t>(settings.averaging) << 5) |
      (static_cast<std::uint16_t>(settings.alertMode) << 4) |
      (static_cast<std::uint16_t>(settings.polarity) << 3) |
      (static_cast<std::uint16_t>(settings.pinFunction) << 2));
}

}  // namespace

Status validateSettings(Model model, const SensorSettings& settings) noexcept {
  if (!isValidModel(model)) return Status::Error(Err::INVALID_CONFIG, "Invalid BOM model");
  if (!isValidMode(settings.mode) || !isValidCycle(settings.cycle) ||
      !isValidAveraging(settings.averaging) || !isValidAlertMode(settings.alertMode) ||
      !isValidPolarity(settings.polarity) || !isValidPinFunction(settings.pinFunction)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid CONFIG selector");
  }
  if (settings.alertMode == AlertMode::THERM && settings.limits.lowCounts > settings.limits.highCounts) {
    return Status::Error(Err::INVALID_CONFIG, "THERM low limit exceeds high limit");
  }
  return Status::Ok();
}

Status encodeConfiguration(Model model, const SensorSettings& settings, std::uint16_t& out) noexcept {
  const auto status = validateSettings(model, settings);
  if (!status.ok()) return status;
  out = packConfiguration(settings);
  return Status::Ok();
}

std::uint8_t ConfigurationObservation::rawMode() const noexcept {
  return static_cast<std::uint8_t>((raw & command::modeMask) >> 10);
}

ConfigurationObservation decodeConfiguration(std::uint16_t raw) noexcept {
  ConfigurationObservation result{};
  result.raw = raw;
  switch (result.rawMode()) {
    case 1: result.mode = ObservedMode::SHUTDOWN; break;
    case 3: result.mode = ObservedMode::ONE_SHOT; break;
    default: result.mode = ObservedMode::CONTINUOUS; break;
  }
  result.cycle = static_cast<Cycle>((raw & command::cycleMask) >> 7);
  result.averaging = static_cast<Averaging>((raw & command::averagingMask) >> 5);
  result.alertMode = (raw & command::therm) ? AlertMode::THERM : AlertMode::ALERT;
  result.polarity = (raw & command::polarity) ? Polarity::ACTIVE_HIGH : Polarity::ACTIVE_LOW;
  result.pinFunction = (raw & command::dataReadyPin) ? PinFunction::DATA_READY : PinFunction::THRESHOLD;
  result.high = (raw & command::highFlag) != 0;
  result.low = (raw & command::lowFlag) != 0;
  result.dataReady = (raw & command::dataReady) != 0;
  result.eepromBusy = (raw & command::configBusy) != 0;
  result.readClearMask = command::dataReady;
  if (result.alertMode == AlertMode::ALERT) result.readClearMask |= command::highFlag | command::lowFlag;
  return result;
}

Status compareExpectedRegister(Model model, const SensorSettings& settings, std::uint8_t pointer,
                               std::uint16_t observed, OwnedTrigger trigger) noexcept {
  const auto status = validateSettings(model, settings);
  if (!status.ok()) return status;

  if (trigger != OwnedTrigger::NONE && trigger != OwnedTrigger::CONFIRMED) {
    return Status::Error(Err::INVALID_PARAM, "Invalid one-shot trigger evidence");
  }
  bool matches;
  switch (pointer) {
    case command::config: {
      // Check OS before dropping MOD1. With ownership established, MOD0 then
      // compares both continuous encodings as 0 and the two shutdown-compatible codes as 1.
      const bool oneShot = (observed & command::modeMask) == command::modeMask;
      matches = (!oneShot || trigger == OwnedTrigger::CONFIRMED) &&
                (observed & command::stableConfigMask) == packConfiguration(settings);
      break;
    }
    case command::highLimit:
      matches = temperature::decodeCounts(observed) == settings.limits.highCounts;
      break;
    case command::lowLimit:
      matches = temperature::decodeCounts(observed) == settings.limits.lowCounts;
      break;
    default: return Status::Error(Err::INVALID_PARAM, "Expected CONFIG, THIGH or TLOW");
  }
  if (!matches) return Status::Error(Err::CONFIG_MISMATCH, "Register differs from desired settings", pointer);
  return Status::Ok();
}

}  // namespace TMP11x
