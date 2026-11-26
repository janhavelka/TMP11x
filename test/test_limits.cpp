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

struct Fixture {
  explicit Fixture(Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x4B;
    config.i2cUser = &bus;
    config.i2cWrite = FakeTmp11xBus::write;
    config.i2cWriteRead = FakeTmp11xBus::writeRead;
    config.timeUser = &tick;
    config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
    config.settings.limits = {0, 1280};  // [0, 10] degrees Celsius.
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0x0117;
    CHECK(bus.attach(config.i2cAddress, chip));
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t tick{};
};

void checkRead(const Fixture& f, std::size_t index, std::uint8_t pointer) {
  const auto& frame = f.bus.trace.at(index);
  CHECK(frame.address == f.config.i2cAddress && frame.combinedRead);
  CHECK(frame.tx.size() == 1 && frame.tx[0] == pointer && frame.rxLen == 2);
}

void checkWrite(const Fixture& f, std::size_t index, std::uint8_t pointer, std::uint16_t word) {
  const auto& frame = f.bus.trace.at(index);
  CHECK(frame.address == f.config.i2cAddress && !frame.combinedRead && frame.tx.size() == 3);
  CHECK(frame.tx[0] == pointer && frame.tx[1] == (word >> 8) && frame.tx[2] == (word & 0xFF));
}

void signedLimitsAndRouting() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117, Model::TMP117M,
                           Model::TMP117N, Model::TMP119}) {
    Fixture f{model};
    CHECK(f.device.begin(f.config).ok());
    auto generation = f.device.getSettingsSnapshot().verifiedGeneration;
    for (const bool routingChange : {false, true}) {
      f.bus.trace.clear();
      if (routingChange) {
        CHECK(f.device.setAlert(AlertMode::ALERT, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY).ok());
      } else {
        CHECK(f.device.setLimits({-1344, -832}).ok());  // -10.5 and -6.5 degrees Celsius.
      }
      const std::uint16_t cc = routingChange ? 0x022C : 0x0220;
      const bool hasOffset = model != Model::TMP116 && model != Model::TMP116N;
      CHECK(f.bus.trace.size() == (hasOffset ? 15u : 14u));
      checkRead(f, 0, 0x0F);
      checkRead(f, 1, 0x04);
      checkRead(f, 2, 0x01);
      checkRead(f, 3, 0x03);
      checkRead(f, 4, 0x02);
      checkWrite(f, 5, 0x01, static_cast<std::uint16_t>(cc | 0x0400));
      checkRead(f, 6, 0x01);
      checkWrite(f, 7, 0x03, 0xFAC0);
      checkRead(f, 8, 0x03);
      checkWrite(f, 9, 0x02, 0xFCC0);
      checkRead(f, 10, 0x02);
      if (hasOffset) checkRead(f, 11, 0x07);
      const auto baseline = hasOffset ? 12u : 11u;
      checkRead(f, baseline, 0x01);
      checkWrite(f, baseline + 1, 0x01, cc);
      checkRead(f, baseline + 2, 0x01);
      const auto settings = f.device.getSettingsSnapshot();
      ++generation;
      CHECK(settings.trust == ConfigTrust::VERIFIED && settings.verifiedGeneration == generation);
      CHECK(settings.config && settings.config->raw == cc);
      CHECK(settings.desired.mode == Mode::CONTINUOUS && settings.desired.alertMode == AlertMode::ALERT);
      CHECK(settings.desired.polarity == (routingChange ? Polarity::ACTIVE_HIGH : Polarity::ACTIVE_LOW));
      CHECK(settings.desired.pinFunction == (routingChange ? PinFunction::DATA_READY : PinFunction::THRESHOLD));

      f.chip.words[1] |= 0xA000;  // Neither limit read acknowledges HIGH or DATA_READY.
      f.bus.trace.clear();
      const auto health = f.device.healthSnapshot();
      Limits result;
      CHECK(f.device.readLimits(result).ok() && result.lowCounts == -1344 && result.highCounts == -832);
      CHECK(f.bus.trace.size() == 2 && f.chip.words[1] == (cc | 0xA000));
      checkRead(f, 0, 0x03);
      checkRead(f, 1, 0x02);
      CHECK(f.device.healthSnapshot().totalSuccess == health.totalSuccess + 2);
    }
  }
}

void admissionAndThermOrdering() {
  Fixture f;
  Limits result{7, 11};
  for (const auto refusal : {Err::NOT_BOUND, Err::NOT_INITIALIZED}) {
    CHECK(f.device.setLimits({1, 2}).is(refusal));
    CHECK(f.device.setAlert(AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY).is(refusal));
    CHECK(f.device.readLimits(result).is(refusal));
    CHECK(result.lowCounts == 7 && result.highCounts == 11 && f.tick == 0 && f.bus.trace.empty());
    if (refusal == Err::NOT_BOUND) CHECK(f.device.bind(f.config).ok());
  }
  CHECK(f.device.begin().ok());
  CHECK(f.device.setLimits({2560, 1280}).ok());  // Alert limits are independent.
  f.bus.trace.clear();
  const auto tick = f.tick;
  CHECK(f.device.setAlert(AlertMode::THERM, Polarity::ACTIVE_LOW, PinFunction::THRESHOLD).is(Err::INVALID_CONFIG));
  CHECK(f.device.setAlert(static_cast<AlertMode>(2), Polarity::ACTIVE_LOW, PinFunction::THRESHOLD).is(Err::INVALID_CONFIG));
  CHECK(f.device.setAlert(AlertMode::ALERT, static_cast<Polarity>(2), PinFunction::THRESHOLD).is(Err::INVALID_CONFIG));
  CHECK(f.device.setAlert(AlertMode::ALERT, Polarity::ACTIVE_LOW, static_cast<PinFunction>(2)).is(Err::INVALID_CONFIG));
  CHECK(f.tick == tick && f.bus.trace.empty() && f.device.desiredSettings().limits.lowCounts == 2560);
  CHECK(f.device.setLimits({1280, 1280}).ok());
  CHECK(f.device.setAlert(AlertMode::THERM, Polarity::ACTIVE_LOW, PinFunction::THRESHOLD).ok());
  f.bus.trace.clear();
  CHECK(f.device.setLimits({1281, 1280}).is(Err::INVALID_CONFIG) && f.bus.trace.empty());
  CHECK(f.device.setLimits({0, 0}).ok() && f.chip.words[1] == 0x0230);
}

void partialReadsAndDrift() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  const auto before = f.device.getSettingsSnapshot();
  f.chip.words[3] = 0x8000;
  f.chip.words[2] = 0x7FFF;
  Limits result{7, 11};
  for (const bool failSecond : {false, true}) {
    f.bus.trace.clear();
    if (failSecond) f.bus.script.push_back({});
    f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short limit", 17), 1});
    const auto status = f.device.readLimits(result);
    CHECK(status.is(Err::I2C_SHORT_TRANSFER) && status.detail == 17);
    CHECK(result.lowCounts == 7 && result.highCounts == 11);
    CHECK(f.bus.trace.size() == (failSecond ? 2u : 1u));
    checkRead(f, 0, 0x03);
    const auto partial = f.device.getSettingsSnapshot();
    CHECK(partial.low && partial.high && before.low && before.high);
    CHECK(partial.high->raw == 0x0500 && partial.high->observedAtMs == before.high->observedAtMs);
    if (failSecond) {
      checkRead(f, 1, 0x02);
      CHECK(partial.low->raw == 0x8000 && partial.low->counts == -32768);
      CHECK(partial.low->observedAtMs == f.device.healthSnapshot().lastSuccessMs);
      CHECK(partial.affectedFields == static_cast<unsigned>(SettingsField::LOW_LIMIT));
      CHECK(partial.trust == ConfigTrust::DIRTY);
    } else {
      CHECK(partial.low->observedAtMs == before.low->observedAtMs && partial.trust == ConfigTrust::VERIFIED);
    }
  }
  CHECK(f.device.readLimits(result).ok() && result.lowCounts == -32768 && result.highCounts == 32767);
  const auto drift = f.device.getSettingsSnapshot();
  CHECK(drift.affectedFields == (static_cast<unsigned>(SettingsField::LOW_LIMIT) |
                                 static_cast<unsigned>(SettingsField::HIGH_LIMIT)));
  CHECK(drift.trust == ConfigTrust::DIRTY && drift.dirtyReason == DirtyReason::READBACK_MISMATCH);
  CHECK(drift.verifiedAtMs == before.verifiedAtMs && drift.verifiedGeneration == before.verifiedGeneration);
}

void invertedStartingPairRequiresRecovery() {
  Fixture f;
  f.config.settings.alertMode = AlertMode::THERM;
  CHECK(f.device.begin(f.config).ok());
  f.chip.words[3] = 0x0A00;  // Unannounced drift: [20, 10] while therm is active.
  f.bus.trace.clear();
  CHECK(f.device.setLimits({1920, 2560}).is(Err::CONFIG_MISMATCH));
  CHECK(f.bus.trace.size() == 4);
  for (const auto& frame : f.bus.trace) CHECK(frame.combinedRead);
  f.bus.trace.clear();
  CHECK(f.device.setLimits({1920, 2560}).is(Err::CONFIG_DIRTY));
  CHECK(f.device.setAlert(AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::THRESHOLD).is(Err::CONFIG_DIRTY));
  CHECK(f.bus.trace.empty() && f.device.desiredSettings().limits.lowCounts == 0);
  CHECK(f.device.recover().ok() && f.bus.trace.size() == 16);
  checkWrite(f, 5, 0x01, 0x0620);  // Shutdown, temporarily using alert comparison.
  checkRead(f, 6, 0x01);
  checkWrite(f, 7, 0x03, 0x0000);
  checkRead(f, 8, 0x03);
  checkWrite(f, 9, 0x02, 0x0500);
  checkRead(f, 10, 0x02);
  checkRead(f, 11, 0x07);
  checkWrite(f, 12, 0x01, 0x0630);  // Enable hysteresis after both verified limits.
  checkRead(f, 13, 0x01);
  checkWrite(f, 14, 0x01, 0x0230);
  checkRead(f, 15, 0x01);
  CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);
}

void interruptedThermPair() {
  Fixture f;
  f.config.settings.alertMode = AlertMode::THERM;
  CHECK(f.device.begin(f.config).ok());
  const auto generation = f.device.getSettingsSnapshot().verifiedGeneration;
  f.bus.trace.clear();
  f.bus.script.resize(9);
  TransferOutcome lostAck;
  lostAck.status = Status::Error(Err::I2C_TIMEOUT, "second limit write timed out");
  lostAck.failedWriteWord = 0x0700;
  f.bus.script.push_back(lostAck);
  CHECK(f.device.setLimits({1920, 2560}).is(Err::I2C_TIMEOUT));
  CHECK(f.bus.trace.size() == 10 && f.chip.words[1] == 0x0630);
  checkWrite(f, 7, 0x02, 0x0A00);  // Verified THIGH before raising TLOW past the old THIGH.
  checkRead(f, 8, 0x02);
  checkWrite(f, 9, 0x03, 0x0780);
  CHECK(f.chip.words[2] == 0x0A00 && f.chip.words[3] == 0x0700);
  const auto dirty = f.device.getSettingsSnapshot();
  CHECK(dirty.trust == ConfigTrust::DIRTY && dirty.dirtyReason == DirtyReason::AMBIGUOUS_WRITE);
  CHECK(dirty.verifiedGeneration == generation && !dirty.low && dirty.high && dirty.high->raw == 0x0A00);
  CHECK(dirty.desired.limits.lowCounts == 1920 && dirty.desired.limits.highCounts == 2560);
  CHECK(f.device.recover().ok() && f.chip.words[3] == 0x0780);
  f.bus.trace.clear();
  CHECK(f.device.setLimits({0, 1280}).ok());
  checkWrite(f, 7, 0x03, 0x0000);  // Lower TLOW and verify it before lowering THIGH.
  checkRead(f, 8, 0x03);
  checkWrite(f, 9, 0x02, 0x0500);
  checkRead(f, 10, 0x02);
  CHECK(f.device.getSettingsSnapshot().verifiedGeneration == generation + 2);
}

}  // namespace

int main() {
  signedLimitsAndRouting();
  admissionAndThermOrdering();
  partialReadsAndDrift();
  invertedStartingPairRequiresRecovery();
  interruptedThermPair();
  return EXIT_SUCCESS;
}
