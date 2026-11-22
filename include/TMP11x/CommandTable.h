/// @file CommandTable.h
/// @brief Register pointers, positioned field masks and first-release access policy.
#pragma once

#include "TMP11x/Config.h"

namespace TMP11x::command {

inline constexpr std::uint8_t temperature = 0x00;  ///< TEMP read consumes DATA_READY.
inline constexpr std::uint8_t config = 0x01;       ///< Read clears DATA_READY and alert-mode HIGH/LOW.
inline constexpr std::uint8_t highLimit = 0x02;    ///< Signed high limit.
inline constexpr std::uint8_t lowLimit = 0x03;     ///< Signed low limit.
inline constexpr std::uint8_t eepromUnlock = 0x04; ///< EEPROM lock and busy observation.
inline constexpr std::uint8_t eeprom1 = 0x05;      ///< First EEPROM data shadow.
inline constexpr std::uint8_t eeprom2 = 0x06;      ///< Second EEPROM data shadow.
inline constexpr std::uint8_t eeprom3Tmp116 = 0x07; ///< Third EEPROM data shadow on TMP116/N.
inline constexpr std::uint8_t offset = 0x07;       ///< Temperature offset on TMP117/M/N and TMP119.
inline constexpr std::uint8_t lastEeprom = 0x08;   ///< EEPROM4 on TMP116/N, EEPROM3 on the other models.
inline constexpr std::uint8_t deviceId = 0x0F;     ///< Device ID and, when available, revision.

inline constexpr std::uint16_t wholeWord = 0xFFFF;     ///< TEMP, limits, data, offset and TMP116/N ID.
inline constexpr std::uint16_t baseIdMask = 0x0FFF;    ///< TMP117-like device ID field.
inline constexpr std::uint16_t revisionMask = 0xF000;  ///< TMP117-like revision field.

inline constexpr std::uint16_t highFlag = 0x8000;      ///< High alert latch or therm level.
inline constexpr std::uint16_t lowFlag = 0x4000;       ///< Latched LOW in alert mode. Reads zero in therm mode.
inline constexpr std::uint16_t dataReady = 0x2000;     ///< Conversion result updated.
inline constexpr std::uint16_t configBusy = 0x1000;    ///< EEPROM busy indication in CONFIG.
inline constexpr std::uint16_t modeMask = 0x0C00;      ///< MOD including alias and one-shot encodings.
inline constexpr std::uint16_t cycleMask = 0x0380;     ///< CONV selection.
inline constexpr std::uint16_t averagingMask = 0x0060; ///< AVG selection.
inline constexpr std::uint16_t therm = 0x0010;         ///< Select therm mode.
inline constexpr std::uint16_t polarity = 0x0008;      ///< Select active-high polarity.
inline constexpr std::uint16_t dataReadyPin = 0x0004;  ///< Route DATA_READY to ALERT.
inline constexpr std::uint16_t softReset = 0x0002;     ///< Explicit command on addressed-reset models.
inline constexpr std::uint16_t stableConfigMask = 0x07FC; ///< Canonical MOD0 and replayable bits 9:2.

/// Reserved CONFIG bits for the selected model's addressed-reset capability.
constexpr std::uint16_t configReserved(bool hasAddressedReset) noexcept {
  return hasAddressedReset ? 0x0001 : 0x0003;
}

inline constexpr std::uint16_t unlock = 0x8000;         ///< EUN in EEPROM_UL.
inline constexpr std::uint16_t unlockBusy = 0x4000;     ///< BUSY in EEPROM_UL.
inline constexpr std::uint16_t unlockReserved = 0x3FFF; ///< Unused bits in EEPROM_UL.

/// Release policy for ordinary register access. All mapped registers permit a word read.
enum class Access : std::uint8_t {
  INVALID,       ///< Unknown model or unmapped pointer.
  OBSERVE,       ///< Read-only in silicon (TEMP or device ID).
  PROTECTED_WORD, ///< Hardware R/W, library read-only (lock register or EEPROM data shadow).
  STABLE_CONFIG,  ///< Mixed status, setting and command fields. Only stable settings are replayable.
  GUARDED_LIMIT,  ///< Volatile limit write requires startup and lock/busy evidence.
  GUARDED_OFFSET, ///< Volatile offset write also requires an explicit management decision.
};

/// Look up the full pointer for an exact BOM model, without performing I2C.
/// Access classification alone does not establish the prerequisites for a write.
Access accessFor(Model model, std::uint8_t pointer) noexcept;

}  // namespace TMP11x::command
