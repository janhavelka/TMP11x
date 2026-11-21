/// @file TMP11x.h
/// @brief Identity and measurement evidence for the portable TMP11x interface.
#pragma once

#include <cstdint>

#include "TMP11x/Config.h"

namespace TMP11x {

/// Selected target and complete ID evidence when available.
struct IdentityInfo {
  Model model = Model::UNSELECTED;               ///< Exact model selected from the BOM.
  std::uint8_t i2cAddress = 0;                   ///< Bound unshifted seven-bit address.
  std::uint16_t raw = 0;                         ///< Complete raw ID word when rawValid is true.
  bool rawValid = false;                         ///< A complete ID word was observed.
  IdentityGroup group = IdentityGroup::UNKNOWN;  ///< Group recognized from the raw ID.
  std::uint8_t revision = 0;                     ///< Raw revision nibble when revisionValid is true.
  bool revisionValid = false;                    ///< The recognized group defines a revision field.
  bool compatible = false;                       ///< A complete observed word matches the selected model's ID rule.
};

/// Acquisition method used to obtain a sample.
enum class ReadKind : std::uint8_t {
  LATEST,            ///< Read the stored temperature word.
  CONTINUOUS_FRESH,  ///< Require a new continuous conversion for the current profile.
  ONE_SHOT,          ///< Require completion of an explicitly owned one-shot conversion.
};

/// Live profile evidence recorded at the time of an observation.
enum class ConfigTrust : std::uint8_t {
  UNKNOWN,   ///< No current profile verification evidence.
  DIRTY,     ///< An unresolved change or mismatch requires verified replay.
  VERIFIED,  ///< Owned settings were verified for the current profile generation.
};

/// Origin and trust of a sample's hardware-offset evidence.
enum class OffsetState : std::uint8_t {
  UNKNOWN,             ///< No offset evidence recorded.
  UNAVAILABLE,         ///< The selected model lacks a hardware offset.
  OBSERVED_UNMANAGED,  ///< Counts observed without taking ownership of the offset.
  VERIFIED_MANAGED,    ///< Counts verified as part of the owned profile.
};

/// Observed or verified hardware offset, independent of conversion attribution.
struct OffsetProvenance {
  OffsetState state = OffsetState::UNKNOWN;  ///< Origin and trust of the evidence.
  std::int16_t counts = 0;                   ///< Offset in units of 1/128 degree Celsius.
  bool countsValid = false;                  ///< The count field carries offset evidence.
};

/// A temperature word with separate acquisition, time, profile and freshness evidence.
/// A default value supplies no measurement evidence. The numeric value alone cannot
/// establish startup validity or freshness. Raw 0x8000 can be a startup or saturated word.
struct Sample {
  std::uint16_t raw = 0;                           ///< Temperature register word.
  std::int16_t counts = 0;                         ///< Exact signed counts of 1/128 degree Celsius.
  double celsius = 0.0;                            ///< Decoded temperature in degrees Celsius.
  std::uint32_t observedAtMs = 0;                  ///< Floor tick at read completion in the binding's epoch.
  bool timestampValid = false;                     ///< The supplied timestamp is valid, including zero.
  ReadKind acquisitionKind = ReadKind::LATEST;     ///< Acquisition method.
  bool freshConversion = false;                    ///< A new conversion was confirmed for this acquisition.
  std::uint32_t profileGeneration = 0;             ///< Verified profile generation at observation, if known.
  ConfigTrust configTrust = ConfigTrust::UNKNOWN;  ///< Profile trust at observation.
  OffsetProvenance offset{};                       ///< Hardware-offset evidence at observation.
};

}  // namespace TMP11x
