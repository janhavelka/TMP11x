/// @file Status.h
/// @brief Portable results and passive transport health states.
#pragma once

#include <cstdint>

namespace TMP11x {

/// Result codes. Transport failures describe one adapter callback.
enum class Err : std::uint8_t {
  OK = 0,                 ///< The requested action completed successfully.
  IN_PROGRESS,            ///< Owner-driven work is pending.
  INVALID_PARAM,          ///< An argument is outside the supported domain.
  INVALID_CONFIG,         ///< A binding or settings profile is invalid for this operation.
  NOT_BOUND,              ///< No binding is installed.
  NOT_INITIALIZED,        ///< Managed initialization is required.
  UNSUPPORTED,            ///< The selected model or binding lacks the requested capability.
  CLOCK_REQUIRED,         ///< The operation requires an elapsed-time callback.
  BUSY,                   ///< An operation or its pending result owns the instance.
  IDENTITY_MISMATCH,      ///< A complete ID is incompatible with the selected model.
  EEPROM_UNSAFE,          ///< The device reported an unlocked EEPROM.
  CONFIG_DIRTY,           ///< The managed profile requires verified replay.
  CONFIG_MISMATCH,        ///< A complete observation disagrees with expected settings.
  MEASUREMENT_NOT_READY,  ///< The requested measurement evidence is unavailable.
  OPERATION_TIMEOUT,      ///< The whole-operation deadline expired.
  WORK_LIMIT,             ///< An operation exhausted its bounded work or token space.
  CANCELLED,              ///< The owner cancelled an admitted operation.
  RESULT_NOT_AVAILABLE,   ///< No active operation or retained result is available for the call.
  TOKEN_MISMATCH,         ///< A token does not identify the retained result.
  I2C_ERROR,              ///< Unclassified transport failure or incoherent adapter status.
  I2C_NACK,               ///< The adapter reported a NACK of unknown phase.
  I2C_NACK_ADDR,          ///< The adapter identified an address-phase NACK.
  I2C_NACK_DATA,          ///< The adapter identified a data-phase NACK.
  I2C_SHORT_TRANSFER,     ///< The adapter detected an incomplete transfer.
  I2C_TIMEOUT,            ///< The adapter reported a transport timeout.
  I2C_BUS,                ///< The adapter identified a bus fault.
};

/// Return the static symbolic name of @p err, or "UNKNOWN" for an invalid code.
constexpr const char* errorName(Err err) noexcept {
  switch (err) {
    case Err::OK: return "OK";
    case Err::IN_PROGRESS: return "IN_PROGRESS";
    case Err::INVALID_PARAM: return "INVALID_PARAM";
    case Err::INVALID_CONFIG: return "INVALID_CONFIG";
    case Err::NOT_BOUND: return "NOT_BOUND";
    case Err::NOT_INITIALIZED: return "NOT_INITIALIZED";
    case Err::UNSUPPORTED: return "UNSUPPORTED";
    case Err::CLOCK_REQUIRED: return "CLOCK_REQUIRED";
    case Err::BUSY: return "BUSY";
    case Err::IDENTITY_MISMATCH: return "IDENTITY_MISMATCH";
    case Err::EEPROM_UNSAFE: return "EEPROM_UNSAFE";
    case Err::CONFIG_DIRTY: return "CONFIG_DIRTY";
    case Err::CONFIG_MISMATCH: return "CONFIG_MISMATCH";
    case Err::MEASUREMENT_NOT_READY: return "MEASUREMENT_NOT_READY";
    case Err::OPERATION_TIMEOUT: return "OPERATION_TIMEOUT";
    case Err::WORK_LIMIT: return "WORK_LIMIT";
    case Err::CANCELLED: return "CANCELLED";
    case Err::RESULT_NOT_AVAILABLE: return "RESULT_NOT_AVAILABLE";
    case Err::TOKEN_MISMATCH: return "TOKEN_MISMATCH";
    case Err::I2C_ERROR: return "I2C_ERROR";
    case Err::I2C_NACK: return "I2C_NACK";
    case Err::I2C_NACK_ADDR: return "I2C_NACK_ADDR";
    case Err::I2C_NACK_DATA: return "I2C_NACK_DATA";
    case Err::I2C_SHORT_TRANSFER: return "I2C_SHORT_TRANSFER";
    case Err::I2C_TIMEOUT: return "I2C_TIMEOUT";
    case Err::I2C_BUS: return "I2C_BUS";
  }
  return "UNKNOWN";
}

/// Copyable result. The caller supplies static-lifetime text for @ref msg.
struct Status {
  Err code = Err::OK;       ///< Typed result.
  std::int32_t detail = 0;  ///< Signed diagnostic detail, such as a native adapter code.
  const char* msg = "OK";   ///< Borrowed pointer to static-lifetime text.

  /// True on successful completion.
  constexpr bool ok() const noexcept { return is(Err::OK); }
  /// Compare the typed result with @p value.
  constexpr bool is(Err value) const noexcept { return code == value; }
  /// True while the owner can advance admitted work.
  constexpr bool inProgress() const noexcept { return is(Err::IN_PROGRESS); }

  /// Construct a successful result.
  static constexpr Status Ok() noexcept { return {}; }
  /// Store a result code, borrowed static @p message and optional diagnostic @p detail.
  static constexpr Status Error(Err value, const char* message, std::int32_t detail = 0) noexcept {
    return {value, detail, message};
  }
};

/// Passive transport evidence, independent of managed initialization and profile trust.
enum class DriverState : std::uint8_t {
  UNINIT,    ///< No tracked transfer outcome since bind or teardown.
  READY,     ///< The latest tracked transfer succeeded.
  DEGRADED,  ///< Consecutive tracked failures below the offline threshold.
  OFFLINE,   ///< Consecutive tracked failures at or above the offline threshold.
};

}  // namespace TMP11x
