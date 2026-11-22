/// @file Temperature.h
/// @brief Pure conversions for signed TEMP, THIGH and TLOW register words.
#pragma once

#include <cstdint>

#include "TMP11x/Status.h"

namespace TMP11x::temperature {

/// Decode the signed 16-bit register representation into counts of 1/128 degree Celsius.
/// Every word has a numeric value, including 0x8000 during startup.
constexpr std::int16_t decodeCounts(std::uint16_t raw) noexcept {
  std::int32_t counts = raw;
  if ((raw & 0x8000u) != 0) counts -= 65536;
  return static_cast<std::int16_t>(counts);
}

/// Convert signed register counts to degrees Celsius, including both arithmetic endpoints.
/// Consult the selected model's datasheet for its specified measurement and accuracy ranges.
constexpr double toCelsius(std::int16_t counts) noexcept {
  return counts / 128.0;
}

/// Encode a finite value in [-256, 255.9921875] degrees Celsius as a register word.
/// Round to the nearest count, with exact half counts rounded away from zero.
/// INVALID_PARAM preserves @p out for nonfinite or out-of-range input.
Status encodeCelsius(double celsius, std::uint16_t& out) noexcept;

}  // namespace TMP11x::temperature
