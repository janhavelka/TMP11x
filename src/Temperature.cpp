#include "TMP11x/Temperature.h"

#include <cmath>
#include <limits>

namespace TMP11x::temperature {

Status encodeCelsius(double celsius, std::uint16_t& out) noexcept {
  constexpr auto minCounts = std::numeric_limits<std::int16_t>::min();
  constexpr auto maxCounts = std::numeric_limits<std::int16_t>::max();
  if (!std::isfinite(celsius) || celsius < toCelsius(minCounts) || celsius > toCelsius(maxCounts)) {
    return Status::Error(Err::INVALID_PARAM, "Temperature outside signed register range");
  }
  const double counts = std::round(celsius * 128.0);
  if (counts < minCounts || counts > maxCounts) {
    return Status::Error(Err::INVALID_PARAM, "Rounded counts outside signed register range");
  }
  out = static_cast<std::uint16_t>(static_cast<std::int16_t>(counts));
  return Status::Ok();
}

}  // namespace TMP11x::temperature
