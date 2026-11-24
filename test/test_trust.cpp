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
    config.i2cAddress = 0x48;
    config.settings.limits = {-256, 1280};
    config.timeUser = &tick;
    config.nowMs = [](void* p) { return (*static_cast<std::uint32_t*>(p))++; };
    config.i2cUser = this;
    config.i2cWrite = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                         std::uint32_t timeout, void* p) {
      auto& f = *static_cast<Fixture*>(p);
      return FakeTmp11xBus::write(address, tx, n, timeout, &f.bus);
    };
    config.i2cWriteRead = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                             std::uint8_t* rx, std::size_t m, std::uint32_t timeout, void* p) {
      auto& f = *static_cast<Fixture*>(p);
      if (*tx == 0x0F) f.idTick = f.tick;
      if (*tx == 0x04) f.guardTick = f.tick;
      return FakeTmp11xBus::writeRead(address, tx, n, rx, m, timeout, &f.bus);
    };
    chip.words[0x0F] = (model == Model::TMP116 || model == Model::TMP116N) ? 0x1116 : 0xB117;
    CHECK(bus.attach(config.i2cAddress, chip));
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t tick = 0;
  std::uint32_t idTick = 0;
  std::uint32_t guardTick = 0;
};

void checkVerificationReads(const Fixture& f) {
  const auto& trace = f.bus.trace;
  CHECK(trace.size() == 3);
  for (const auto& frame : trace) CHECK(frame.combinedRead && frame.tx.size() == 1);
  CHECK(trace[0].tx[0] == 1 && trace[1].tx[0] == 3 && trace[2].tx[0] == 2);
}

void admissionAndObservations() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117, Model::TMP117M,
                           Model::TMP117N, Model::TMP119}) {
    Fixture f{model};
    auto& device = f.device;
    ConfigurationObservation observed{};
    observed.raw = 0xBEEF;
    CHECK(device.invalidateDeviceState().is(Err::NOT_BOUND));
    CHECK(device.readConfiguration(observed).is(Err::NOT_BOUND));
    CHECK(device.verifyConfiguration().is(Err::NOT_BOUND));
    CHECK(device.bind(f.config).ok());
    CHECK(device.readConfiguration(observed).is(Err::NOT_INITIALIZED));
    CHECK(device.verifyConfiguration().is(Err::NOT_INITIALIZED));
    CHECK(observed.raw == 0xBEEF && f.bus.trace.empty());
    const auto idleTick = f.tick;
    const auto empty = device.getSettingsSnapshot();
    CHECK(f.tick == idleTick && f.bus.trace.empty());
    CHECK(empty.trust == ConfigTrust::UNKNOWN && !empty.verifiedAtMs);
    CHECK(!empty.config && !empty.low && !empty.high && empty.desired.limits.lowCounts == -256);

    CHECK(device.begin().ok());
    const auto initial = device.getSettingsSnapshot();
    CHECK(initial.trust == ConfigTrust::VERIFIED && initial.dirtyReason == DirtyReason::NONE);
    CHECK(initial.verifiedAtMs && initial.config && initial.low && initial.high);
    CHECK(initial.config->raw == 0x0220 && initial.config->timestampValid);
    CHECK(initial.low->raw == 0xFF00 && initial.low->counts == -256 && initial.low->observedAtMs);
    CHECK(initial.high->raw == 0x0500 && initial.high->counts == 1280 && initial.high->observedAtMs);

    f.bus.trace.clear();
    f.chip.words[1] = 0xFA20;  // Status, BUSY, and the continuous alias.
    const auto successes = device.healthSnapshot().totalSuccess;
    CHECK(device.readConfiguration(observed).ok());
    CHECK(observed.raw == 0xFA20 && observed.rawMode() == 2 && observed.eepromBusy && observed.dataReady);
    CHECK(observed.timestampValid && observed.observedAtMs == device.healthSnapshot().lastSuccessMs);
    CHECK(f.chip.words[1] == 0x1A20 && device.configTrust() == ConfigTrust::VERIFIED);
    CHECK(f.bus.trace.size() == 1 && f.bus.trace[0].combinedRead && f.bus.trace[0].tx[0] == 1);
    CHECK(device.healthSnapshot().totalSuccess == successes + 1);
    CHECK(device.getSettingsSnapshot().verifiedAtMs == initial.verifiedAtMs);
    f.bus.trace.clear();
    CHECK(device.verifyConfiguration().ok());
    checkVerificationReads(f);
    const auto confirmed = device.getSettingsSnapshot();
    CHECK(confirmed.verifiedGeneration == initial.verifiedGeneration);
    CHECK(confirmed.verifiedAtMs == device.healthSnapshot().lastSuccessMs);
    CHECK(confirmed.verifiedAtMs != initial.verifiedAtMs);

    f.bus.trace.clear();
    f.chip.words[1] |= 0x0008;  // Same BUSY plus a stable polarity mismatch.
    CHECK(device.readConfiguration(observed).ok());
    CHECK(observed.raw == 0x1A28 && observed.polarity == Polarity::ACTIVE_HIGH);
    const auto dirty = device.getSettingsSnapshot();
    CHECK(dirty.config && dirty.config->raw == observed.raw);
    CHECK(dirty.trust == ConfigTrust::DIRTY && dirty.desired.polarity == Polarity::ACTIVE_LOW);
    CHECK(dirty.dirtyReason == DirtyReason::READBACK_MISMATCH);
    CHECK(dirty.affectedFields == static_cast<std::uint8_t>(SettingsField::CONFIG));
    CHECK(dirty.verifiedGeneration == initial.verifiedGeneration && dirty.verifiedAtMs == confirmed.verifiedAtMs);
    CHECK(f.bus.trace.size() == 1);
    f.bus.trace.clear();
    CHECK(device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
    checkVerificationReads(f);
    CHECK(device.healthSnapshot().totalFailures == 0);
    f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "partial CONFIG"), 1, true});
    const auto cached = device.getSettingsSnapshot();
    observed.raw = 0xAAAA;
    CHECK(device.readConfiguration(observed).is(Err::I2C_SHORT_TRANSFER) && observed.raw == 0xAAAA);
    CHECK(device.getSettingsSnapshot().config->observedAtMs == cached.config->observedAtMs);
  }
}

void verifyLimitDrift() {
  for (const auto pointer : {2, 3}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    const auto generation = f.device.getSettingsSnapshot().verifiedGeneration;
    f.chip.words[pointer] ^= 1;
    const auto changed = f.chip.words[pointer];
    f.bus.trace.clear();
    CHECK(f.device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
    checkVerificationReads(f);
    const auto snapshot = f.device.getSettingsSnapshot();
    const bool low = pointer == 3;
    const auto field = low ? SettingsField::LOW_LIMIT : SettingsField::HIGH_LIMIT;
    const auto& limit = low ? snapshot.low : snapshot.high;
    CHECK(snapshot.dirtyReason == DirtyReason::READBACK_MISMATCH);
    CHECK(snapshot.affectedFields == static_cast<std::uint8_t>(field));
    CHECK(limit && limit->raw == changed && limit->counts == (low ? -255 : 1281));
    CHECK(snapshot.desired.limits.lowCounts == -256 && snapshot.desired.limits.highCounts == 1280);
    CHECK(f.device.recover().ok());
    const auto recovered = f.device.getSettingsSnapshot();
    CHECK(recovered.trust == ConfigTrust::VERIFIED && recovered.verifiedGeneration == generation + 1);
    CHECK(recovered.dirtyReason == DirtyReason::NONE && recovered.affectedFields == 0);
    CHECK(f.chip.words[pointer] == (low ? 0xFF00 : 0x0500));
  }
}

void partialVerificationRetainsEvidence() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  f.bus.trace.clear();
  CHECK(f.device.verifyConfiguration().ok());
  checkVerificationReads(f);
  const auto before = f.device.getSettingsSnapshot();
  const auto health = f.device.healthSnapshot();
  f.bus.script = {{}, {}, {Status::Error(Err::I2C_TIMEOUT, "final limit timed out")}};
  f.chip.words[1] ^= 0x0004;
  f.chip.words[2] ^= 1;
  f.chip.words[3] ^= 1;
  f.bus.trace.clear();
  CHECK(f.device.verifyConfiguration().is(Err::I2C_TIMEOUT));
  checkVerificationReads(f);
  const auto partial = f.device.getSettingsSnapshot();
  CHECK(partial.config && partial.config->raw == 0x0224);
  CHECK(partial.low && partial.high && partial.trust == ConfigTrust::DIRTY);
  const auto afterTimeout = f.device.healthSnapshot();
  CHECK(partial.config->timestampValid && partial.config->observedAtMs > before.config->observedAtMs);
  CHECK(partial.low->raw == 0xFF01 && partial.low->observedAtMs);
  CHECK(*partial.low->observedAtMs > partial.config->observedAtMs);
  CHECK(partial.low->observedAtMs == afterTimeout.lastSuccessMs);
  CHECK(partial.high->raw == 0x0500 && partial.high->observedAtMs == before.high->observedAtMs);
  const auto prefix = static_cast<unsigned>(SettingsField::CONFIG) | static_cast<unsigned>(SettingsField::LOW_LIMIT);
  CHECK(partial.affectedFields == prefix);
  CHECK(partial.verificationReads == partial.affectedFields);
  CHECK(partial.dirtyReason == DirtyReason::READBACK_MISMATCH && partial.verifiedAtMs == before.verifiedAtMs);
  CHECK(partial.verifiedGeneration == before.verifiedGeneration);
  CHECK(partial.desired.pinFunction == PinFunction::THRESHOLD);
  CHECK(partial.desired.limits.lowCounts == -256 && partial.desired.limits.highCounts == 1280);
  CHECK(afterTimeout.totalSuccess == health.totalSuccess + 2 && afterTimeout.totalFailures == health.totalFailures + 1);
  CHECK(afterTimeout.lastError.is(Err::I2C_TIMEOUT) && afterTimeout.lastErrorMs > afterTimeout.lastSuccessMs);

  // An outside correction cannot certify the uninterrupted profile.
  f.chip.words[1] = 0x0220;
  f.chip.words[2] = 0x0500;
  f.chip.words[3] = 0xFF00;
  f.bus.trace.clear();
  CHECK(f.device.verifyConfiguration().is(Err::CONFIG_DIRTY));
  checkVerificationReads(f);
  const auto matching = f.device.getSettingsSnapshot();
  CHECK(matching.trust == ConfigTrust::DIRTY && matching.dirtyReason == partial.dirtyReason);
  CHECK(matching.affectedFields == partial.affectedFields && matching.verifiedAtMs == before.verifiedAtMs);
  CHECK(matching.verifiedGeneration == before.verifiedGeneration);
  CHECK(matching.verificationReads == (prefix | static_cast<unsigned>(SettingsField::HIGH_LIMIT)));
  CHECK(matching.config->raw == 0x0220 && matching.low->raw == 0xFF00 && matching.high->raw == 0x0500);
  CHECK(matching.high->observedAtMs == f.device.healthSnapshot().lastSuccessMs);

  f.bus.script.push_back({Status::Error(Err::I2C_NACK, "CONFIG failed")});
  f.bus.trace.clear();
  CHECK(f.device.verifyConfiguration().is(Err::I2C_NACK));
  CHECK(f.bus.trace.size() == 1 && f.bus.trace[0].tx[0] == 1);
  const auto failed = f.device.getSettingsSnapshot();
  CHECK(failed.verificationReads == 0 && failed.config->observedAtMs == matching.config->observedAtMs);
  CHECK(failed.low->observedAtMs == matching.low->observedAtMs && failed.high->observedAtMs == matching.high->observedAtMs);
  CHECK(failed.dirtyReason == DirtyReason::READBACK_MISMATCH && failed.affectedFields == prefix);

  f.bus.trace.clear();
  CHECK(f.device.recover().ok());
  CHECK(f.bus.trace.size() == 14);
  CHECK(f.bus.trace[0].combinedRead && f.bus.trace[0].tx[0] == 0x0F);
  CHECK(f.bus.trace[1].combinedRead && f.bus.trace[1].tx[0] == 0x04);
  CHECK(f.bus.trace.back().combinedRead && f.bus.trace.back().tx[0] == 0x01);
  const auto recovered = f.device.getSettingsSnapshot();
  CHECK(recovered.trust == ConfigTrust::VERIFIED && recovered.dirtyReason == DirtyReason::NONE);
  CHECK(recovered.affectedFields == 0 && recovered.verificationReads == 0);
  CHECK(recovered.verifiedGeneration == before.verifiedGeneration + 1);
  CHECK(recovered.verifiedAtMs == f.device.healthSnapshot().lastSuccessMs);
}

void ambiguousWritesKeepTheTarget() {
  for (const bool initialized : {false, true}) {
    for (const bool firstWrite : {false, true}) {
      Fixture f;
      CHECK(f.device.bind(f.config).ok());
      if (initialized) CHECK(f.device.begin().ok());
      const auto previous = f.device.getSettingsSnapshot();
      // ID, EEPROM_UL, CONFIG, TLOW, THIGH precede the first SD write.
      // The first limit follows SD and its readback.
      f.bus.script.resize(firstWrite ? 5 : 7);
      TransferOutcome fault;
      fault.status = Status::Error(Err::I2C_NACK_DATA, "ambiguous write");
      fault.failedWriteWord = firstWrite ? 0x0620 : 0xFF00;
      f.bus.script.push_back(fault);
      f.bus.trace.clear();
      CHECK(f.device.recover().is(Err::I2C_NACK_DATA));
      const auto pointer = firstWrite ? 1 : 3;
      CHECK(!f.bus.trace.back().combinedRead && f.bus.trace.back().tx[0] == pointer);
      CHECK(f.chip.words[pointer] == *fault.failedWriteWord);
      const auto dirty = f.device.getSettingsSnapshot();
      CHECK(dirty.trust == ConfigTrust::DIRTY && dirty.dirtyReason == DirtyReason::AMBIGUOUS_WRITE);
      const auto field = firstWrite ? SettingsField::CONFIG : SettingsField::LOW_LIMIT;
      CHECK((dirty.affectedFields & static_cast<std::uint8_t>(field)) != 0);
      CHECK(firstWrite ? !dirty.config : !dirty.low);
      CHECK(dirty.verifiedAtMs == previous.verifiedAtMs && dirty.verifiedGeneration == previous.verifiedGeneration);
      CHECK(dirty.desired.mode == Mode::CONTINUOUS && dirty.desired.limits.lowCounts == -256);
      Sample sample{};
      f.bus.trace.clear();
      if (initialized) {
        CHECK(f.device.readLatest(sample).ok() && sample.configTrust == ConfigTrust::DIRTY && !sample.freshConversion);
        ConfigurationObservation config;
        CHECK(f.device.readConfiguration(config).ok() && config.mode == ObservedMode::SHUTDOWN);
        CHECK(f.device.getSettingsSnapshot().dirtyReason == DirtyReason::AMBIGUOUS_WRITE);
      } else {
        CHECK(f.device.readLatest(sample).is(Err::NOT_INITIALIZED) && f.bus.trace.empty());
      }
      CHECK(f.device.recover().ok());
      const auto recovered = f.device.getSettingsSnapshot();
      CHECK(recovered.trust == ConfigTrust::VERIFIED && recovered.dirtyReason == DirtyReason::NONE);
      CHECK(recovered.config && recovered.config->raw == 0x0220);
      CHECK(recovered.low && recovered.low->counts == -256 && recovered.high && recovered.high->counts == 1280);
      CHECK(recovered.verifiedGeneration == previous.verifiedGeneration + 1);
    }
  }
}

void interruptedReadbackKeepsItsFirstCause() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  const auto verified = f.device.getSettingsSnapshot();
  f.bus.script.resize(6);  // Complete the SD write; its readback then fails.
  f.bus.script.push_back({Status::Error(Err::I2C_TIMEOUT, "SD readback failed")});
  CHECK(f.device.recover().is(Err::I2C_TIMEOUT));
  const auto interrupted = f.device.getSettingsSnapshot();
  CHECK(interrupted.trust == ConfigTrust::DIRTY && interrupted.dirtyReason == DirtyReason::INCOMPLETE_APPLY);
  CHECK(interrupted.affectedFields == static_cast<std::uint8_t>(SettingsField::CONFIG));
  CHECK(!interrupted.config && interrupted.verifiedAtMs == verified.verifiedAtMs);

  f.bus.script.resize(7);  // Next replay reaches its first limit write.
  f.bus.script.push_back({Status::Error(Err::I2C_NACK, "TLOW write failed")});
  CHECK(f.device.recover().is(Err::I2C_NACK));
  const auto twice = f.device.getSettingsSnapshot();
  CHECK(twice.dirtyReason == DirtyReason::INCOMPLETE_APPLY && !twice.low);
  CHECK(twice.affectedFields ==
        (static_cast<unsigned>(SettingsField::CONFIG) | static_cast<unsigned>(SettingsField::LOW_LIMIT)));
  CHECK(twice.verifiedGeneration == verified.verifiedGeneration && twice.verifiedAtMs == verified.verifiedAtMs);
}

void invalidationAfterExternalCorrection() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  const auto generation = f.device.getSettingsSnapshot().verifiedGeneration;
  const auto verification = f.device.getSettingsSnapshot().verifiedAtMs;
  Sample sample;
  f.bus.script.push_back({Status::Error(Err::I2C_TIMEOUT, "TEMP timeout", 17)});
  CHECK(f.device.readLatest(sample).is(Err::I2C_TIMEOUT));
  f.chip.words[4] = 0x8000;
  CHECK(f.device.recover().is(Err::EEPROM_UNSAFE));
  f.chip.words[4] = 0;  // The owner corrected EUN under restored stable power.
  f.bus.trace.clear();
  CHECK(f.device.recover().is(Err::EEPROM_UNSAFE) && f.bus.trace.empty());
  const auto health = f.device.healthSnapshot();
  const auto tick = f.tick;
  CHECK(f.device.invalidateDeviceState().ok() && f.device.isBound() && !f.device.isInitialized());
  const auto invalid = f.device.getSettingsSnapshot();
  CHECK(invalid.trust == ConfigTrust::UNKNOWN && !invalid.config && !invalid.low && !invalid.high);
  CHECK(invalid.verifiedGeneration == generation && invalid.verifiedAtMs == verification);
  CHECK(invalid.desired.limits.lowCounts == -256 && invalid.desired.limits.highCounts == 1280);
  CHECK(f.device.healthSnapshot().totalSuccess == health.totalSuccess);
  CHECK(f.device.healthSnapshot().lastSuccessMs == health.lastSuccessMs);
  CHECK(f.device.healthSnapshot().totalFailures == 1 && f.device.healthSnapshot().lastError.detail == 17);
  CHECK(f.device.healthSnapshot().lastErrorMs == health.lastErrorMs);
  CHECK(f.device.verifyConfiguration().is(Err::NOT_INITIALIZED));
  CHECK(f.bus.trace.empty() && f.tick == tick);
  CHECK(f.device.recover().ok());
  CHECK(f.bus.trace[0].tx[0] == 0x0F && f.bus.trace[1].tx[0] == 0x04);
  CHECK(f.guardTick - f.idTick >= 4);
  CHECK(f.device.getSettingsSnapshot().verifiedGeneration == generation + 1);
}

}  // namespace

int main() {
  admissionAndObservations();
  verifyLimitDrift();
  partialVerificationRetainsEvidence();
  ambiguousWritesKeepTheTarget();
  interruptedReadbackKeepsItsFirstCause();
  invalidationAfterExternalCorrection();
  return EXIT_SUCCESS;
}
