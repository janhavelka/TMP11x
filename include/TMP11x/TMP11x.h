/// @file TMP11x.h
/// @brief Device binding, identity diagnostics and measurement evidence.
#pragma once

#include <cstdint>
#include <optional>

#include "TMP11x/Config.h"

namespace TMP11x {

/// Selected target and complete ID evidence when available.
/// TMP116/N requires the full word 0x1116 and has no decoded revision.
/// TMP117/M/N and TMP119 require bits 11:0 to equal 0x117. Bits 15:12
/// carry the revision, which is valid even when zero.
/// The group describes the observed word even when it is incompatible with
/// the selection. The exact model and grade come from the BOM.
/// An explicit ID read publishes a complete word even on IDENTITY_MISMATCH.
/// A rejected or failed transfer leaves the previous observation intact.
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

/// One application-owned sensor and its evidence. Serialize all access, including
/// getters, outside the instance. Each callback must finish before returning.
/// Keep the instance at a stable address and its borrowed resources alive while bound.
class TMP11x {
 public:
  TMP11x() = default;  ///< Construct an unbound device without invoking any hooks.
  TMP11x(const TMP11x&) = delete;
  TMP11x& operator=(const TMP11x&) = delete;
  TMP11x(TMP11x&&) = delete;
  TMP11x& operator=(TMP11x&&) = delete;

  /// Copy the selected target, hooks, transport policy and desired settings without I/O.
  /// INVALID_CONFIG preserves the previous target and evidence. Acceptance discards
  /// all previous observations, even on a same-target rebind after an external reset.
  /// Only the binding structure is validated here. Sensor settings remain unverified.
  Status bind(const Config& config) noexcept;

  /// Clear local observations, initialization and startup proof, keeping the bound
  /// target and desired settings. Safe to repeat. The physical power mode is untouched.
  Status end() noexcept;

  /// Clear local evidence and release the binding and its borrowed hooks without I/O.
  /// Safe to repeat on an empty instance.
  Status unbind() noexcept;

  /// Read DEVICE_ID once using the bound callback cap. Works before initialization
  /// and without a clock. A complete word updates @p out and the cached identity,
  /// including on IDENTITY_MISMATCH. A mismatch also invalidates live profile and
  /// startup evidence. NOT_BOUND and failed transfers preserve @p out and the cache.
  Status readIdentity(IdentityInfo& out);

  /// Perform the same health-neutral ID check as readIdentity and cache its evidence.
  /// Neither diagnostic establishes initialization or a power-on time.
  Status probe();

  /// Whether this instance holds an accepted target. All inspectors are bus-silent.
  bool isBound() const noexcept { return _config.model != Model::UNSELECTED; }
  /// Whether managed initialization has verified the sensor profile.
  bool isInitialized() const noexcept { return _live.initialized; }
  /// Copy the selected target and most recent complete ID, including any mismatch.
  IdentityInfo identityInfo() const noexcept { return _identity; }
  /// Copy the software target. An unbound device returns the default settings.
  SensorSettings desiredSettings() const noexcept { return _config.settings; }
  /// Return the current trust in the sensor's live configuration.
  ConfigTrust configTrust() const noexcept { return _live.trust; }

 private:
  // These facts expire together on teardown or incompatible identity. A timed
  // initialization operation owns the first compatible ACK timestamp and proof.
  struct LiveState {
    std::optional<std::uint32_t> firstAckMs;
    bool startupReady = false;
    bool initialized = false;
    ConfigTrust trust = ConfigTrust::UNKNOWN;
  };

  void clearObservations() noexcept;

  Config _config{};
  IdentityInfo _identity{};
  LiveState _live{};
};

}  // namespace TMP11x
