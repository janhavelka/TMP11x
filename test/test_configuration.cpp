#include "TMP11x/Configuration.h"

#include <cstdint>
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

// Independent field encodings from the TMP116, TMP117 and TMP119 CONFIG tables.
void allSteadySettings(Model model) {
  constexpr struct {
    Cycle cycle;
    std::uint16_t bits;
  } cycles[] = {
      {Cycle::CODE_0, 0x0000}, {Cycle::CODE_1, 0x0080}, {Cycle::CODE_2, 0x0100},
      {Cycle::CODE_3, 0x0180}, {Cycle::CODE_4, 0x0200}, {Cycle::CODE_5, 0x0280},
      {Cycle::CODE_6, 0x0300}, {Cycle::CODE_7, 0x0380},
  };
  constexpr struct {
    Averaging averaging;
    std::uint16_t bits;
  } averages[] = {
      {Averaging::SAMPLES_1, 0x0000}, {Averaging::SAMPLES_8, 0x0020},
      {Averaging::SAMPLES_32, 0x0040}, {Averaging::SAMPLES_64, 0x0060},
  };
  constexpr struct {
    AlertMode alertMode;
    Polarity polarity;
    PinFunction pinFunction;
    std::uint16_t bits;
  } routes[] = {
      {AlertMode::ALERT, Polarity::ACTIVE_LOW,  PinFunction::THRESHOLD,  0x0000},
      {AlertMode::ALERT, Polarity::ACTIVE_LOW,  PinFunction::DATA_READY, 0x0004},
      {AlertMode::ALERT, Polarity::ACTIVE_HIGH, PinFunction::THRESHOLD,  0x0008},
      {AlertMode::ALERT, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY, 0x000C},
      {AlertMode::THERM, Polarity::ACTIVE_LOW,  PinFunction::THRESHOLD,  0x0010},
      {AlertMode::THERM, Polarity::ACTIVE_LOW,  PinFunction::DATA_READY, 0x0014},
      {AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::THRESHOLD,  0x0018},
      {AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY, 0x001C},
  };
  for (const auto mode : {Mode::CONTINUOUS, Mode::SHUTDOWN}) {
    const bool shutdown = mode == Mode::SHUTDOWN;
    for (const auto& cycle : cycles) {
      for (const auto& avg : averages) {
        for (const auto& route : routes) {
          const SensorSettings desired{mode, cycle.cycle, avg.averaging, route.alertMode,
                                       route.polarity, route.pinFunction, {}};
          const auto expected = static_cast<std::uint16_t>(
              (shutdown ? 0x0400 : 0) | cycle.bits | avg.bits | route.bits);
          std::uint16_t encoded = 0xFFFF;
          CHECK(validateSettings(model, desired).ok());
          CHECK(encodeConfiguration(model, desired, encoded).ok());
          CHECK(encoded == expected);
          CHECK((encoded & 0xF803) == 0);

          const auto observed = decodeConfiguration(expected);
          CHECK(observed.raw == expected);
          CHECK(observed.rawMode() == (shutdown ? 1 : 0));
          CHECK(observed.mode == (shutdown ? ObservedMode::SHUTDOWN : ObservedMode::CONTINUOUS));
          CHECK(observed.cycle == cycle.cycle && observed.averaging == avg.averaging);
          CHECK(observed.alertMode == route.alertMode && observed.polarity == route.polarity);
          CHECK(observed.pinFunction == route.pinFunction);
          CHECK(compareExpectedRegister(model, desired, 0x01, expected).ok());
        }
      }
    }
  }
}

void statusAndReadEffects(Model model) {
  constexpr struct {
    std::uint16_t flags;
    bool high, low, ready, busy;
  } rows[] = {
      {0x0000, false, false, false, false}, {0x8000, true, false, false, false},
      {0x4000, false, true, false, false},  {0x2000, false, false, true, false},
      {0x1000, false, false, false, true},  {0xB000, true, false, true, true},
  };
  for (const auto alertMode : {AlertMode::ALERT, AlertMode::THERM}) {
    SensorSettings desired{};
    desired.mode = Mode::SHUTDOWN;
    desired.alertMode = alertMode;
    const bool alert = alertMode == AlertMode::ALERT;
    const std::uint16_t settings = alert ? 0x0620 : 0x0630;
    for (const auto& row : rows) {
      // Include a synthetic therm LOW bit to verify raw flag retention separately
      // from its meaning. A real therm reading reports LOW zero.
      const auto word = static_cast<std::uint16_t>(settings | row.flags);
      const auto observed = decodeConfiguration(word);
      CHECK(observed.raw == word);
      CHECK(!observed.timestampValid);
      CHECK(observed.high == row.high && observed.low == row.low);
      CHECK(observed.dataReady == row.ready && observed.eepromBusy == row.busy);
      CHECK(observed.readClearMask == (alert ? 0xE000 : 0x2000));
      CHECK(compareExpectedRegister(model, desired, 0x01, word).ok());
    }
  }
}

void nonfactoryResetProfile(Model model) {
  // Following POR, programmed MOD0 and bits 9:2 may differ from the factory targets.
  const SensorSettings programmed{Mode::SHUTDOWN, Cycle::CODE_3, Averaging::SAMPLES_32,
                                  AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY, {}};
  constexpr std::uint16_t resetReadback = 0x05DC;
  CHECK(compareExpectedRegister(model, SensorSettings{}, 0x01, resetReadback).is(Err::CONFIG_MISMATCH));
  CHECK(compareExpectedRegister(model, programmed, 0x01, resetReadback).ok());
  // Hypothetical reads with either reserved or reset-command bits set retain
  // the raw evidence. The subsequent ordinary write must drop both low bits.
  constexpr std::uint16_t syntheticWords[] = {0x05DD, 0x05DE, 0x05DF, 0xB5DF};
  for (const auto raw : syntheticWords) {
    CHECK(decodeConfiguration(raw).raw == raw);
    CHECK(compareExpectedRegister(model, programmed, 0x01, raw).ok());
    auto encoded = raw;
    CHECK(encodeConfiguration(model, programmed, encoded).ok());
    CHECK(encoded == resetReadback);
  }
}

void normalizedModeComparison(Model model) {
  constexpr struct {
    std::uint16_t raw;
    std::uint8_t rawMode;
    ObservedMode mode;
    bool continuousMatches, shutdownMatches, ownedShutdownMatches;
  } cases[] = {
      {0x0220, 0, ObservedMode::CONTINUOUS, true,  false, false},
      {0x0620, 1, ObservedMode::SHUTDOWN,   false, true,  true},
      {0x0A20, 2, ObservedMode::CONTINUOUS, true,  false, false},
      {0x0E20, 3, ObservedMode::ONE_SHOT,   false, false, true},
  };
  SensorSettings continuous{};
  auto shutdown = continuous;
  shutdown.mode = Mode::SHUTDOWN;
  for (const auto& row : cases) {
    const auto observed = decodeConfiguration(row.raw);
    CHECK(observed.rawMode() == row.rawMode && observed.mode == row.mode);
    for (const auto evidence : {OwnedTrigger::NONE, OwnedTrigger::CONFIRMED}) {
      const bool shutdownMatches = evidence == OwnedTrigger::CONFIRMED
                                       ? row.ownedShutdownMatches : row.shutdownMatches;
      for (unsigned flags = 0; flags < 16; ++flags) {
        const auto word = static_cast<std::uint16_t>((flags << 12) | row.raw);
        const auto cont = compareExpectedRegister(model, continuous, 0x01, word, evidence);
        const auto sd = compareExpectedRegister(model, shutdown, 0x01, word, evidence);
        CHECK(cont.is(row.continuousMatches ? Err::OK : Err::CONFIG_MISMATCH));
        CHECK(sd.is(shutdownMatches ? Err::OK : Err::CONFIG_MISMATCH));
      }
      // Flip each non-mode field bit. Even a confirmed trigger only excuses MOD.
      for (unsigned bit = 2; bit <= 9; ++bit) {
        const auto changed = static_cast<std::uint16_t>(row.raw ^ (1u << bit));
        CHECK(compareExpectedRegister(model, continuous, 0x01, changed, evidence).is(Err::CONFIG_MISMATCH));
        CHECK(compareExpectedRegister(model, shutdown, 0x01, changed, evidence).is(Err::CONFIG_MISMATCH));
      }
    }
  }
  CHECK(compareExpectedRegister(model, shutdown, 0x01, 0xB620).ok());
  CHECK(compareExpectedRegister(model, shutdown, 0x01, 0x0640).is(Err::CONFIG_MISMATCH));
  CHECK(compareExpectedRegister(model, shutdown, 0x01, 0x0E20,
                                static_cast<OwnedTrigger>(2)).is(Err::INVALID_PARAM));
}

void expectInvalid(Model model, const SensorSettings& settings) {
  std::uint16_t word = 0xA55A;
  CHECK(validateSettings(model, settings).is(Err::INVALID_CONFIG));
  CHECK(encodeConfiguration(model, settings, word).is(Err::INVALID_CONFIG));
  CHECK(word == 0xA55A);
  constexpr std::uint8_t ownedPointers[] = {0x01, 0x02, 0x03};
  for (const auto pointer : ownedPointers) {
    CHECK(compareExpectedRegister(model, settings, pointer, 0).is(Err::INVALID_CONFIG));
  }
}

template <typename Field>
void rejectSelector(Model model, Field SensorSettings::* member, unsigned firstInvalid) {
  SensorSettings settings{};
  for (unsigned code = firstInvalid; code <= 0xFF; ++code) {
    settings.*member = static_cast<Field>(code);
    expectInvalid(model, settings);
  }
}

void signedLimitComparisons(Model model) {
  SensorSettings settings{};
  settings.alertMode = AlertMode::THERM;
  settings.limits = {-32768, 32767};
  CHECK(compareExpectedRegister(model, settings, 0x03, 0x8000).ok());
  CHECK(compareExpectedRegister(model, settings, 0x02, 0x7FFF).ok());
  settings.limits = {-832, -1};
  CHECK(compareExpectedRegister(model, settings, 0x03, 0xFCC0).ok());
  CHECK(compareExpectedRegister(model, settings, 0x02, 0xFFFF).ok());
  for (unsigned bit = 0; bit < 16; ++bit) {
    const auto changedLow = static_cast<std::uint16_t>(0xFCC0u ^ (1u << bit));
    const auto changedHigh = static_cast<std::uint16_t>(0xFFFFu ^ (1u << bit));
    CHECK(compareExpectedRegister(model, settings, 0x03, changedLow).is(Err::CONFIG_MISMATCH));
    CHECK(compareExpectedRegister(model, settings, 0x02, changedHigh).is(Err::CONFIG_MISMATCH));
  }
  settings.limits = {-1, -1};
  CHECK(validateSettings(model, settings).ok());
  settings.limits = {32767, -32768};
  settings.alertMode = AlertMode::ALERT;
  CHECK(compareExpectedRegister(model, settings, 0x02, 0x8000).ok());
  CHECK(compareExpectedRegister(model, settings, 0x03, 0x7FFF).ok());
  settings.alertMode = AlertMode::THERM;
  expectInvalid(model, settings);
}

}  // namespace

int main() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    allSteadySettings(model);
    statusAndReadEffects(model);
    nonfactoryResetProfile(model);
    normalizedModeComparison(model);
    signedLimitComparisons(model);
    rejectSelector(model, &SensorSettings::mode, 2);
    rejectSelector(model, &SensorSettings::cycle, 8);
    rejectSelector(model, &SensorSettings::averaging, 4);
    rejectSelector(model, &SensorSettings::alertMode, 2);
    rejectSelector(model, &SensorSettings::polarity, 2);
    rejectSelector(model, &SensorSettings::pinFunction, 2);
    constexpr std::uint8_t otherPointers[] = {0x00, 0x04, 0x07, 0x0F, 0xFF};
    for (const auto pointer : otherPointers) {
      CHECK(compareExpectedRegister(model, SensorSettings{}, pointer, 0).is(Err::INVALID_PARAM));
    }
  }
  expectInvalid(Model::UNSELECTED, SensorSettings{});
  expectInvalid(static_cast<Model>(0xFF), SensorSettings{});
  return EXIT_SUCCESS;
}
