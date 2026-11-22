/// @file Config.h
/// @brief Exact model selection, borrowed transport binding and stable sensor settings.
#pragma once

#include <cstddef>
#include <cstdint>

#include "TMP11x/Status.h"

namespace TMP11x {

/// Exact BOM models. UNSELECTED requires an explicit choice before binding.
enum class Model : std::uint8_t { UNSELECTED = 0, TMP116, TMP116N, TMP117, TMP117M, TMP117N, TMP119 };

/// Groups recognizable from the documented device ID fields.
enum class IdentityGroup : std::uint8_t {
  UNKNOWN,      ///< No compatible ID pattern recognized or no complete word observed.
  TMP116,       ///< Full-word ID shared by TMP116 and TMP116N.
  TMP117_LIKE,  ///< Base ID shared by TMP117, TMP117M, TMP117N and TMP119.
};

/// Hardware capabilities of a selected BOM model, independent of the
/// operations available in this library version.
struct Capabilities {
  bool hasOffset = false;            ///< Model supports a hardware temperature offset.
  bool hasAddressedReset = false;    ///< Model supports an addressed CONFIG reset command.
  std::uint8_t eepromWordCount = 0;  ///< Number of readable 16-bit EEPROM data shadows.
};

/// True only for the six supported exact BOM model selections.
bool isValidModel(Model model) noexcept;

/// Copy the selected model's capabilities without I/O. INVALID_PARAM preserves @p out.
Status getCapabilities(Model model, Capabilities& out) noexcept;

/// True for unshifted seven-bit addresses 0x48 through 0x4B on the supported models.
constexpr bool isValidAddress(std::uint8_t address) noexcept {
  return address >= 0x48 && address <= 0x4B;
}

/// Stable desired operating mode. One-shot is a separate operation.
enum class Mode : std::uint8_t { CONTINUOUS = 0, SHUTDOWN = 1 };

/// CONV selector in TI table order. The effective period also depends on averaging.
enum class Cycle : std::uint8_t { CODE_0 = 0, CODE_1, CODE_2, CODE_3, CODE_4, CODE_5, CODE_6, CODE_7 };

/// Number of conversions averaged per result. The enumerators encode the AVG field.
enum class Averaging : std::uint8_t { SAMPLES_1 = 0, SAMPLES_8, SAMPLES_32, SAMPLES_64 };

/// Threshold alert behavior or therm hysteresis behavior.
enum class AlertMode : std::uint8_t { ALERT = 0, THERM = 1 };
/// Polarity of the physical ALERT pin assertion.
enum class Polarity : std::uint8_t { ACTIVE_LOW = 0, ACTIVE_HIGH = 1 };
/// Source routed to the physical ALERT pin.
enum class PinFunction : std::uint8_t { THRESHOLD = 0, DATA_READY = 1 };

/// True for stable settings. Raw continuous alias and one-shot encodings are excluded.
constexpr bool isValidMode(Mode mode) noexcept { return mode <= Mode::SHUTDOWN; }
/// True for the eight documented CONV codes.
constexpr bool isValidCycle(Cycle cycle) noexcept { return cycle <= Cycle::CODE_7; }
/// True for the four documented AVG codes.
constexpr bool isValidAveraging(Averaging averaging) noexcept {
  return averaging <= Averaging::SAMPLES_64;
}
/// True for either documented alert mode.
constexpr bool isValidAlertMode(AlertMode mode) noexcept { return mode <= AlertMode::THERM; }
/// True for either documented assertion polarity.
constexpr bool isValidPolarity(Polarity polarity) noexcept { return polarity <= Polarity::ACTIVE_HIGH; }
/// True for either documented ALERT pin source.
constexpr bool isValidPinFunction(PinFunction function) noexcept {
  return function <= PinFunction::DATA_READY;
}

/// Exact signed limit values in counts of 1/128 degree Celsius.
struct Limits {
  std::int16_t lowCounts = -32768;  ///< Low limit, defaults to -256 degrees Celsius.
  std::int16_t highCounts = 24576;  ///< High limit, defaults to 192 degrees Celsius.
};

/// Copyable desired profile. Defaults are software targets until verified on hardware.
struct SensorSettings {
  Mode mode = Mode::CONTINUOUS;                      ///< Replayable mode.
  Cycle cycle = Cycle::CODE_4;                       ///< CONV field selector.
  Averaging averaging = Averaging::SAMPLES_8;        ///< AVG field selector.
  AlertMode alertMode = AlertMode::ALERT;            ///< Threshold event semantics.
  Polarity polarity = Polarity::ACTIVE_LOW;          ///< Pin assertion polarity.
  PinFunction pinFunction = PinFunction::THRESHOLD;  ///< Pin signal source.
  Limits limits{};                                   ///< Both limit words. THERM requires lowCounts <= highCounts.
};

/// Complete one synchronous write to a seven-bit address within the supplied ms budget.
/// Return terminal status. OK confirms all bytes transferred. Borrow the TX buffer only
/// for the call. The budget includes lock acquisition and the adapter must report shorts.
/// A register write supplies three bytes in order: pointer, MSB, LSB.
using I2cWriteFn = Status (*)(std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                              std::uint32_t timeoutMs, void* user);

/// Complete one synchronous, combined write/read with a repeated START within the ms budget.
/// OK confirms both lengths. Lend neither buffer beyond return and report a short transfer
/// as an error. The same terminal status and lock budget rules as I2cWriteFn apply.
/// A word read sends one pointer, reads the MSB with ACK, then the LSB with NACK and STOP.
using I2cWriteReadFn = Status (*)(std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                                  std::uint8_t* rx, std::size_t rxLen, std::uint32_t timeoutMs,
                                  void* user);

/// Report monotonic elapsed milliseconds floored to the clock tick and truncated to 32 bits.
using NowMsFn = std::uint32_t (*)(void* user);
/// Synchronously report the physical GPIO level in @p high on success with terminal status.
using GpioReadFn = Status (*)(bool& high, void* user);

/// Binding and desired profile copied on acceptance. The owner serializes access, keeps
/// resources and contexts alive through the last callback, and prevents callback re-entry.
/// Contexts may be null when the corresponding callbacks support it.
struct Config {
  Model model = Model::UNSELECTED;          ///< Explicit BOM choice required.
  std::uint8_t i2cAddress = 0;              ///< Explicit seven-bit address required.
  I2cWriteFn i2cWrite = nullptr;            ///< Required write callback.
  I2cWriteReadFn i2cWriteRead = nullptr;    ///< Required combined write/read callback.
  void* i2cUser = nullptr;                  ///< Borrowed context shared by both bus callbacks.
  std::uint32_t i2cTimeoutMs = 50;          ///< Per-callback cap in ms, in [1, INT32_MAX].
  std::uint32_t operationTimeoutMs = 1000;  ///< Synchronous apply/reset budget in ms, in [1, INT32_MAX].
  NowMsFn nowMs = nullptr;                  ///< Optional for observations, required for timed work.
  void* timeUser = nullptr;                 ///< Borrowed clock context.
  std::uint32_t clockResolutionMs = 1;      ///< Clock tick in whole ms, in [1, INT32_MAX].
  GpioReadFn readAlertPin = nullptr;        ///< Optional physical ALERT level callback.
  void* gpioUser = nullptr;                 ///< Borrowed GPIO context.
  std::uint8_t offlineThreshold = 5;        ///< Positive consecutive tracked failure threshold.
  SensorSettings settings{};                ///< Stable desired settings and limit counts.
};

}  // namespace TMP11x
