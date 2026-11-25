#include "TMP11x/Timing.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {

using namespace TMP11x;

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition); \
      std::exit(EXIT_FAILURE);                                             \
    }                                                                      \
  } while (false)

void checkDuration(const ConversionDuration& value, const ConversionDuration& expected) {
  CHECK(value.minUs == expected.minUs);
  CHECK(value.typicalUs == expected.typicalUs);
  CHECK(value.maxUs == expected.maxUs);
}

void timingTable(Model model, const ConversionDuration (&blocks)[4]) {
  // Independent cycle-table cells in ms and standby from TMP116 Table 7 notes 1-5.
  constexpr double periodMs[8][4] = {
      {15.5, 125, 500, 1000}, {125, 125, 500, 1000}, {250, 250, 500, 1000}, {500, 500, 500, 1000},
      {1000, 1000, 1000, 1000}, {4000, 4000, 4000, 4000}, {8000, 8000, 8000, 8000}, {16000, 16000, 16000, 16000},
  };
  constexpr double standbyMs[8][4] = {
      {0, 0, 0, 0}, {109.5, 0, 0, 0}, {234.5, 125, 0, 0}, {484.5, 375, 0, 0},
      {984.5, 875, 500, 0}, {3984.5, 3875, 3500, 3000}, {7984.5, 7875, 7500, 7000}, {15984.5, 15875, 15500, 15000},
  };
  constexpr unsigned counts[] = {1, 8, 32, 64};
  for (unsigned cycle = 0; cycle < 8; ++cycle) {
    for (unsigned avg = 0; avg < 4; ++avg) {
      ConversionTiming value{};
      CHECK(getConversionTiming(model, static_cast<Cycle>(cycle), static_cast<Averaging>(avg), value).ok());
      CHECK(value.conversionsPerResult == counts[avg]);
      CHECK(value.nominalPeriodUs == periodMs[cycle][avg] * 1000);
      CHECK(value.nominalStandbyUs == standbyMs[cycle][avg] * 1000);
      CHECK(std::abs(value.nominalUpdateRateHz * periodMs[cycle][avg] - 1000.0) < 1e-9);
      checkDuration(value.singleConversion, blocks[0]);
      checkDuration(value.activeEstimate, blocks[avg]);
      CHECK(value.oneShotWindowUs == blocks[avg].maxUs + 10000u);
      // All published nominal cells with the 25% margin cover the active upper estimate.
      CHECK(value.continuousWindowUs == periodMs[cycle][avg] * 1250 + 10000);
    }
  }
}

void invalidTimingInputs() {
  const ConversionTiming saved{8, {1, 2, 3}, {4, 5, 6}, 7, 8, 9.0, 10, 11};
  auto rejected = [&](Model model, Cycle cycle, Averaging averaging) {
    auto result = saved;
    CHECK(getConversionTiming(model, cycle, averaging, result).is(Err::INVALID_PARAM));
    checkDuration(result.singleConversion, saved.singleConversion);
    checkDuration(result.activeEstimate, saved.activeEstimate);
    CHECK(result.conversionsPerResult == 8 && result.nominalPeriodUs == 7 && result.nominalStandbyUs == 8);
    CHECK(result.nominalUpdateRateHz == 9.0 && result.continuousWindowUs == 10 && result.oneShotWindowUs == 11);
  };
  rejected(Model::UNSELECTED, Cycle::CODE_0, Averaging::SAMPLES_1);
  rejected(static_cast<Model>(255), Cycle::CODE_0, Averaging::SAMPLES_1);
  rejected(Model::TMP117, static_cast<Cycle>(8), Averaging::SAMPLES_1);
  rejected(Model::TMP119, Cycle::CODE_0, static_cast<Averaging>(4));
}

}  // namespace

int main() {
  using namespace TMP11x;
  // Count-scaled electrical rows for complete blocks, in microseconds.
  constexpr ConversionDuration tmp116[] = {
      {13500, 15500, 17000}, {108000, 124000, 136000}, {432000, 496000, 544000}, {864000, 992000, 1088000},
  };
  constexpr ConversionDuration tmp117And119[] = {
      {13000, 15500, 17500}, {104000, 124000, 140000}, {416000, 496000, 560000}, {832000, 992000, 1120000},
  };
  for (auto model : {Model::TMP116, Model::TMP116N}) timingTable(model, tmp116);
  for (auto model : {Model::TMP117, Model::TMP117M, Model::TMP117N, Model::TMP119}) timingTable(model, tmp117And119);
  invalidTimingInputs();
}
