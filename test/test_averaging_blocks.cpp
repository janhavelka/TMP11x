#include "TMP11x/TMP11x.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>

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
  CHECK(device.setMode(Mode::SHUTDOWN).ok());
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
    resetAndTwoAveragingBlocks(model);
  }
  return EXIT_SUCCESS;
}
