/// @file Configuration.h
/// @brief Bus-silent CONFIG word conversion and owned register comparisons for all six models.
#pragma once

#include "TMP11x/Config.h"

namespace TMP11x {

/// Interpretation of the observed MOD field. Both continuous encodings map to CONTINUOUS.
enum class ObservedMode : std::uint8_t { CONTINUOUS, SHUTDOWN, ONE_SHOT };

/// Evidence supplied by the operation that issued a one-shot command.
enum class OwnedTrigger : std::uint8_t {
  NONE,       ///< No retained physical trigger success for the current device.
  CONFIRMED,  ///< Terminal physical success, including late success, since the last successful SD or invalidation.
};

/// Fields from one complete CONFIG read. A read consumes DATA_READY in either alert mode.
/// In ALERT it also acknowledges both comparator latches. In THERM, HIGH is the
/// hysteresis level and LOW reads zero. The raw flags preserve exactly what was observed.
struct ConfigurationObservation {
  std::uint16_t raw{};             ///< Entire observed word, including status and reserved bits.
  ObservedMode mode{};             ///< Decoded MOD. A mode observation alone cannot establish freshness.
  Cycle cycle{};                   ///< Conversion cycle selector.
  Averaging averaging{};           ///< Averaging selector.
  AlertMode alertMode{};           ///< Comparator behavior at the read.
  Polarity polarity{};             ///< Assertion polarity of the physical ALERT pin.
  PinFunction pinFunction{};       ///< Signal routed to the ALERT pin.
  bool high{};                     ///< Raw HIGH flag, interpreted according to alertMode.
  bool low{};                      ///< Raw LOW flag, an event only in ALERT mode.
  bool dataReady{};                ///< Raw DATA_READY flag before acknowledgement.
  bool eepromBusy{};               ///< CONFIG busy indication. Lock evidence requires EEPROM_UL at 0x04.
  std::uint16_t readClearMask{};   ///< Flag positions acknowledged by the read, even if their value was zero.
  std::uint32_t observedAtMs = 0;  ///< Optional completion tick supplied by the read owner.
  bool timestampValid = false;     ///< True when the read owner supplied the tick, including zero.

  /// Original two-bit MOD code, including the continuous alias (2) and one-shot (3).
  std::uint8_t rawMode() const noexcept;
};

/// Check an exact BOM selection, the six replayable selectors and signed limits.
/// THERM requires lowCounts <= highCounts. ALERT accepts independent limits.
/// An engaged managed offset on TMP116/N returns UNSUPPORTED.
Status validateSettings(Model model, const SensorSettings& settings) noexcept;

/// Encode the validated profile's CONFIG fields. Emits MOD=00 for continuous or 01 for
/// shutdown, with status, reserved and reset bits clear. INVALID_CONFIG preserves @p out.
Status encodeConfiguration(Model model, const SensorSettings& settings, std::uint16_t& out) noexcept;

/// Decode any complete CONFIG word using the field layout shared by all supported models.
/// This pure conversion describes the effects of the original read and supplies no timestamp.
ConfigurationObservation decodeConfiguration(std::uint16_t raw) noexcept;

/// Compare a complete CONFIG, THIGH, TLOW or explicitly managed OFFSET read with the desired profile.
/// CONFIG ignores status and bits 1:0. Only a confirmed owned trigger permits observed
/// MOD=11 for desired shutdown. Retain that evidence through completion, cancellation or
/// timeout until successful explicit SD or invalidation. An ambiguous write cannot confirm
/// a trigger or shutdown. The caller owns trust, provenance and each new guarded SD baseline.
/// Limits and managed OFFSET use the full signed word. An unmanaged OFFSET has no comparison target.
/// Returns CONFIG_MISMATCH for drift, UNSUPPORTED for an unsupported managed offset,
/// INVALID_CONFIG for an invalid profile/model, or INVALID_PARAM for an invalid pointer or trigger value.
Status compareExpectedRegister(Model model, const SensorSettings& settings, std::uint8_t pointer,
                               std::uint16_t observed, OwnedTrigger trigger = OwnedTrigger::NONE) noexcept;

}  // namespace TMP11x
