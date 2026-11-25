#include "Apply.h"
#include "LatestRead.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using ::TMP11x::detail::Apply;
using ::TMP11x::detail::LatestRead;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);      \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

struct Fixture {
  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t tick = 0;
  bool injectFinalFlags = false;
  bool changeFinalRouting = false;
  std::optional<std::size_t> expireOnRead;

  explicit Fixture(Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x4A;
    config.i2cUser = this;
    config.timeUser = this;
    config.nowMs = [](void* user) { return static_cast<Fixture*>(user)->tick++; };
    config.i2cWrite = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                         std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      return FakeTmp11xBus::write(address, tx, n, ms, &f.bus);
    };
    config.i2cWriteRead = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                             std::uint8_t* rx, std::size_t m, std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      if (f.injectFinalFlags && f.bus.trace.size() == 13) {
        CHECK(n == 1 && *tx == 0x01);
        f.chip.words[1] |= 0xA000;  // HIGH and READY published after returning to CC.
        if (f.changeFinalRouting) f.chip.words[1] ^= 0x0004;
      }
      if (f.expireOnRead == f.bus.trace.size()) f.tick += f.config.operationTimeoutMs;
      return FakeTmp11xBus::writeRead(address, tx, n, rx, m, ms, &f.bus);
    };
    config.settings.limits = {-320, 960};
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0xB117;
    CHECK(bus.attach(config.i2cAddress, chip));
  }
};

void checkSelectors(const Fixture& f, Mode mode, Cycle cycle, Averaging averaging) {
  const auto s = f.device.desiredSettings();
  CHECK(s.mode == mode && s.cycle == cycle && s.averaging == averaging);
  CHECK(s.polarity == Polarity::ACTIVE_HIGH && s.pinFunction == PinFunction::DATA_READY);
  CHECK(s.alertMode == AlertMode::THERM && s.limits.lowCounts == -320 && s.limits.highCounts == 960);
  const auto word = (static_cast<unsigned>(mode) << 10) | (static_cast<unsigned>(cycle) << 7) |
                    (static_cast<unsigned>(averaging) << 5) | 0x001C;
  CHECK(f.chip.words[1] == word && f.chip.words[2] == 960 && f.chip.words[3] == 0xFEC0);
  CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);
  CHECK(f.device.identityInfo().model == f.config.model && f.device.identityInfo().i2cAddress == 0x4A);
}

void allPublicProfiles() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117, Model::TMP117M,
                           Model::TMP117N, Model::TMP119}) {
    Fixture f{model};
    f.config.settings.alertMode = AlertMode::THERM;
    f.config.settings.polarity = Polarity::ACTIVE_HIGH;
    f.config.settings.pinFunction = PinFunction::DATA_READY;
    CHECK(f.device.begin(f.config).ok());
    auto cycle = f.config.settings.cycle;
    auto averaging = f.config.settings.averaging;
    for (const auto mode : {Mode::SHUTDOWN, Mode::CONTINUOUS}) {
      CHECK(f.device.setMode(mode).ok());
      checkSelectors(f, mode, cycle, averaging);
      for (unsigned c = 0; c < 8; ++c) {
        cycle = static_cast<Cycle>(c);
        CHECK(f.device.setCycle(cycle).ok());
        checkSelectors(f, mode, cycle, averaging);
        for (unsigned a = 0; a < 4; ++a) {
          averaging = static_cast<Averaging>(a);
          CHECK(f.device.setAveraging(averaging).ok());
          checkSelectors(f, mode, cycle, averaging);
          f.bus.trace.clear();
          ConfigurationObservation config;
          Sample sample;
          CHECK(f.device.readConfiguration(config).ok() && config.raw == f.chip.words[1]);
          CHECK(f.device.readLatest(sample).ok() && !sample.freshConversion);
          CHECK(f.bus.trace.size() == 2 && f.bus.trace[0].combinedRead && f.bus.trace[1].combinedRead);
          CHECK(f.bus.trace[0].tx[0] == 0x01 && f.bus.trace[1].tx[0] == 0x00);
          checkSelectors(f, mode, cycle, averaging);
        }
      }
    }
  }
}

void checkRefused(Fixture& f, Err error) {
  const auto tick = f.tick;
  const auto frames = f.bus.trace.size();
  CHECK(f.device.setMode(Mode::SHUTDOWN).is(error));
  CHECK(f.device.setCycle(Cycle::CODE_0).is(error));
  CHECK(f.device.setAveraging(Averaging::SAMPLES_64).is(error));
  CHECK(f.tick == tick && f.bus.trace.size() == frames);
}

void readinessAfterPreflightFailure() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  const auto before = f.device.getSettingsSnapshot();
  const Apply evidence{f.device};
  const auto baseline = evidence.baselineMs();
  CHECK(baseline && !evidence.readyPending());
  f.chip.words[1] |= 0xA000;  // A continuous result and HIGH before any mutation.
  f.bus.trace.clear();
  f.bus.script.resize(3);  // ID, EEPROM guard and matching CONFIG complete.
  f.bus.script.push_back({Status::Error(Err::I2C_TIMEOUT, "TLOW timed out")});
  CHECK(f.device.setCycle(Cycle::CODE_6).is(Err::I2C_TIMEOUT));
  CHECK(f.bus.trace.size() == 4 && f.bus.trace.back().tx[0] == 0x03);
  for (const auto& frame : f.bus.trace) CHECK(frame.combinedRead);
  const auto after = f.device.getSettingsSnapshot();
  CHECK(after.trust == ConfigTrust::VERIFIED && after.verifiedGeneration == before.verifiedGeneration);
  CHECK(after.desired.cycle == before.desired.cycle && after.config && after.config->raw == 0xA220);
  CHECK(f.chip.words[1] == 0x0220 && evidence.alerts().pendingHigh);
  CHECK(evidence.baselineMs() == baseline && evidence.readyPending());
  Sample latest;
  CHECK(f.device.readLatest(latest).ok() && !latest.freshConversion && !evidence.readyPending());

  // A late matching preflight retains its flags but supplies no new ready credit.
  f.bus.trace.clear();
  f.chip.words[1] |= 0x6000;  // LOW and READY
  f.expireOnRead = 2;
  CHECK(f.device.setCycle(Cycle::CODE_6).is(Err::OPERATION_TIMEOUT));
  const auto late = f.device.getSettingsSnapshot();
  CHECK(f.bus.trace.size() == 3 && late.config && late.config->raw == 0x6220);
  CHECK(late.verifiedGeneration == before.verifiedGeneration && late.trust == ConfigTrust::VERIFIED);
  CHECK(late.desired.cycle == before.desired.cycle && f.chip.words[1] == 0x0220);
  CHECK(evidence.alerts().pendingHigh && evidence.alerts().pendingLow);
  CHECK(evidence.baselineMs() == baseline && !evidence.readyPending());
}

void localRefusalsAndPreflightDrift() {
  Fixture f;
  checkRefused(f, Err::NOT_BOUND);
  CHECK(f.device.bind(f.config).ok());
  checkRefused(f, Err::NOT_INITIALIZED);
  CHECK(f.device.begin().ok());
  const auto verified = f.device.getSettingsSnapshot();
  const auto tick = f.tick;
  f.bus.trace.clear();
  CHECK(f.device.setMode(static_cast<Mode>(2)).is(Err::INVALID_CONFIG));
  CHECK(f.device.setMode(static_cast<Mode>(3)).is(Err::INVALID_CONFIG));
  CHECK(f.device.setCycle(static_cast<Cycle>(8)).is(Err::INVALID_CONFIG));
  CHECK(f.device.setAveraging(static_cast<Averaging>(4)).is(Err::INVALID_CONFIG));
  CHECK(f.tick == tick && f.bus.trace.empty() && f.chip.words[1] == 0x0220);
  CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);

  f.chip.words[1] ^= 0x0008;  // Previously unobserved polarity drift.
  CHECK(f.device.setMode(Mode::SHUTDOWN).is(Err::CONFIG_MISMATCH));
  CHECK(f.bus.trace.size() == 3 && f.bus.trace.back().combinedRead && f.bus.trace.back().tx[0] == 1);
  for (const auto& frame : f.bus.trace) CHECK(frame.combinedRead);
  const auto dirty = f.device.getSettingsSnapshot();
  CHECK(dirty.trust == ConfigTrust::DIRTY && dirty.dirtyReason == DirtyReason::READBACK_MISMATCH);
  CHECK(dirty.desired.mode == Mode::CONTINUOUS && dirty.desired.cycle == Cycle::CODE_4 &&
        dirty.desired.averaging == Averaging::SAMPLES_8);
  CHECK(dirty.verifiedGeneration == verified.verifiedGeneration && dirty.verifiedAtMs == verified.verifiedAtMs);
  checkRefused(f, Err::CONFIG_DIRTY);
  CHECK(f.device.recover().ok() && f.chip.words[1] == 0x0220);
  CHECK(f.device.invalidateDeviceState().ok());
  checkRefused(f, Err::NOT_INITIALIZED);
  CHECK(f.device.recover().ok() && f.device.setMode(Mode::SHUTDOWN).ok());
}

// Every register callback for a simple ALERT-mode change, in wire order.
void checkBoundary(const Fixture& f, std::uint16_t cc, std::uint16_t low) {
  const auto write = [](std::uint8_t pointer, std::uint16_t word) {
    return std::vector<std::uint8_t>{pointer, static_cast<std::uint8_t>(word >> 8),
                                     static_cast<std::uint8_t>(word)};
  };
  const std::vector<std::vector<std::uint8_t>> expected{
      {0x0F}, {0x04}, {0x01}, {0x03}, {0x02}, write(0x01, cc | 0x0400), {0x01},
      write(0x03, low), {0x03}, write(0x02, 960), {0x02}, {0x01}, write(0x01, cc), {0x01}};
  CHECK(f.bus.trace.size() == expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    const auto& frame = f.bus.trace[i];
    CHECK(frame.address == 0x4A && frame.tx == expected[i]);
    CHECK(frame.combinedRead == (expected[i].size() == 1));
  }
}

enum class Change { CYCLE, AVERAGING, LIMIT, ROUTING };

void generationChanges() {
  for (const auto change : {Change::CYCLE, Change::AVERAGING, Change::LIMIT, Change::ROUTING}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    f.chip.words[0] = 0x0600;
    Sample old;
    CHECK(f.device.readLatest(old).ok());
    f.chip.words[1] |= 0x2000;
    ConfigurationObservation observed;
    CHECK(f.device.readConfiguration(observed).ok() && observed.dataReady);
    Apply evidence{f.device};
    CHECK(evidence.readyPending() && evidence.baselineMs());
    f.bus.trace.clear();
    f.injectFinalFlags = true;
    auto cc = std::uint16_t{0x0220};
    auto low = std::uint16_t{0xFEC0};
    if (change == Change::CYCLE) {
      cc = 0x0320;
      CHECK(f.device.setCycle(Cycle::CODE_6).ok());
    } else if (change == Change::AVERAGING) {
      cc = 0x0260;
      CHECK(f.device.setAveraging(Averaging::SAMPLES_64).ok());
    } else {
      // The public limit and routing setters are owned by stage 22.
      auto candidate = f.device.desiredSettings();
      if (change == Change::LIMIT) {
        candidate.limits.lowCounts = -256;
        low = 0xFF00;
      } else {
        candidate.pinFunction = PinFunction::DATA_READY;
        cc = 0x0224;
      }
      Apply apply{f.device, candidate};
      auto status = apply.start();
      while (status.inProgress()) {
        status = apply.step();
        if (f.bus.trace.size() >= 6 && status.inProgress()) {
          CHECK(f.device.configTrust() == ConfigTrust::DIRTY && !apply.readyPending() && !apply.baselineMs());
          CHECK(f.device.getSettingsSnapshot().verifiedGeneration == old.profileGeneration);
        }
      }
      CHECK(status.ok());
    }
    checkBoundary(f, cc, low);
    const auto snapshot = f.device.getSettingsSnapshot();
    CHECK(snapshot.trust == ConfigTrust::VERIFIED && snapshot.verifiedGeneration == old.profileGeneration + 1);
    CHECK(snapshot.config && snapshot.config->raw == (cc | 0xA000) && snapshot.config->timestampValid);
    CHECK(evidence.baselineMs() == snapshot.config->observedAtMs && !evidence.readyPending());
    CHECK(evidence.alerts().pendingHigh && !evidence.alerts().pendingLow && f.chip.words[1] == cc);
    const auto& cached = LatestRead{f.device}.cache;
    CHECK(cached && cached->profileGeneration == old.profileGeneration && cached->raw == old.raw);
    CHECK(!cached->freshConversion);
  }
}

void incompleteChangesKeepIntent() {
  for (const auto fault : {Err::I2C_NACK_DATA, Err::CONFIG_MISMATCH, Err::OPERATION_TIMEOUT}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    const auto verified = f.device.getSettingsSnapshot();
    f.bus.trace.clear();
    if (fault == Err::I2C_NACK_DATA) {
      f.bus.script.resize(5);
      TransferOutcome lostAck;
      lostAck.status = Status::Error(fault, "shutdown ACK lost");
      lostAck.failedWriteWord = 0x0660;
      f.bus.script.push_back(lostAck);
    } else {
      f.injectFinalFlags = true;
      f.changeFinalRouting = fault == Err::CONFIG_MISMATCH;
      if (fault == Err::OPERATION_TIMEOUT) f.expireOnRead = 13;
    }
    CHECK(f.device.setAveraging(Averaging::SAMPLES_64).is(fault));
    const auto failed = f.device.getSettingsSnapshot();
    const Apply evidence{f.device};
    CHECK(failed.trust == ConfigTrust::DIRTY && failed.desired.averaging == Averaging::SAMPLES_64);
    CHECK(failed.verifiedGeneration == verified.verifiedGeneration && failed.verifiedAtMs == verified.verifiedAtMs);
    CHECK(!evidence.baselineMs() && !evidence.readyPending());
    if (fault == Err::I2C_NACK_DATA) {
      CHECK(f.bus.trace.size() == 6 && f.chip.words[1] == 0x0660);
      CHECK(failed.dirtyReason == DirtyReason::AMBIGUOUS_WRITE);
    } else {
      CHECK(f.bus.trace.size() == 14 && evidence.alerts().pendingHigh);
      CHECK(failed.config && failed.config->dataReady && failed.config->high);
    }
    checkRefused(f, Err::CONFIG_DIRTY);
    f.injectFinalFlags = false;
    f.expireOnRead.reset();
    CHECK(f.device.recover().ok() && f.chip.words[1] == 0x0260);
    CHECK(f.device.getSettingsSnapshot().verifiedGeneration == verified.verifiedGeneration + 1);
  }
}

void ambiguousShutdownDuringBlock() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  CHECK(f.chip.addContinuousConversion(80));
  const auto previousTemp = f.chip.words[0];
  f.bus.trace.clear();
  f.bus.script.resize(5);  // Stop at the first mutating callback.
  TransferOutcome partial;
  partial.status = Status::Error(Err::I2C_TIMEOUT, "shutdown write timed out", -31);
  partial.failedWriteWord = 0x0120;  // Script a changed cycle with the device still in CC.
  f.bus.script.push_back(partial);
  const auto status = f.device.setCycle(Cycle::CODE_6);
  CHECK(status.is(Err::I2C_TIMEOUT) && status.detail == -31);
  CHECK(f.bus.trace.size() == 6 && f.chip.words[1] == 0x0120);
  CHECK(f.device.configTrust() == ConfigTrust::DIRTY && f.device.desiredSettings().cycle == Cycle::CODE_6);
  CHECK(!f.chip.addContinuousConversion(8));  // Unspecified accumulator after a mid-block CONFIG write.
  CHECK(f.device.recover().ok() && f.chip.words[1] == 0x0320);
  for (unsigned i = 1; i < 8; ++i) {
    CHECK(f.chip.addContinuousConversion(8));
    CHECK(f.chip.words[0] == previousTemp && !(f.chip.words[1] & 0x2000));
  }
  CHECK(f.chip.addContinuousConversion(8));
  CHECK(f.chip.words[0] == 8 && (f.chip.words[1] & 0x2000));
}

}  // namespace

int main() {
  allPublicProfiles();
  localRefusalsAndPreflightDrift();
  readinessAfterPreflightFailure();
  ambiguousShutdownDuringBlock();
  generationChanges();
  incompleteChangesKeepIntent();
  return EXIT_SUCCESS;
}
