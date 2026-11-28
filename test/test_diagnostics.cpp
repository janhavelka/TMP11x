#include "TMP11x/TMP11x.h"
#include "LatestRead.h"
#include "support/FakeTmp11xDevice.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);      \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

constexpr Model models[] = {Model::TMP116, Model::TMP116N, Model::TMP117,
                            Model::TMP117M, Model::TMP117N, Model::TMP119};
constexpr std::array<std::uint8_t, 10> order{0x0F, 0, 1, 2, 3, 4, 5, 6, 7, 8};

bool fourShadows(Model model) {
  return model == Model::TMP116 || model == Model::TMP116N;
}

void checkFrame(const TransferTrace& frame, std::uint8_t address, std::uint8_t pointer,
                std::size_t received = 2) {
  CHECK(frame.combinedRead && frame.address == address && frame.timeoutMs == 17);
  CHECK(frame.txLen == 1 && frame.tx.size() == 1 && frame.tx[0] == pointer);
  CHECK(frame.rxLen == 2 && frame.rx.size() == received);
}

void checkTransportHealth(const Device& device, const HealthSnapshot& before) {
  const auto after = device.healthSnapshot();
  CHECK(after.state == before.state && after.consecutiveFailures == before.consecutiveFailures);
  CHECK(after.totalSuccess == before.totalSuccess && after.totalFailures == before.totalFailures);
  CHECK(after.lastSuccessMs == before.lastSuccessMs && after.lastSuccessMsValid == before.lastSuccessMsValid);
  CHECK(after.lastErrorMs == before.lastErrorMs && after.lastErrorMsValid == before.lastErrorMsValid);
  CHECK(after.lastError.code == before.lastError.code && after.lastError.detail == before.lastError.detail);
}

struct Fixture {
  explicit Fixture(Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x4B;
    config.i2cTimeoutMs = 17;
    config.i2cWrite = FakeTmp11xBus::write;
    config.i2cWriteRead = FakeTmp11xBus::writeRead;
    config.i2cUser = &bus;
    CHECK(bus.attach(config.i2cAddress, chip));
    chip.words[0x0F] = fourShadows(model) ? 0x1116 : 0xD117;
  }

  void initialize() {
    config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
    config.timeUser = &tick;
    CHECK(device.begin(config).ok());
    bus.trace.clear();
  }

  void shortReadAfter(std::size_t complete) {
    bus.script.resize(complete);
    bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "one byte", -19), 1, true});
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t tick = 0;
};

void scalarMap() {
  for (const auto model : models) {
    Fixture f{model};
    RegisterObservation word;
    word.raw = 0xCAFE;
    CHECK(f.device.readRegister(0, word).is(Err::NOT_BOUND));
    CHECK(word.raw == 0xCAFE && f.bus.trace.empty());
    CHECK(f.device.bind(f.config).ok());
    const auto health = f.device.healthSnapshot();
    const char* names[] = {"TEMP", "CONFIG", "THIGH", "TLOW", "EEPROM_UL", "EEPROM1", "EEPROM2",
                           fourShadows(model) ? "EEPROM3" : "OFFSET",
                           fourShadows(model) ? "EEPROM4" : "EEPROM3"};
    for (unsigned byte = 0; byte < 256; ++byte) {
      const auto pointer = static_cast<std::uint8_t>(byte);
      const bool mapped = pointer <= 8 || pointer == 0x0F;
      if (pointer <= 8) f.chip.words[pointer] = static_cast<std::uint16_t>(0xFF80 + byte);
      const auto before = f.bus.trace.size();
      word.raw = 0xCAFE;
      const auto status = f.device.readRegister(pointer, word);
      if (!mapped) {
        CHECK(status.is(Err::INVALID_PARAM) && word.raw == 0xCAFE && f.bus.trace.size() == before);
        continue;
      }
      CHECK(status.ok() && f.bus.trace.size() == before + 1);
      checkFrame(f.bus.trace.back(), 0x4B, pointer);
      CHECK(word.pointer == pointer && word.raw == ((f.bus.trace.back().rx[0] << 8) | f.bus.trace.back().rx[1]));
      CHECK(word.identity.model == model && word.identity.i2cAddress == 0x4B);
      CHECK(word.identity.rawValid == (pointer == 0x0F) && word.identity.compatible == (pointer == 0x0F));
      CHECK(!word.observedAtMs && !word.startupReady && word.trust == ConfigTrust::UNKNOWN);
      CHECK(std::string_view(word.name()) == (pointer == 0x0F ? "DEVICE_ID" : names[pointer]));
      const bool signedWord = pointer == 0 || pointer == 2 || pointer == 3 || (pointer == 7 && !fourShadows(model));
      CHECK(word.signedCounts().has_value() == signedWord);
      if (signedWord) CHECK(*word.signedCounts() == -128 + static_cast<int>(byte));
    }
    CHECK(f.bus.trace.size() == order.size() && !f.device.isInitialized());
    checkTransportHealth(f.device, health);
  }
}

void bindOnlyTranscript() {
  Fixture f;
  CHECK(f.device.bind(f.config).ok() && f.bus.trace.empty());
  f.chip.words[1] = 0xE400;
  f.chip.words[0] = 0xFF80;
  f.chip.words[8] = 0xC0DE;
  RegisterObservation word;
  for (const std::uint8_t pointer : {0x0F, 1, 0, 8}) {
    const auto expected = f.chip.words[pointer];
    CHECK(f.device.readRegister(pointer, word).ok() && word.raw == expected);
    checkFrame(f.bus.trace.back(), 0x4B, pointer);
    CHECK(word.identity.compatible && !word.startupReady && word.trust == ConfigTrust::UNKNOWN);
    CHECK(!word.observedAtMs && !f.device.isInitialized());
  }
  CHECK(f.bus.trace.size() == 4 && f.chip.words[8] == 0xC0DE);
  const auto config = f.device.getSettingsSnapshot().config;
  CHECK(config && config->raw == 0xE400 && !detail::LatestRead{f.device}.cache);
}

// ID completion is tick zero. A timestamp sampled before its callback is distinguishable.
std::uint32_t frameTick(void* user) {
  return static_cast<std::uint32_t>(static_cast<FakeTmp11xBus*>(user)->trace.size()) - 1;
}

void snapshotPrefixes() {
  for (const auto model : models) {
    for (std::size_t complete = 0; complete <= order.size(); ++complete) {
      Fixture f{model};
      RegisterSnapshot out;
      out.wordCount = 1;
      out.words[0].raw = 0xBEEF;
      out.failedPointer = 8;
      CHECK(f.device.readRegisterSnapshot(out).is(Err::NOT_BOUND));
      CHECK(out.wordCount == 1 && out.words[0].raw == 0xBEEF && out.failedPointer == 8);
      f.config.nowMs = frameTick;
      f.config.timeUser = &f.bus;
      CHECK(f.device.bind(f.config).ok());
      f.chip.words[0] = 0xFF80;
      f.chip.words[1] = 0xE400;  // Shutdown, with DATA_READY and both alert latches asserted.
      f.chip.words[2] = 0xABCD;
      f.chip.words[3] = 0xEF02;
      f.chip.words[7] = 0x0040;
      const auto health = f.device.healthSnapshot();
      const bool fails = complete < order.size();
      if (fails) f.shortReadAfter(complete);
      const auto status = f.device.readRegisterSnapshot(out);
      CHECK(out.wordCount == complete && out.identity.model == model && out.identity.i2cAddress == 0x4B);
      CHECK(out.identity.rawValid == (complete != 0) && out.identity.compatible == (complete != 0));
      CHECK(f.bus.trace.size() == complete + (fails ? 1 : 0));
      if (fails) {
        const auto failedPointer = order[complete];
        CHECK(status.is(Err::I2C_SHORT_TRANSFER) && status.detail == -19);
        CHECK(out.failedPointer == failedPointer);
        CHECK(out.possibleReadClearMask == (failedPointer == 0 ? 0x2000 : failedPointer == 1 ? 0xE000 : 0));
        checkFrame(f.bus.trace.back(), 0x4B, failedPointer, 1);
      } else {
        CHECK(status.ok() && !out.failedPointer && out.possibleReadClearMask == 0);
      }
      for (std::size_t i = 0; i < complete; ++i) {
        const auto& word = out.words[i];
        const auto& frame = f.bus.trace[i];
        checkFrame(frame, 0x4B, order[i]);
        CHECK(word.pointer == order[i] && word.raw == ((frame.rx[0] << 8) | frame.rx[1]));
        CHECK(word.observedAtMs == i && word.identity.compatible && !word.startupReady);
        CHECK(word.trust == ConfigTrust::UNKNOWN && word.profileGeneration == 0);
        CHECK(word.readClearMask == (order[i] == 0 ? 0x2000 : order[i] == 1 ? 0xE000 : 0));
      }
      const auto alerts = f.device.getAlertStatus();
      CHECK(alerts.possibleLoss == (fails && order[complete] == 1));
      if (complete >= 3) {
        // TEMP precedes CONFIG. HIGH and LOW survive TEMP, DATA_READY does not.
        CHECK(out.words[2].raw == 0xC400 && alerts.pendingHigh && alerts.pendingLow);
        CHECK(alerts.firstEvent && alerts.firstEvent->source == ConfigReadSource::DIAGNOSTIC);
        CHECK(alerts.firstEvent->observedAtMs == 2 && alerts.mixedOrUnknown);
        const auto config = f.device.getSettingsSnapshot().config;
        CHECK(config && config->timestampValid && config->observedAtMs == 2);
      }
      CHECK(!f.device.isInitialized() && !detail::LatestRead{f.device}.cache);
      checkTransportHealth(f.device, health);
    }
  }
}

void sharedBusNeighbors() {
  // Match the stage 13 ID fixtures. Each foreign device's 0x02 has different
  // semantics. A compatible neighbor at 0x4B must never receive a fallback probe.
  Fixture f;
  FakeTmp11xDevice tmp114{Model::TMP117}, tmp118{Model::TMP117};
  CHECK(f.bus.attach(0x48, tmp114) && f.bus.attach(0x49, tmp118));
  RegisterSnapshot snap;
  CHECK(f.device.bind(f.config).ok() && f.device.readRegisterSnapshot(snap).ok());
  for (const std::uint8_t address : {0x48, 0x49}) {
    f.config.i2cAddress = address;
    CHECK(f.device.bind(f.config).ok());
    f.bus.trace.clear();
    CHECK(f.device.readRegisterSnapshot(snap).is(Err::IDENTITY_MISMATCH));
    CHECK(f.bus.trace.size() == 1 && snap.wordCount == 1 && snap.failedPointer == 0x0F);
    checkFrame(f.bus.trace.front(), address, 0x0F);
    CHECK(snap.words[0].raw == 0 && snap.identity.rawValid && !snap.identity.compatible);
  }

  // Independent CRC-enabled TMP114 adverse outcomes. TI does not specify the
  // exact response to an omitted CRC byte. Neither case licenses another read.
  f.config.i2cAddress = 0x48;
  CHECK(f.device.bind(f.config).ok());
  f.bus.trace.clear();
  f.bus.script.push_back({Status::Error(Err::I2C_NACK_DATA, "CRC NACK", -14)});
  CHECK(f.device.readRegisterSnapshot(snap).is(Err::I2C_NACK_DATA));
  CHECK(f.bus.trace.size() == 1 && snap.wordCount == 0 && snap.failedPointer == 0x0F);
  checkFrame(f.bus.trace.front(), 0x48, 0x0F, 0);
  tmp114.words[0x0F] = 0xFFFF;
  f.bus.trace.clear();
  CHECK(f.device.readRegisterSnapshot(snap).is(Err::IDENTITY_MISMATCH));
  CHECK(f.bus.trace.size() == 1 && snap.wordCount == 1 && snap.words[0].raw == 0xFFFF);
  checkFrame(f.bus.trace.front(), 0x48, 0x0F);
}

void changedIdentityAndTrackedFault() {
  Fixture f;
  f.initialize();
  Sample sample;
  f.bus.script.push_back({Status::Error(Err::I2C_TIMEOUT, "tracked timeout", -7)});
  CHECK(f.device.readLatest(sample).is(Err::I2C_TIMEOUT));
  const auto health = f.device.healthSnapshot();
  CHECK(health.totalSuccess != 0 && health.consecutiveFailures == 1 && health.lastError.detail == -7);
  RegisterSnapshot snap;
  CHECK(f.device.readRegisterSnapshot(snap).ok());
  // A failed ID recheck publishes no ID from the previous scan.
  f.shortReadAfter(0);
  f.bus.trace.clear();
  CHECK(f.device.readRegisterSnapshot(snap).is(Err::I2C_SHORT_TRANSFER));
  CHECK(snap.wordCount == 0 && snap.failedPointer == 0x0F && !snap.identity.rawValid);
  CHECK(f.bus.trace.size() == 1 && f.device.identityInfo().compatible && f.device.isInitialized());
  f.chip.words[0x0F] = 0x1116;
  f.bus.trace.clear();
  CHECK(f.device.readRegisterSnapshot(snap).is(Err::IDENTITY_MISMATCH));
  CHECK(snap.wordCount == 1 && snap.failedPointer == 0x0F && snap.identity.raw == 0x1116);
  checkFrame(f.bus.trace.at(0), 0x4B, 0x0F);
  CHECK(!f.device.isInitialized() && f.device.configTrust() == ConfigTrust::UNKNOWN);
  RegisterObservation word;
  word.raw = 0xCAFE;
  CHECK(f.device.readRegister(1, word).is(Err::IDENTITY_MISMATCH) && word.raw == 0xCAFE);
  CHECK(f.bus.trace.size() == 1);
  CHECK(f.device.readRegister(0x0F, word).is(Err::IDENTITY_MISMATCH));
  CHECK(word.raw == 0x1116 && word.identity.rawValid && !word.identity.compatible);
  f.chip.words[0x0F] = 0xB117;
  CHECK(f.device.readRegister(0x0F, word).ok() && word.identity.compatible && word.identity.revision == 11);
  CHECK(f.device.readRegister(1, word).ok() && word.identity.raw == 0xB117);
  CHECK(f.bus.trace.size() == 4);
  checkTransportHealth(f.device, health);
}

void repeatedConfigAndFreshness() {
  Fixture f;
  f.initialize();
  const auto generation = f.device.getSettingsSnapshot().verifiedGeneration;
  const auto health = f.device.healthSnapshot();
  RegisterObservation word;
  f.chip.words[1] |= 0xE000;
  CHECK(f.device.readRegister(1, word).ok() && (word.raw & 0xE000) == 0xE000);
  CHECK(word.readClearMask == 0xE000 && word.trust == ConfigTrust::VERIFIED && word.startupReady);
  const auto capturedAt = word.observedAtMs;
  CHECK(capturedAt && f.device.readRegister(1, word).ok() && (word.raw & 0xE000) == 0);
  AlertStatus alerts;
  CHECK(f.device.takeAlertEvents(alerts).ok() && alerts.pendingHigh && alerts.pendingLow);
  CHECK(alerts.firstEvent && alerts.firstEvent->source == ConfigReadSource::DIAGNOSTIC);
  CHECK(alerts.firstEvent->profileGeneration == generation && alerts.firstEvent->observedAtMs == capturedAt);
  CHECK(alerts.lastConfigAtMs == word.observedAtMs && !alerts.possibleLoss && !alerts.mixedOrUnknown);
  CHECK(f.device.takeAlertEvents(alerts).ok() && !alerts.pendingHigh && !alerts.pendingLow);
  checkTransportHealth(f.device, health);
  CHECK(!detail::LatestRead{f.device}.cache);
  f.bus.trace.clear();
  Sample sample;
  CHECK(f.device.tryReadFresh(sample).ok() && sample.freshConversion && sample.profileGeneration == generation);
  CHECK(f.bus.trace.size() == 2 && f.bus.trace[0].tx[0] == 4 && f.bus.trace[1].tx[0] == 0);
  CHECK(f.device.tryReadFresh(sample).is(Err::MEASUREMENT_NOT_READY));

  Fixture therm;
  CHECK(therm.device.bind(therm.config).ok());
  therm.chip.words[1] = 0xA010;
  CHECK(therm.device.readRegister(1, word).ok() && word.raw == 0xA010 && word.readClearMask == 0x2000);
  CHECK(therm.device.readRegister(1, word).ok() && word.raw == 0x8010 && word.readClearMask == 0x2000);
  CHECK(therm.device.takeAlertEvents(alerts).ok() && alerts.thermHigh == true);
  CHECK(!alerts.pendingHigh && !alerts.pendingLow && !alerts.firstEvent);
}

void ambiguousDestructiveScalars() {
  for (const std::uint8_t pointer : {0, 1}) {
    Fixture f;
    f.initialize();
    f.chip.words[1] |= 0x2000;
    RegisterObservation word;
    CHECK(f.device.readRegister(1, word).ok());
    f.chip.words[1] |= 0xA000;
    f.shortReadAfter(0);
    word.raw = 0xCAFE;
    const auto health = f.device.healthSnapshot();
    const auto status = f.device.readRegister(pointer, word);
    CHECK(status.is(Err::I2C_SHORT_TRANSFER) && status.detail == -19 && word.raw == 0xCAFE);
    CHECK(f.device.getAlertStatus().possibleLoss == (pointer == 1));
    CHECK(f.chip.words[1] >> 13 == (pointer == 1 ? 0 : 4));
    checkTransportHealth(f.device, health);
    Sample sample;
    f.bus.trace.clear();
    CHECK(f.device.tryReadFresh(sample).is(Err::MEASUREMENT_NOT_READY));
    CHECK(f.bus.trace.size() == 2 && f.bus.trace.back().tx[0] == 1);
    CHECK(!detail::LatestRead{f.device}.cache);
  }
}

void driftDoesNotRetarget() {
  Fixture f;
  f.config.settings.managedOffsetCounts = 31;
  f.initialize();
  const auto original = f.chip.words;
  f.chip.words[1] ^= 0x0020;
  f.chip.words[2] ^= 1;
  f.chip.words[3] ^= 1;
  f.chip.words[7] = 64;
  RegisterSnapshot snap;
  CHECK(f.device.readRegisterSnapshot(snap).ok() && snap.wordCount == order.size());
  auto settings = f.device.getSettingsSnapshot();
  CHECK(settings.trust == ConfigTrust::DIRTY && settings.dirtyReason == DirtyReason::READBACK_MISMATCH);
  CHECK(settings.affectedFields == 0x0F);
  CHECK(settings.config && settings.config->raw == f.chip.words[1]);
  CHECK(settings.low && settings.low->raw == f.chip.words[3] && settings.high && settings.high->raw == f.chip.words[2]);
  CHECK(settings.offset && settings.offset->counts == 64 && settings.desired.managedOffsetCounts == 31);
  CHECK(settings.desired.averaging == f.config.settings.averaging);
  CHECK(settings.desired.limits.lowCounts == f.config.settings.limits.lowCounts);
  CHECK(settings.desired.limits.highCounts == f.config.settings.limits.highCounts);
  f.chip.words = original;
  CHECK(f.device.readRegisterSnapshot(snap).ok());
  settings = f.device.getSettingsSnapshot();
  CHECK(settings.trust == ConfigTrust::DIRTY && settings.dirtyReason == DirtyReason::READBACK_MISMATCH);
  CHECK(settings.affectedFields == 0x0F && settings.offset && settings.offset->counts == 31);
}

void calibrationContinuity() {
  for (const auto model : models) {
    for (const bool dump : {false, true}) {
      Fixture f{model};
      f.initialize();
      f.chip.words[0] = 0x0C00;
      f.chip.words[1] |= 0x2000;
      Sample sample;
      CHECK(f.device.tryReadFresh(sample).ok() && sample.raw == 0x0C00 && sample.freshConversion);
      f.chip.words[1] |= 0x2000;
      RegisterObservation word;
      CHECK(f.device.readRegister(1, word).ok() && (word.raw & 0x2000) != 0);
      f.chip.words[7] = 64;
      f.bus.trace.clear();
      if (dump) {
        RegisterSnapshot snap;
        CHECK(f.device.readRegisterSnapshot(snap).ok() && snap.wordCount == order.size());
        word = snap.words[8];
        CHECK(f.bus.trace.size() == order.size());
      } else {
        CHECK(f.device.readRegister(7, word).ok() && f.bus.trace.size() == 1);
      }
      CHECK(word.pointer == 7 && word.raw == 64);
      const auto settings = f.device.getSettingsSnapshot();
      const auto& cached = detail::LatestRead{f.device}.cache;
      CHECK(cached && cached->raw == sample.raw && cached->profileGeneration == sample.profileGeneration);
      CHECK(cached->freshConversion && cached->observedAtMs == sample.observedAtMs);
      CHECK(cached->offset.state == sample.offset.state && cached->offset.counts == 0);
      CHECK(!settings.desired.managedOffsetCounts);
      f.bus.trace.clear();
      if (fourShadows(model)) {
        CHECK(!word.signedCounts() && !settings.offset && f.device.isInitialized());
        CHECK(settings.trust == ConfigTrust::VERIFIED && cached->offset.state == OffsetState::UNAVAILABLE);
        CHECK(!cached->offset.countsValid);
        // Only the full dump read TEMP and acknowledged the pending indication.
        const auto status = f.device.tryReadFresh(sample);
        CHECK(dump ? status.is(Err::MEASUREMENT_NOT_READY) : status.ok());
      } else {
        CHECK(word.signedCounts() == 64 && settings.offset && settings.offset->counts == 64);
        CHECK(settings.offset->observedAtMs == word.observedAtMs);
        CHECK(!f.device.isInitialized() && settings.trust == ConfigTrust::UNKNOWN);
        CHECK(cached->offset.state == OffsetState::OBSERVED_UNMANAGED && cached->offset.countsValid);
        CHECK(f.device.tryReadFresh(sample).is(Err::NOT_INITIALIZED) && f.bus.trace.empty());
        CHECK(f.device.recover().ok());
        CHECK(f.device.tryReadFresh(sample).is(Err::MEASUREMENT_NOT_READY));
      }
    }
  }
}

void guardObservations() {
  Fixture f;
  f.initialize();
  RegisterObservation word;
  f.chip.words[1] |= 0x2000;
  CHECK(f.device.readRegister(1, word).ok());
  f.chip.words[4] = 0x4000;
  CHECK(f.device.readRegister(4, word).ok() && word.raw == 0x4000);
  f.chip.words[4] = 0;
  Sample sample;
  CHECK(f.device.tryReadFresh(sample).is(Err::MEASUREMENT_NOT_READY));
  const auto health = f.device.healthSnapshot();
  RegisterSnapshot snap;
  f.chip.words[4] = 0xC000;
  f.shortReadAfter(6);
  CHECK(f.device.readRegisterSnapshot(snap).is(Err::I2C_SHORT_TRANSFER));
  CHECK(snap.wordCount == 6 && snap.words[5].raw == 0xC000 && snap.failedPointer == 5);
  f.chip.words[4] = 0;
  CHECK(f.device.readRegister(4, word).ok() && word.raw == 0);
  f.bus.trace.clear();
  CHECK(f.device.recover().is(Err::EEPROM_UNSAFE) && f.bus.trace.empty());
  checkTransportHealth(f.device, health);
}

}  // namespace

int main() {
  scalarMap();
  bindOnlyTranscript();
  snapshotPrefixes();
  sharedBusNeighbors();
  changedIdentityAndTrackedFault();
  repeatedConfigAndFreshness();
  ambiguousDestructiveScalars();
  driftDoesNotRetarget();
  calibrationContinuity();
  guardObservations();
  return EXIT_SUCCESS;
}
