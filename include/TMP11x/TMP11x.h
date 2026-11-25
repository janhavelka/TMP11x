/// @file TMP11x.h
/// @brief Device binding, verified volatile initialization, transport health and measurement evidence.
#pragma once

#include <cstdint>
#include <optional>

#include "TMP11x/Configuration.h"

namespace TMP11x {

namespace detail {
struct Acquisition;
struct Apply;
struct DeviceEvidence;
struct RegisterTransport;
struct TransferResult;
struct LatestRead;
/// Origin of a destructive CONFIG read that captured an alert.
enum class ConfigReadSource : std::uint8_t {
  APPLY,         ///< Guarded profile apply or replay.
  OBSERVATION,   ///< A single managed CONFIG query.
  VERIFICATION,  ///< Read-only profile verification.
  ACQUISITION,   ///< Freshness preflight, shutdown baseline or readiness poll.
};
}

/// Per-instance evidence since the last accepted bind, end or unbind.
/// Each tracked physical callback contributes one outcome, even before initialization.
struct HealthSnapshot {
  DriverState state = DriverState::UNINIT;  ///< State derived from tracked callbacks.
  bool initialized = false;                 ///< Managed initialization state at inspection.
  std::uint8_t consecutiveFailures = 0;     ///< Saturating failure streak, cleared by tracked success.
  std::uint32_t totalSuccess = 0;           ///< Saturating count of successful tracked callbacks.
  std::uint32_t totalFailures = 0;          ///< Saturating count of failed tracked callbacks.
  std::uint32_t lastSuccessMs = 0;          ///< Wrapping clock tick at the latest tracked success.
  std::uint32_t lastErrorMs = 0;            ///< Wrapping clock tick at the latest tracked failure.
  bool lastSuccessMsValid = false;         ///< A clock supplied lastSuccessMs, including a valid zero.
  bool lastErrorMsValid = false;           ///< A clock supplied lastErrorMs, including a valid zero.
  Status lastError{};                      ///< Latest tracked fault retained across success, or OK before any fault.
};

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

/// First unresolved cause of lost profile trust.
enum class DirtyReason : std::uint8_t {
  NONE,               ///< No unresolved change.
  READBACK_MISMATCH,  ///< A complete owned register differed from its expected value.
  AMBIGUOUS_WRITE,    ///< The callback failed after a write was attempted.
  INCOMPLETE_APPLY,   ///< Replay started writing and has yet to verify the entire target.
};

/// Register bits in SettingsSnapshot::affectedFields and SettingsSnapshot::verificationReads.
enum class SettingsField : std::uint8_t {
  CONFIG = 1,      ///< Stable CONFIG fields at 0x01.
  LOW_LIMIT = 2,   ///< Complete TLOW word at 0x03.
  HIGH_LIMIT = 4,  ///< Complete THIGH word at 0x02.
};

/// Complete threshold word and its signed value in units of 1/128 degree Celsius.
struct LimitObservation {
  std::uint16_t raw{};   ///< Two received bytes in register bit order.
  std::int16_t counts{}; ///< Signed two's-complement interpretation of raw.
  std::optional<std::uint32_t> observedAtMs;  ///< Wrapping completion tick when available.
};

/// Separate the software target, latest complete observations and whole-profile verification.
struct SettingsSnapshot {
  SensorSettings desired{};                       ///< Owned replay target.
  std::optional<ConfigurationObservation> config; ///< Last complete CONFIG since invalidation or write.
  std::optional<LimitObservation> low;             ///< Last complete TLOW since invalidation or write.
  std::optional<LimitObservation> high;            ///< Last complete THIGH since invalidation or write.
  ConfigTrust trust = ConfigTrust::UNKNOWN;        ///< Current live profile trust.
  DirtyReason dirtyReason = DirtyReason::NONE;     ///< Retained until verified replay or invalidation.
  std::uint8_t affectedFields = 0;                 ///< Registers written or found different while dirty.
  /// Fields read by the last explicit verification. The first replay write attempt or invalidation clears this mask.
  std::uint8_t verificationReads = 0;
  std::uint32_t verifiedGeneration = 0;            ///< Generation established by the last full replay.
  std::optional<std::uint32_t> verifiedAtMs;       ///< Last successful full check, including read-only verification.
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
  /// INVALID_CONFIG preserves the previous target and evidence. Acceptance starts
  /// a new observation history. A known EEPROM unlock still blocks mutation.
  /// Checks all settings, including ordered THERM limits. Hardware verification requires begin().
  Status bind(const Config& config) noexcept;

  /// Validate the candidate profile, clock and operation budget before replacing the binding,
  /// then run begin(). Once accepted, the new target remains desired even after an I2C failure.
  Status begin(const Config& config);

  /// Apply the bound settings under stable power and exclusive ownership of this sensor.
  /// Requires nowMs and at least one clock tick of operationTimeoutMs. Checks identity,
  /// waits for startup, reads EEPROM_UL for locked/idle and verifies CONFIG and both limits
  /// through guarded shutdown and restoration of the desired mode. CONFIG reads consume flags.
  /// A successful call admits readLatest(). Hardware offset provenance remains UNKNOWN.
  Status begin();

  /// Replay and verify the retained target with the same deadline and guards as begin().
  /// A failed write can leave partial changes and a DIRTY target. Recovery performs no reset
  /// or rollback. A known unlocked device requires external correction and invalidation.
  Status recover();

  /// Select continuous conversion or shutdown on an initialized, verified device.
  /// All three conversion setters use the binding's operation deadline and tracked
  /// transfers to check identity, EEPROM lock/busy and the current profile, enter
  /// shutdown, verify settings and restore the requested mode. Shutdown aborts a
  /// partial block. CONFIG reads acknowledge flags and retain alert evidence.
  /// Before the first write, a timely matching preflight can retain readiness
  /// for the current continuous generation.
  /// A timely final read establishes a new generation. After an attempted write,
  /// a failure retains the requested profile for recover(). Invalid selectors and
  /// known dirty profiles are rejected locally.
  Status setMode(Mode mode);

  /// Set the continuous CONV code through the same guarded sequence as setMode().
  /// Retains the desired mode and averaging. CONV has no effect on one-shot duration.
  Status setCycle(Cycle cycle);

  /// Select 1, 8, 32 or 64 conversions per result through the guarded setMode()
  /// sequence. Retains the desired mode and cycle. TEMP updates at block completion.
  Status setAveraging(Averaging averaging);

  /// Report loss of stable power or exclusive access. Drops initialization, startup proof
  /// and live observations without calling any hooks. Keeps the target, health, last
  /// verification and historical samples/events. After external correction of an EEPROM
  /// unlock, this permits a fresh locked/idle check. Requires an existing binding.
  Status invalidateDeviceState() noexcept;

  /// Read CONFIG once after initialization. Publishes a complete observation even on
  /// drift, latching DIRTY. Reading acknowledges DATA_READY and ALERT-mode latches.
  /// A failed transfer preserves @p out and may already have acknowledged hardware.
  Status readConfiguration(ConfigurationObservation& out);

  /// Read CONFIG, TLOW and THIGH after initialization. Each complete read records its
  /// evidence. A transport fault stops the scan. CONFIG_MISMATCH reports observed drift,
  /// CONFIG_DIRTY a matching scan with unresolved changes. Only guarded replay clears DIRTY.
  Status verifyConfiguration();

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

  /// Read the stored TEMP word once in either desired mode, including with dirty settings.
  /// Requires binding and managed initialization. A complete TEMP read acknowledges
  /// DATA_READY. It proves no new conversion, including when the word is 0x8000.
  /// On success, copy the sample to @p out and the cache with current profile evidence
  /// and an optional callback-completion timestamp. Any failure preserves both
  /// copies. An attempted TEMP read can consume DATA_READY even if it fails.
  Status readLatest(Sample& out);

  /// Try once for a fresh continuous result under the verified CONFIG and limits.
  /// Check EEPROM lock/busy, consume same-generation ready evidence and read TEMP
  /// in one combined transfer. At most three callbacks, zero writes or waits.
  /// MEASUREMENT_NOT_READY leaves @p out and the last good sample intact.
  /// Polling CONFIG acknowledges comparator alerts and can release ALERT.
  /// Continuous-update byte latching is undocumented. Offset provenance is UNKNOWN.
  Status tryReadFresh(Sample& out);

  /// Wait for a fresh continuous result, pacing CONFIG reads at the nominal block
  /// period. Requires verified continuous mode and a monotonic clock. @p timeoutMs
  /// covers all callbacks and waits and must span at least one clock tick.
  /// WORK_LIMIT reports exhaustion of the finite work allowance. Each callback
  /// releases the bus. All failures preserve @p out and the previous sample.
  Status readFreshBlocking(Sample& out, std::uint32_t timeoutMs);

  /// Acquire one conversion block in verified shutdown. Guard and compare CONFIG,
  /// write explicit shutdown, consume stale readiness, then trigger exactly once.
  /// Poll after the model's upper active estimate and require new DATA_READY.
  /// Completion returns the sensor to physical shutdown with a stable TEMP word
  /// under exclusive ownership. Desired mode remains shutdown. The clock, budget
  /// and output rules follow readFreshBlocking(). A timeout can leave a shot active.
  Status readOneShotBlocking(Sample& out, std::uint32_t timeoutMs);

  /// Read once through readLatest and return its Celsius value in @p out on success.
  /// Use readLatest to inspect freshness, timestamp validity and profile provenance.
  /// A failed read can consume DATA_READY and leaves @p out untouched.
  Status readTemperature(double& out);

  /// Whether this instance holds an accepted target. All inspectors are bus-silent.
  bool isBound() const noexcept { return _config.model != Model::UNSELECTED; }
  /// Whether managed initialization has succeeded since binding or invalidation.
  /// Inspect configTrust() separately after a failed replay.
  bool isInitialized() const noexcept { return _live.initialized; }
  /// Copy the current initialization flag and tracked transport history without calling any hooks.
  HealthSnapshot healthSnapshot() const noexcept;
  /// True when initialized and the last tracked state is READY or DEGRADED.
  /// Reads memory only. Profile trust and conversion freshness have separate evidence.
  bool isOnline() const noexcept;
  /// Copy the selected target and most recent complete ID, including any mismatch.
  IdentityInfo identityInfo() const noexcept { return _identity; }
  /// Copy the software target. An unbound device returns the default settings.
  SensorSettings desiredSettings() const noexcept { return _config.settings; }
  /// Return the current trust in the sensor's live configuration.
  ConfigTrust configTrust() const noexcept { return _live.trust; }
  /// Copy the target, observations and trust evidence without calling the bus or clock.
  SettingsSnapshot getSettingsSnapshot() const noexcept;

 private:
  friend struct detail::Acquisition;
  friend struct detail::Apply;
  friend struct detail::DeviceEvidence;
  friend struct detail::RegisterTransport;
  friend struct detail::LatestRead;

  // Current device knowledge expires on teardown or incompatible identity.
  // A pending FIRST_ACK origin belongs only to the operation that read the ID.
  struct LiveState {
    bool startupReady = false;
    bool initialized = false;
    ConfigTrust trust = ConfigTrust::UNKNOWN;
    std::uint32_t profileGeneration = 0;
    std::optional<std::uint32_t> verifiedAtMs;
    DirtyReason dirtyReason = DirtyReason::NONE;
    std::uint8_t affectedFields = 0;
    std::uint8_t verificationReads = 0;
    std::optional<ConfigurationObservation> config;
    std::optional<LimitObservation> low;
    std::optional<LimitObservation> high;
    std::optional<std::uint32_t> baselineMs;
    bool readyPending = false;
    bool readyHistoryUncertain = false;
    OwnedTrigger trigger = OwnedTrigger::NONE;
  };

  // Pending ALERT-mode events captured by CONFIG reads. THERM HIGH is
  // the level at the last applicable observation and is never latched here.
  struct AlertEvidence {
    bool pendingHigh = false;
    bool pendingLow = false;
    bool possibleLoss = false;
    bool mixedOrUnknown = false;
    std::optional<bool> thermHigh;
    std::optional<std::uint32_t> observedAtMs;
    std::uint32_t profileGeneration = 0;
    detail::ConfigReadSource source{};
    ObservedMode mode{};
  };

  void clearObservations() noexcept;
  void clearLiveState() noexcept;
  Status requireInitialized() const noexcept;
  Status applySettings(const SensorSettings& candidate);

  Config _config{};
  IdentityInfo _identity{};
  LiveState _live{};
  AlertEvidence _alerts{};
  // Sticky across ordinary begin/recover, teardown and rebind. The owner must
  // correct a known unlocked part before reporting device-state invalidation.
  bool _eepromUnsafe = false;
  HealthSnapshot _health{};
  std::optional<Sample> _lastSample;
};

}  // namespace TMP11x
