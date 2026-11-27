#include "TMP11x/TMP11x.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);      \
      std::exit(EXIT_FAILURE);                                                  \
    }                                                                           \
  } while (false)

constexpr Model models[] = {Model::TMP116, Model::TMP116N, Model::TMP117,
                            Model::TMP117M, Model::TMP117N, Model::TMP119};

struct Fixture {
  explicit Fixture(Model model) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x4A;
    config.i2cTimeoutMs = 17;
    config.i2cWrite = FakeTmp11xBus::write;
    config.i2cWriteRead = FakeTmp11xBus::writeRead;
    config.i2cUser = &bus;
    CHECK(bus.attach(config.i2cAddress, chip));
    const bool fourWords = model == Model::TMP116 || model == Model::TMP116N;
    chip.words[0x0F] = fourWords ? 0x1116 : 0xD117;
    if (fourWords) pointers.push_back(0x07);
    pointers.push_back(0x08);
    chip.words[0x05] = 0x12AB;
    chip.words[0x06] = 0x805C;
    chip.words[0x07] = 0x7A13;
    chip.words[0x08] = 0xFE01;
  }

  void checkFrame(std::size_t index, std::uint8_t pointer, std::size_t received = 2) const {
    const auto& frame = bus.trace.at(index);
    CHECK(frame.combinedRead && frame.address == 0x4A && frame.timeoutMs == 17);
    CHECK(frame.txLen == 1 && frame.tx.size() == 1 && frame.tx[0] == pointer);
    CHECK(frame.rxLen == 2 && frame.rx.size() == received);
    const auto raw = chip.words.at(pointer);
    if (received > 0) CHECK(frame.rx[0] == (raw >> 8));
    if (received == 2) CHECK(frame.rx[1] == (raw & 0xFF));
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::vector<std::uint8_t> pointers{0x0F, 0x04, 0x05, 0x06};
};

void checkNeutral(const Device& device, const HealthSnapshot& before) {
  const auto after = device.healthSnapshot();
  CHECK(after.state == before.state && after.initialized == before.initialized);
  CHECK(after.totalSuccess == before.totalSuccess && after.totalFailures == before.totalFailures);
  CHECK(after.consecutiveFailures == before.consecutiveFailures);
  CHECK(after.lastSuccessMsValid == before.lastSuccessMsValid && after.lastSuccessMs == before.lastSuccessMs);
  CHECK(after.lastErrorMsValid == before.lastErrorMsValid && after.lastErrorMs == before.lastErrorMs);
  CHECK(after.lastError.code == before.lastError.code && after.lastError.detail == before.lastError.detail);
}

void scalarBoundsAndFrames() {
  for (const auto model : models) {
    Fixture f{model};
    std::uint16_t result = 0xABCD;
    CHECK(f.device.readEepromWord(1, result).is(Err::NOT_BOUND) && result == 0xABCD);
    CHECK(f.device.bind(f.config).ok());
    const auto count = static_cast<std::uint8_t>(f.pointers.size() - 2);
    for (const std::uint8_t index : {0, count + 1, 255}) {
      CHECK(f.device.readEepromWord(index, result).is(Err::INVALID_PARAM));
      CHECK(result == 0xABCD && f.bus.trace.empty());
    }
    const auto health = f.device.healthSnapshot();
    for (std::uint8_t index = 1; index <= count; ++index) {
      const auto pointer = f.pointers.at(index + 1);
      CHECK(f.device.readEepromWord(index, result).ok() && result == f.chip.words.at(pointer));
      CHECK(f.bus.trace.size() == index);
      f.checkFrame(index - 1, pointer);
    }
    f.chip.words[0x05] = 0;
    CHECK(f.device.readEepromWord(1, result).ok() && result == 0);
    f.checkFrame(count, 0x05);
    f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short shadow", 73), 1});
    result = 0xFACE;
    const auto shortRead = f.device.readEepromWord(2, result);
    CHECK(shortRead.is(Err::I2C_SHORT_TRANSFER) && shortRead.detail == 73 && result == 0xFACE);
    CHECK(f.bus.trace.size() == static_cast<std::size_t>(count) + 2);
    f.checkFrame(count + 1, 0x06, 1);
    CHECK(!f.device.identityInfo().rawValid);
    checkNeutral(f.device, health);
  }
}

// Only complete callbacks supply observation times. Trace length distinguishes
// clock samples taken before and after each read, including tick zero after ID.
std::uint32_t completionTick(void* user) {
  return static_cast<std::uint32_t>(static_cast<FakeTmp11xBus*>(user)->trace.size()) - 1u;
}

void checkPrefix(const Fixture& f, const EepromSnapshot& snap, std::size_t completed, bool timed) {
  CHECK(snap.identity.model == f.config.model && snap.identity.i2cAddress == f.config.i2cAddress);
  CHECK(snap.identity.rawValid == (completed > 0));
  if (completed > 0) {
    CHECK(snap.identity.raw == f.chip.words[0x0F] && snap.identity.compatible);
  }
  CHECK(snap.identityAtMs.has_value() == (completed > 0 && timed));
  if (snap.identityAtMs) CHECK(*snap.identityAtMs == 0);
  CHECK(snap.lock.has_value() == (completed > 1));
  if (snap.lock) {
    const auto raw = f.chip.words[0x04];
    CHECK(snap.lock->raw == raw && snap.lock->unlocked() == ((raw & 0x8000) != 0));
    CHECK(snap.lock->busy() == ((raw & 0x4000) != 0));
    CHECK(snap.lock->observedAtMs.has_value() == timed);
    if (timed) CHECK(snap.lock->observedAtMs == 1);
  }
  CHECK(snap.wordCount == (completed > 2 ? completed - 2 : 0));
  for (std::size_t i = 0; i < snap.words.size(); ++i) {
    const auto& word = snap.words[i];
    if (i < snap.wordCount) {
      CHECK(word.pointer == f.pointers.at(i + 2) && word.raw == f.chip.words.at(word.pointer));
      CHECK(word.observedAtMs.has_value() == timed);
      if (timed) CHECK(word.observedAtMs == i + 2);
    } else {
      CHECK(word.pointer == 0 && word.raw == 0 && !word.observedAtMs);
    }
  }
}

void snapshotsAndPartialResults() {
  for (const auto model : models) {
    for (const bool timed : {false, true}) {
      Fixture f{model};
      if (timed) {
        f.config.nowMs = completionTick;
        f.config.timeUser = &f.bus;
      }
      EepromSnapshot snap;
      snap.identity.raw = 0xF00D;
      snap.wordCount = 1;
      snap.words[0].raw = 0xCAFE;
      snap.failedPointer = 0x08;
      CHECK(f.device.readEepromSnapshot(snap).is(Err::NOT_BOUND));
      CHECK(snap.identity.raw == 0xF00D && snap.wordCount == 1 && snap.words[0].raw == 0xCAFE);
      CHECK(snap.failedPointer == 0x08 && f.bus.trace.empty());
      CHECK(f.device.bind(f.config).ok());
      CHECK(f.bus.trace.empty());
      const auto health = f.device.healthSnapshot();
      f.chip.words[0x01] = 0xE000;  // DATA_READY, HIGH and LOW await a separate observer.
      for (const std::uint16_t lock : {0x0000, 0x4000, 0x8000, 0xC000, 0x3FFF, 0xFFFF}) {
        f.bus.trace.clear();
        f.chip.words[0x04] = lock;
        CHECK(f.device.readEepromSnapshot(snap).ok() && !snap.failedPointer);
        CHECK(f.bus.trace.size() == f.pointers.size());
        checkPrefix(f, snap, f.pointers.size(), timed);
        for (std::size_t i = 0; i < f.pointers.size(); ++i) f.checkFrame(i, f.pointers[i]);
      }

      // Each failure reuses a full, nonzero snapshot to expose leaked suffixes.
      const auto full = snap;
      for (std::size_t failed = 0; failed < f.pointers.size(); ++failed) {
        f.bus.trace.clear();
        f.bus.script.resize(failed);
        f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "one byte", 1), 1});
        snap = full;
        const auto status = f.device.readEepromSnapshot(snap);
        CHECK(status.is(Err::I2C_SHORT_TRANSFER) && status.detail == 1);
        CHECK(snap.failedPointer == f.pointers[failed] && f.bus.trace.size() == failed + 1);
        checkPrefix(f, snap, failed, timed);
        for (std::size_t i = 0; i < failed; ++i) f.checkFrame(i, f.pointers[i]);
        f.checkFrame(failed, f.pointers[failed], 1);
      }
      for (std::size_t i = 2; i < f.pointers.size(); ++i) f.chip.words[f.pointers[i]] = 0;
      f.bus.trace.clear();
      CHECK(f.device.readEepromSnapshot(snap).ok() && !snap.failedPointer);
      checkPrefix(f, snap, f.pointers.size(), timed);
      CHECK(f.chip.words[0x01] == 0xE000);
      checkNeutral(f.device, health);
    }
  }
}

void identityChangesBetweenObservations() {
  for (const auto model : models) {
    Fixture f{model};
    CHECK(f.device.bind(f.config).ok());
    CHECK(f.device.probe().ok());
    const auto expected = f.chip.words[0x0F];
    f.chip.words[0x0F] = expected == 0x1116 ? 0x0117 : 0x1116;
    EepromSnapshot snap;
    f.bus.trace.clear();
    CHECK(f.device.readEepromSnapshot(snap).is(Err::IDENTITY_MISMATCH));
    CHECK(snap.failedPointer == 0x0F && snap.identity.rawValid && !snap.identity.compatible);
    CHECK(snap.identity.raw == f.chip.words[0x0F] && !snap.lock && snap.wordCount == 0);
    CHECK(f.device.identityInfo().raw == f.chip.words[0x0F] && f.bus.trace.size() == 1);
    f.checkFrame(0, 0x0F);
    f.chip.words[0x0F] = expected;
    std::uint16_t scalar = 0x4567;
    CHECK(f.device.readEepromWord(1, scalar).is(Err::IDENTITY_MISMATCH));
    CHECK(scalar == 0x4567 && f.bus.trace.size() == 1);
    f.bus.trace.clear();
    CHECK(f.device.readEepromSnapshot(snap).ok());
    checkPrefix(f, snap, f.pointers.size(), false);
    CHECK(!snap.failedPointer && f.bus.trace.size() == f.pointers.size());
    CHECK(f.device.readEepromWord(1, scalar).ok() && scalar == 0x12AB);
  }
}

void snapshotsPreserveAlertsAndRespectBusy() {
  Fixture f{Model::TMP117};
  std::uint32_t tick = 0;
  f.config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
  f.config.timeUser = &tick;
  CHECK(f.device.begin(f.config).ok());
  for (const bool busy : {false, true}) {
    f.chip.words[0x01] |= 0xA000;
    ConfigurationObservation observed;
    CHECK(f.device.readConfiguration(observed).ok() && observed.dataReady);
    const auto health = f.device.healthSnapshot();
    EepromSnapshot snap;
    f.chip.words[0x04] = busy ? 0x4000 : 0;
    CHECK(f.device.readEepromSnapshot(snap).ok() && snap.lock && snap.lock->busy() == busy);
    checkNeutral(f.device, health);
    AlertStatus alerts;
    CHECK(f.device.takeAlertEvents(alerts).ok() && alerts.pendingHigh && !alerts.possibleLoss);
    CHECK(alerts.lastConfigAtMs == observed.observedAtMs);
    f.chip.words[0x04] = 0;
    f.bus.trace.clear();
    Sample sample;
    const auto status = f.device.tryReadFresh(sample);
    CHECK(f.bus.trace.size() == 2);
    f.checkFrame(0, 0x04);
    if (busy) {
      CHECK(status.is(Err::MEASUREMENT_NOT_READY));
      f.checkFrame(1, 0x01);
    } else {
      CHECK(status.ok() && sample.freshConversion);
      f.checkFrame(1, 0x00);
    }
  }
}

void unlockEvidenceSurvivesPartialSnapshot() {
  Fixture f{Model::TMP117};
  std::uint32_t tick = 0;
  f.config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
  f.config.timeUser = &tick;
  CHECK(f.device.begin(f.config).ok());
  Sample sample;
  f.bus.script.push_back({Status::Error(Err::I2C_TIMEOUT, "tracked timeout", -10)});
  CHECK(f.device.readLatest(sample).is(Err::I2C_TIMEOUT));
  const auto health = f.device.healthSnapshot();
  CHECK(health.totalSuccess > 0 && health.totalFailures == 1 && health.consecutiveFailures == 1);
  CHECK(health.lastSuccessMsValid && health.lastErrorMsValid && health.lastError.detail == -10);
  EepromSnapshot snap;
  f.chip.words[0x04] = 0xC000;
  f.bus.script.resize(2);
  f.bus.script.push_back({Status::Error(Err::I2C_NACK_DATA, "EEPROM1 offline")});
  CHECK(f.device.readEepromSnapshot(snap).is(Err::I2C_NACK_DATA));
  CHECK(snap.lock && snap.lock->unlocked() && snap.lock->busy());
  CHECK(snap.wordCount == 0 && snap.failedPointer == 0x05);
  f.chip.words[0x04] = 0;
  CHECK(f.device.readEepromSnapshot(snap).ok() && snap.lock && !snap.lock->unlocked());
  f.bus.trace.clear();
  CHECK(f.device.tryReadFresh(sample).is(Err::EEPROM_UNSAFE));
  CHECK(f.device.recover().is(Err::EEPROM_UNSAFE) && f.bus.trace.empty());
  checkNeutral(f.device, health);
  CHECK(f.device.invalidateDeviceState().ok());
  CHECK(f.device.recover().ok());

  // A short lock read may deliver the high byte with EUN set. The byte
  // alone cannot establish a complete unlocked observation.
  f.chip.words[0x04] = 0x8000;
  f.bus.script.resize(1);
  f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short lock"), 1});
  CHECK(f.device.readEepromSnapshot(snap).is(Err::I2C_SHORT_TRANSFER) && !snap.lock);
  f.chip.words[0x04] = 0;
  CHECK(f.device.recover().ok());
}

}  // namespace

int main() {
  scalarBoundsAndFrames();
  snapshotsAndPartialResults();
  identityChangesBetweenObservations();
  snapshotsPreserveAlertsAndRespectBusy();
  unlockEvidenceSurvivesPartialSnapshot();
  return EXIT_SUCCESS;
}
