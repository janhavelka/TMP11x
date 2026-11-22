#include "TMP11x/Temperature.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {

using namespace TMP11x;

#define CHECK(condition)                                                         \
  do {                                                                           \
    if (!(condition)) {                                                          \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);       \
      std::exit(EXIT_FAILURE);                                                   \
    }                                                                            \
  } while (false)

void signedRegisterVectors() {
  // temperature_and_limits from docs/registers/behavior-vectors.json.
  constexpr struct {
    std::uint16_t word;
    std::int16_t counts;
    double celsius;
  } cases[] = {
      {0x8000, -32768, -256.0},    {0xF380, -3200, -25.0},    {0xFAC0, -1344, -10.5},
      {0xFCC0, -832, -6.5},        {0xFF80, -128, -1.0},      {0xFFFF, -1, -0.0078125},
      {0x0000, 0, 0.0},            {0x0001, 1, 0.0078125},    {0x0C80, 3200, 25.0},
      {0x6000, 24576, 192.0},      {0x7FFF, 32767, 255.9921875},
  };
  for (const auto& row : cases) {
    CHECK(temperature::decodeCounts(row.word) == row.counts);
    CHECK(temperature::toCelsius(row.counts) == row.celsius);
    std::uint16_t encoded = 0x5A5A;
    CHECK(temperature::encodeCelsius(row.celsius, encoded).ok());
    CHECK(encoded == row.word);
  }
}

void nearestCountTiesAwayFromZero() {
  constexpr struct {
    double celsius;
    std::uint16_t word;
  } cases[] = {
      {-0.00390624, 0x0000}, {-0.00390625, 0xFFFF}, {-0.00390626, 0xFFFF},
      {0.00390624, 0x0000},  {0.00390625, 0x0001},  {0.00390626, 0x0001},
      {-6.50390625, 0xFCBF},
  };
  for (const auto& row : cases) {
    std::uint16_t encoded = 0x5A5A;
    CHECK(temperature::encodeCelsius(row.celsius, encoded).ok());
    CHECK(encoded == row.word);
  }
}

void rejectBeforeRounding() {
  constexpr double infinity = std::numeric_limits<double>::infinity();
  const double invalid[] = {
      std::nextafter(-256.0, -infinity), std::nextafter(255.9921875, infinity),
      -257.0, 256.0, std::numeric_limits<double>::quiet_NaN(), -infinity, infinity,
  };
  for (const double input : invalid) {
    std::uint16_t encoded = 0x5A5A;
    CHECK(temperature::encodeCelsius(input, encoded).is(Err::INVALID_PARAM));
    CHECK(encoded == 0x5A5A);
  }
}

void everyWordRoundTrips() {
  for (std::uint32_t word = 0; word <= 0xFFFFu; ++word) {
    const auto counts = temperature::decodeCounts(static_cast<std::uint16_t>(word));
    std::uint16_t encoded = 0;
    CHECK(temperature::encodeCelsius(temperature::toCelsius(counts), encoded).ok());
    CHECK(encoded == word);
  }
}

}  // namespace

int main() {
  signedRegisterVectors();
  nearestCountTiesAwayFromZero();
  rejectBeforeRounding();
  everyWordRoundTrips();
  return EXIT_SUCCESS;
}
