/// @file Timing.h
/// @brief Bus-silent estimates for continuous conversions and individual one-shots.
#pragma once

#include "TMP11x/Config.h"

namespace TMP11x {

/// Durations in microseconds. The enclosing field identifies their source.
struct ConversionDuration {
  std::uint32_t minUs{};      ///< Lower value under the selected part's electrical conditions.
  std::uint32_t typicalUs{};  ///< Typical value at 25 degrees Celsius and 3.3 V.
  std::uint32_t maxUs{};      ///< Upper value under the same conditions.
};

/// Timing for the selected CONV and AVG codes. Shutdown has no periodic updates.
struct ConversionTiming {
  std::uint8_t conversionsPerResult{};  ///< Number of successive conversions in one complete block.
  ConversionDuration singleConversion;  ///< Published electrical row for the selected model.
  ConversionDuration activeEstimate;    ///< Count-scaled block durations assuming no extra overhead.
  std::uint32_t nominalPeriodUs{};       ///< TI continuous cycle-table period including standby.
  std::uint32_t nominalStandbyUs{};      ///< Continuous standby from the table's rounded active budget.
  double nominalUpdateRateHz{};          ///< Estimated complete results per second in continuous mode.
  std::uint32_t continuousWindowUs{};    ///< Library conversion budget before tick rounding, excluding transfers.
  std::uint32_t oneShotWindowUs{};       ///< Active upper estimate plus 10 ms of library slack.
};

/// Report timing for an exact BOM model without accessing a device. TMP116/N
/// specifies one conversion, TMP117/M/N specifies one-shot, and TMP119 specifies
/// one conversion without averaging. Continuous use of the TMP117-family row
/// is an extrapolation. TI specifies no maximum continuous period. For a
/// one-shot use activeEstimate at any CONV code, with zero standby.
/// The continuous window uses the larger of 125% of the nominal period and
/// the active upper estimate, plus 10 ms. Round each window up to the owner's
/// tick and budget setup and transfers separately. Elapsed time cannot prove
/// completion. INVALID_PARAM preserves @p out for an unsupported model or code.
Status getConversionTiming(Model model, Cycle cycle, Averaging averaging, ConversionTiming& out) noexcept;

}  // namespace TMP11x
