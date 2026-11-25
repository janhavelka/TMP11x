#include "TMP11x/Timing.h"

#include <algorithm>

namespace TMP11x {

Status getConversionTiming(Model model, Cycle cycle, Averaging averaging, ConversionTiming& out) noexcept {
  if (!isValidModel(model) || !isValidCycle(cycle) || !isValidAveraging(averaging)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid timing model, cycle or averaging");
  }

  // SBOS740A Table 7, SNOSD82D Table 7-7, SNIS236 Table 8-6.
  constexpr std::uint32_t cycleUs[] = {15500, 125000, 250000, 500000, 1000000, 4000000, 8000000, 16000000};
  constexpr std::uint32_t nominalActiveUs[] = {15500, 125000, 500000, 1000000};
  constexpr std::uint8_t conversions[] = {1, 8, 32, 64};
  const auto avg = static_cast<unsigned>(averaging);
  const auto count = conversions[avg];
  const auto periodUs = std::max(cycleUs[static_cast<unsigned>(cycle)], nominalActiveUs[avg]);

  // Electrical characteristics, one-based PDF pages 5 / 6 / 7 respectively.
  const bool tmp116 = model == Model::TMP116 || model == Model::TMP116N;
  const ConversionDuration single = tmp116 ? ConversionDuration{13500, 15500, 17000}
                                           : ConversionDuration{13000, 15500, 17500};
  const ConversionDuration active{single.minUs * count, single.typicalUs * count, single.maxUs * count};
  out.conversionsPerResult = count;
  out.singleConversion = single;
  out.activeEstimate = active;
  out.nominalPeriodUs = periodUs;
  out.nominalStandbyUs = periodUs - nominalActiveUs[avg];
  out.nominalUpdateRateHz = 1000000.0 / periodUs;
  constexpr std::uint32_t slackUs = 10000;
  out.continuousWindowUs = std::max(periodUs + periodUs / 4, active.maxUs) + slackUs;
  out.oneShotWindowUs = active.maxUs + slackUs;
  return Status::Ok();
}

}  // namespace TMP11x
