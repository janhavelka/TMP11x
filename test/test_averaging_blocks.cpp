#include "TMP11x/TMP11x.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);     \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

void allBlockLengths(Model model) {
  // CONFIG AVG bits and the number of inputs needed for one result.
  constexpr struct { std::uint16_t bits; int length; } blocks[] = {
      {0x0000, 1}, {0x0020, 8}, {0x0040, 32}, {0x0060, 64},
  };
  for (const auto& block : blocks) {
    FakeTmp11xDevice chip{model};
    chip.words[0x00] = 0x8000;
    chip.words[0x01] = block.bits;
    for (int i = 1; i < block.length; ++i) {
      CHECK(chip.addContinuousConversion(1));
      CHECK(chip.words[0x00] == 0x8000 && (chip.words[0x01] & 0x2000u) == 0);
    }
    CHECK(chip.addContinuousConversion(1));
    CHECK(chip.words[0x00] == 1 && (chip.words[0x01] & 0x2000u) != 0);
  }
}

void oneShotBlocks(Model model) {
  constexpr unsigned lengths[] = {1, 8, 32, 64};
  for (unsigned avg = 0; avg < 4; ++avg) {
    for (unsigned cycle = 0; cycle < 8; ++cycle) {
      for (bool reportShutdown : {false, true}) {
        FakeTmp11xDevice chip{model};
        FakeTmp11xBus bus;
        CHECK(bus.attach(0x48, chip));
        chip.reportShutdownForOneShot = reportShutdown;
        chip.words[0] = 0x8000;
        chip.words[2] = 0x7FFF;
        chip.words[3] = 0x8000;
        const auto fields = static_cast<std::uint16_t>((cycle << 7) | (avg << 5));
        const auto writeConfig = [&](std::uint16_t word) {
          const std::uint8_t tx[] = {1, static_cast<std::uint8_t>(word >> 8),
                                     static_cast<std::uint8_t>(word)};
          CHECK(bus.write(0x48, tx, 3, 1, &bus).ok());
        };
        const auto readConfig = [&]() {
          const std::uint8_t pointer = 1;
          std::uint8_t rx[2]{};
          CHECK(bus.writeRead(0x48, &pointer, 1, rx, 2, 1, &bus).ok());
          return (rx[0] << 8) | rx[1];
        };
        writeConfig(0x0400 | fields);
        CHECK(!chip.addOneShotConversion(-8));
        writeConfig(0x0C00 | fields);
        const auto observed = (reportShutdown ? 0x0400 : 0x0C00) | fields;
        for (unsigned i = 1; i < lengths[avg]; ++i) {
          CHECK(chip.addOneShotConversion(-8));
          CHECK(chip.words[0] == 0x8000 && readConfig() == observed);
        }
        CHECK(chip.addOneShotConversion(-8));
        CHECK(readConfig() == (0x2000 | observed));
        CHECK(!chip.addOneShotConversion(32) && !chip.addContinuousConversion(32));
        CHECK(chip.words[0] == 0xFFF8 && readConfig() == observed);
        // A MOD readback of 01 cannot arm the simulator's next shot. Repeat
        // triggers without an explicit shutdown have no modeled silicon result.
        writeConfig(0x0C00 | fields);
        CHECK(!chip.addOneShotConversion(32));

        // Abort immediately before the last constituent and change the cycle.
        // All following constituents belong to the new block.
        writeConfig(0x0400 | fields);
        writeConfig(0x0C00 | fields);
        for (unsigned i = 1; i < lengths[avg]; ++i) CHECK(chip.addOneShotConversion(32));
        writeConfig(0x0400 | fields);
        CHECK(!chip.addOneShotConversion(32) && chip.words[0] == 0xFFF8);
        CHECK(readConfig() == (0x0400 | fields));
        const auto newFields = fields ^ 0x0080;
        writeConfig(0x0400 | newFields);
        writeConfig(0x0C00 | newFields);
        for (unsigned i = 1; i < lengths[avg]; ++i) {
          CHECK(chip.addOneShotConversion(8));
          CHECK(chip.words[0] == 0xFFF8 && (readConfig() & 0x2000) == 0);
        }
        CHECK(chip.addOneShotConversion(8));
        CHECK(chip.words[0] == 8 && (readConfig() & 0x2000) != 0);
      }
    }
  }
}

void resetAndTwoAveragingBlocks(Model model) {
  FakeTmp11xDevice chip{model};
  FakeTmp11xBus bus;
  Device device;
  Config config;
  std::uint32_t tick = 0;
  config.model = model;
  config.i2cAddress = 0x48;
  config.i2cUser = &bus;
  config.i2cWrite = FakeTmp11xBus::write;
  config.i2cWriteRead = FakeTmp11xBus::writeRead;
  config.nowMs = [](void* p) { return (*static_cast<std::uint32_t*>(p))++; };
  config.timeUser = &tick;
  config.settings.averaging = Averaging::SAMPLES_8;
  config.settings.limits = {-1, 8};
  chip.words[0x00] = 0x8000;
  chip.words[0x0F] = (model == Model::TMP116 || model == Model::TMP116N) ? 0x1116 : 0x0117;
  CHECK(bus.attach(config.i2cAddress, chip));
  CHECK(device.begin(config).ok());

  const auto checkLatest = [&](std::uint16_t raw) {
    Sample sample;
    CHECK(device.readLatest(sample).ok());
    CHECK(sample.raw == raw && !sample.freshConversion);
  };

  // Stage 6 reset-and-two-averaging-blocks. Several constituent counts
  // exceed THIGH=8. The first completed mean of 7 remains inside the limits.
  checkLatest(0x8000);
  for (const std::int16_t counts : {0, 2, 4, 6, 8, 10, 12}) {
    CHECK(chip.addContinuousConversion(counts));
    CHECK(chip.words[0x00] == 0x8000 && (chip.words[0x01] & 0xE000u) == 0);
  }
  CHECK(chip.addContinuousConversion(14));
  CHECK(chip.words[0x00] == 0x0007);
  ConfigurationObservation observed;
  CHECK(device.readConfiguration(observed).ok());
  CHECK(observed.dataReady && !observed.high && !observed.low);
  CHECK((chip.words[0x01] & 0xE000u) == 0);

  for (const std::int16_t counts : {20, 22, 24, 26, 28, 30, 32}) {
    CHECK(chip.addContinuousConversion(counts));
    CHECK(chip.words[0x00] == 0x0007 && (chip.words[0x01] & 0xE000u) == 0);
  }
  CHECK(chip.addContinuousConversion(34));
  CHECK((chip.words[0x01] & 0xE000u) == 0xA000u);  // HIGH and READY.
  for (int read = 0; read < 2; ++read) {
    checkLatest(0x001B);
    CHECK((chip.words[0x01] & 0xE000u) == 0x8000u);  // TEMP consumes only READY.
  }
  CHECK(device.readConfiguration(observed).ok());
  CHECK(observed.high && !observed.low && !observed.dataReady);

  for (int i = 0; i < 3; ++i) {
    CHECK(chip.addContinuousConversion(120));
    CHECK(chip.words[0x00] == 0x001B && (chip.words[0x01] & 0xE000u) == 0);
  }
  bus.trace.clear();
  CHECK(device.setMode(Mode::SHUTDOWN).ok());
  // The first CONFIG read after SD would acknowledge any spurious READY from the abort.
  CHECK(bus.trace.size() >= 7 && bus.trace[5].tx == std::vector<std::uint8_t>({0x01, 0x06, 0x20}));
  CHECK(bus.trace[6].combinedRead && bus.trace[6].tx == std::vector<std::uint8_t>({0x01}));
  CHECK(bus.trace[6].rx == std::vector<std::uint8_t>({0x06, 0x20}));
  checkLatest(0x001B);
  // A full new block after resuming exposes any carry-over from the aborted three inputs.
  CHECK(device.setMode(Mode::CONTINUOUS).ok());
  for (int i = 0; i < 7; ++i) {
    CHECK(chip.addContinuousConversion(-8));
    CHECK(chip.words[0x00] == 0x001B && (chip.words[0x01] & 0xE000u) == 0);
  }
  CHECK(chip.addContinuousConversion(-8));
  CHECK(device.readConfiguration(observed).ok());
  CHECK(observed.dataReady && observed.low && !observed.high);
  checkLatest(0xFFF8);
}

}  // namespace

int main() {
  // In the second case, block-end comparison is inferred for TMP116/N.
  // TMP117/M/N and TMP119 explicitly include averaging in the comparison.
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    allBlockLengths(model);
    oneShotBlocks(model);
    resetAndTwoAveragingBlocks(model);
  }
  return EXIT_SUCCESS;
}
