#include "Acquisition.h"
#include "Apply.h"
#include "LatestRead.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <vector>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using ::TMP11x::detail::Acquisition;
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

constexpr Model models[]{Model::TMP116, Model::TMP116N, Model::TMP117,
                         Model::TMP117M, Model::TMP117N, Model::TMP119};
constexpr unsigned lengths[]{1, 8, 32, 64};
constexpr Sample sentinel{0x1234, 4660, 36.40625, 47, true, ReadKind::LATEST, false,
                          19, ConfigTrust::DIRTY, {OffsetState::UNKNOWN, 0, false}};

struct Fixture {
  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t tick = 0;
  std::uint32_t lastTick = 0;
  unsigned clockReads = 0;
  unsigned readsPerTick = 4;  // Repeated readings at one floor tick are ordinary.
  std::function<void(std::size_t)> beforeIo;
  std::vector<std::uint32_t> dispatchTicks;

  explicit Fixture(Mode mode = Mode::CONTINUOUS, Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.settings.mode = mode;
    config.i2cAddress = 0x49;
    config.i2cUser = this;
    config.timeUser = this;
    config.nowMs = [](void* user) {
      auto& f = *static_cast<Fixture*>(user);
      f.lastTick = f.tick;
      ++f.clockReads;
      if (f.readsPerTick && f.clockReads % f.readsPerTick == 0) f.tick += f.config.clockResolutionMs;
      return f.lastTick;
    };
    config.i2cWrite = [](std::uint8_t addr, const std::uint8_t* tx, std::size_t n,
                         std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      f.onIo();
      return FakeTmp11xBus::write(addr, tx, n, ms, &f.bus);
    };
    config.i2cWriteRead = [](std::uint8_t addr, const std::uint8_t* tx, std::size_t n,
                             std::uint8_t* rx, std::size_t m, std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      f.onIo();
      return FakeTmp11xBus::writeRead(addr, tx, n, rx, m, ms, &f.bus);
    };
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0xA117;
    chip.words[0] = 0x8000;
    CHECK(bus.attach(config.i2cAddress, chip));
  }

  void onIo() {
    dispatchTicks.push_back(lastTick);
    if (beforeIo) beforeIo(bus.trace.size());
  }

  void clearTrace() {
    bus.trace.clear();
    dispatchTicks.clear();
  }

  void begin() {
    CHECK(device.begin(config).ok());
    clearTrace();
  }

  void failAt(unsigned index, bool consume = false) {
    bus.script.resize(index);
    bus.script.push_back({Status::Error(Err::I2C_TIMEOUT, "scripted callback failure", -7), 1, consume});
  }

  // Negative values denote writes to the absolute register pointer.
  void checkTrace(std::initializer_list<int> expected) const {
    CHECK(bus.trace.size() == expected.size());
    std::size_t index = 0;
    for (int pointer : expected) {
      const auto& frame = bus.trace[index++];
      CHECK(frame.combinedRead == (pointer >= 0));
      CHECK(frame.tx.size() == (pointer >= 0 ? 1u : 3u) && frame.tx[0] == std::abs(pointer));
      CHECK(frame.rxLen == (pointer >= 0 ? 2u : 0u) && frame.address == config.i2cAddress);
      CHECK(frame.timeoutMs > 0 && frame.timeoutMs <= config.i2cTimeoutMs);
    }
  }
};

void checkFresh(const Fixture& f, const Sample& out, ReadKind kind) {
  CHECK(out.raw == 0x0C80 && out.counts == 3200 && out.celsius == 25.0);
  CHECK(out.timestampValid && out.freshConversion && out.acquisitionKind == kind);
  CHECK(out.configTrust == ConfigTrust::VERIFIED);
  CHECK(out.profileGeneration == f.device.getSettingsSnapshot().verifiedGeneration);
  CHECK(out.offset.state == OffsetState::UNKNOWN && !out.offset.countsValid);
}

void admissions() {
  Fixture f;
  auto out = sentinel;
  CHECK(f.device.tryReadFresh(out).is(Err::NOT_BOUND));
  CHECK(f.device.readOneShotBlocking(out, 100).is(Err::NOT_BOUND));
  CHECK(f.device.bind(f.config).ok());
  CHECK(f.device.readFreshBlocking(out, 100).is(Err::NOT_INITIALIZED));
  CHECK(f.device.tryReadFresh(out).is(Err::NOT_INITIALIZED));
  f.checkTrace({});
  f.config.clockResolutionMs = 4;
  f.begin();
  CHECK(f.device.readOneShotBlocking(out, 100).is(Err::INVALID_CONFIG));
  for (auto ms : {0u, 3u, 0x80000000u}) CHECK(f.device.readFreshBlocking(out, ms).is(Err::INVALID_PARAM));
  CHECK(out.raw == sentinel.raw);
  f.checkTrace({});
  CHECK(f.device.setMode(Mode::SHUTDOWN).ok());
  f.clearTrace();
  CHECK(f.device.readFreshBlocking(out, 100).is(Err::INVALID_CONFIG));
  CHECK(f.device.tryReadFresh(out).is(Err::INVALID_CONFIG));
  CHECK(f.device.invalidateDeviceState().ok());
  CHECK(f.device.readOneShotBlocking(out, 100).is(Err::NOT_INITIALIZED));
  f.checkTrace({});
  f.config.nowMs = nullptr;
  CHECK(f.device.bind(f.config).ok());
  CHECK(f.device.readOneShotBlocking(out, 100).is(Err::CLOCK_REQUIRED));
  f.checkTrace({});
}

void allContinuousProfiles() {
  for (const auto model : models) {
    for (unsigned c = 0; c < 8; ++c) {
      for (unsigned a = 0; a < 4; ++a) {
        Fixture f{Mode::CONTINUOUS, model};
        f.config.settings.cycle = static_cast<Cycle>(c);
        f.config.settings.averaging = static_cast<Averaging>(a);
        f.begin();
        Sample out;
        CHECK(f.device.readLatest(out).ok() && out.raw == 0x8000 && !out.freshConversion);
        for (unsigned i = 1; i < lengths[a]; ++i) CHECK(f.chip.addContinuousConversion(3200));
        for (unsigned block = 0; block < 2; ++block) {
          f.clearTrace();
          CHECK(f.device.tryReadFresh(out).is(Err::MEASUREMENT_NOT_READY));
          f.checkTrace({4, 1});
          const unsigned count = block == 0 ? 1 : lengths[a];
          for (unsigned i = 0; i < count; ++i) CHECK(f.chip.addContinuousConversion(3200));
          f.clearTrace();
          CHECK(f.device.tryReadFresh(out).ok());
          checkFresh(f, out, ReadKind::CONTINUOUS_FRESH);
          f.checkTrace({4, 1, 0});
        }
      }
    }
  }
}

void continuousGuardAndReadEffects() {
  Fixture f;
  f.begin();
  Apply evidence{f.device};
  auto out = sentinel;
  f.chip.words[4] = 0x4000;
  CHECK(f.device.tryReadFresh(out).is(Err::MEASUREMENT_NOT_READY));
  f.checkTrace({4});
  f.chip.words[4] = 0;
  f.clearTrace();
  f.chip.words[1] = 0xB220;  // HIGH, READY and BUSY, matching stable fields.
  CHECK(f.device.tryReadFresh(out).is(Err::MEASUREMENT_NOT_READY));
  f.checkTrace({4, 1});
  CHECK(evidence.alerts().pendingHigh && !evidence.readyPending());
  f.chip.words[1] = 0x6220;
  f.chip.words[0] = 0x0C80;
  ConfigurationObservation observed;
  CHECK(f.device.readConfiguration(observed).ok() && evidence.readyPending());
  f.clearTrace();
  CHECK(f.device.tryReadFresh(out).ok());
  f.checkTrace({4, 0});
  checkFresh(f, out, ReadKind::CONTINUOUS_FRESH);
  CHECK(evidence.alerts().pendingHigh && evidence.alerts().pendingLow && !evidence.readyPending());

  for (const bool failConfig : {true, false}) {
    f.clearTrace();
    f.chip.words[1] = 0x2220;
    f.failAt(failConfig ? 1 : 2, true);
    const auto failure = f.device.tryReadFresh(out);
    CHECK(failure.is(Err::I2C_TIMEOUT) && failure.detail == -7);
    CHECK(evidence.readyHistoryUncertain() && !evidence.readyPending() && !evidence.baselineMs());
    CHECK(evidence.alerts().pendingHigh && evidence.alerts().pendingLow);
    f.clearTrace();
    f.chip.words[1] = 0x2220;
    CHECK(f.device.tryReadFresh(out).is(Err::MEASUREMENT_NOT_READY));  // First observation is baseline only.
    f.checkTrace({4, 1});
    CHECK(evidence.baselineMs() && !evidence.readyPending());
  }
  CHECK(evidence.alerts().possibleLoss);
  for (const auto raw : {0xA240, 0xB240}) {
    f.clearTrace();
    f.chip.words[1] = static_cast<std::uint16_t>(raw);
    CHECK(f.device.tryReadFresh(out).is(Err::CONFIG_MISMATCH));
    f.checkTrace({4, 1});
    CHECK(f.device.configTrust() == ConfigTrust::DIRTY && evidence.alerts().pendingHigh);
    CHECK(f.device.tryReadFresh(out).is(Err::CONFIG_DIRTY));
    f.checkTrace({4, 1});
    f.chip.words[1] = 0x0220;
    CHECK(f.device.recover().ok());
  }
}

void guardDenialsAndLimits() {
  for (const auto raw : {0x8000, 0xC000}) {
    Fixture f{Mode::SHUTDOWN};
    f.begin();
    f.chip.words[4] = static_cast<std::uint16_t>(raw);
    auto out = sentinel;
    CHECK(f.device.readOneShotBlocking(out, 500).is(Err::EEPROM_UNSAFE));
    f.chip.words[4] = 0;
    CHECK(f.device.readOneShotBlocking(out, 500).is(Err::EEPROM_UNSAFE));
    f.checkTrace({4});
    CHECK(out.raw == sentinel.raw);
  }

  for (const bool frozen : {false, true}) {
    Fixture f;
    f.begin();
    f.chip.words[4] = 0x4000;
    if (frozen) f.readsPerTick = 0;
    const unsigned initialReads = f.clockReads;
    auto out = sentinel;
    CHECK(f.device.readFreshBlocking(out, 5000).is(Err::WORK_LIMIT));
    CHECK(out.raw == sentinel.raw);
    if (frozen) {
      f.checkTrace({4});
      CHECK(f.clockReads - initialReads >= 1000001u);
    } else {
      CHECK(f.bus.trace.size() == 256);
      for (const auto& frame : f.bus.trace) CHECK(frame.combinedRead && frame.tx[0] == 4);
    }
  }
  Fixture f{Mode::SHUTDOWN};
  f.begin();
  f.chip.words[4] = 0x4000;
  auto out = sentinel;
  CHECK(f.device.readOneShotBlocking(out, 6).is(Err::OPERATION_TIMEOUT));
  CHECK(!f.bus.trace.empty());
  for (const auto& frame : f.bus.trace) CHECK(frame.combinedRead && frame.tx[0] == 4);
  f.clearTrace();
  f.failAt(0);
  CHECK(f.device.readOneShotBlocking(out, 500).is(Err::I2C_TIMEOUT));
  f.checkTrace({4});
}

void oneShotWireAndTiming() {
  for (const bool reportShutdown : {false, true}) {
    for (unsigned a = 0; a < 4; ++a) {
      for (const auto c : {Cycle::CODE_0, Cycle::CODE_4, Cycle::CODE_7}) {
        Fixture f{Mode::SHUTDOWN};
        f.config.settings.cycle = c;
        f.config.settings.averaging = static_cast<Averaging>(a);
        f.config.settings.limits.highCounts = 3199;
        f.begin();
        f.chip.reportShutdownForOneShot = reportShutdown;
        f.chip.words[1] |= 0x2000;  // Stale READY, before the owned SD boundary.
        const auto stable = f.chip.words[1] & 0x0FFF;
        f.readsPerTick = 0;
        f.tick = 100;
        f.beforeIo = [&](std::size_t index) {
          if (index == 4) {
            f.tick = 104;  // Unknown sub-tick phase at physical trigger completion.
            f.clockReads = 0;
            f.readsPerTick = 4;
          } else if (index == 5) {
            for (unsigned i = 0; i < lengths[a]; ++i) CHECK(f.chip.addOneShotConversion(3200));
          }
        };
        auto out = sentinel;
        CHECK(f.device.readOneShotBlocking(out, 1500).ok());
        f.checkTrace({4, 1, -1, 1, -1, 1, 0});
        const auto word = [](const std::vector<std::uint8_t>& bytes) {
          return (bytes[bytes.size() - 2] << 8) | bytes.back();
        };
        CHECK(word(f.bus.trace[0].rx) == 0);
        CHECK(word(f.bus.trace[1].rx) == (stable | 0x2000));
        CHECK(word(f.bus.trace[2].tx) == stable && word(f.bus.trace[3].rx) == stable);
        CHECK(word(f.bus.trace[4].tx) == (stable | 0x0800));
        CHECK(word(f.bus.trace[5].rx) == (stable | (reportShutdown ? 0xA000 : 0xA800)));
        CHECK(word(f.bus.trace[6].rx) == 0x0C80);
        constexpr unsigned firstPollTicks[]{123, 245, 665, 1225};
        CHECK(f.dispatchTicks[5] == firstPollTicks[a]);
        checkFresh(f, out, ReadKind::ONE_SHOT);
        CHECK(Apply{f.device}.alerts().pendingHigh && f.device.desiredSettings().mode == Mode::SHUTDOWN);
        CHECK(LatestRead{f.device}.cache->acquisitionKind == ReadKind::ONE_SHOT);
      }
    }
  }
}

void oneShotConfigFailures() {
  for (const auto raw : {0x0640, 0x0E20, 0xB640, 0xAE40}) {
    Fixture f{Mode::SHUTDOWN};
    f.begin();
    const bool atPoll = raw == 0xAE40;
    f.beforeIo = [&](std::size_t index) {
      if (index == (atPoll ? 5u : 1u)) f.chip.words[1] = static_cast<std::uint16_t>(raw);
    };
    auto out = sentinel;
    CHECK(f.device.readOneShotBlocking(out, 500).is(Err::CONFIG_MISMATCH));
    if (atPoll) f.checkTrace({4, 1, -1, 1, -1, 1});
    else f.checkTrace({4, 1});
    CHECK(f.device.configTrust() == ConfigTrust::DIRTY && out.raw == sentinel.raw);
    CHECK(Apply{f.device}.alerts().pendingHigh == ((raw & 0x8000) != 0));
  }
  for (const unsigned index : {2u, 4u}) {
    Fixture f{Mode::SHUTDOWN};
    f.begin();
    f.failAt(index);
    auto out = sentinel;
    CHECK(f.device.readOneShotBlocking(out, 500).is(Err::I2C_TIMEOUT));
    CHECK(f.bus.trace.size() == index + 1 && f.device.configTrust() == ConfigTrust::DIRTY);
    CHECK(f.device.getSettingsSnapshot().dirtyReason == DirtyReason::AMBIGUOUS_WRITE);
    CHECK(out.raw == sentinel.raw && !LatestRead{f.device}.cache);
  }
}

void renewedGuards() {
  Fixture f{Mode::SHUTDOWN};
  f.begin();
  f.chip.words[4] = 0x4000;
  f.beforeIo = [&](std::size_t index) {
    if (index == 1) f.chip.words[4] = 0;
    if (index == 2) f.chip.words[1] = 0xB620;  // Matching CONFIG revokes the new guard.
    if (index == 3) f.chip.words[1] = 0x0620;
    if (index == 8) {
      f.chip.words[1] |= 0x2000;
      f.chip.words[0] = 0x0C80;
    }
  };
  Sample out;
  CHECK(f.device.readOneShotBlocking(out, 500).ok());
  f.checkTrace({4, 4, 1, 4, 1, -1, 1, -1, 1, 0});
  CHECK(f.dispatchTicks[1] - f.dispatchTicks[0] >= 2);
  CHECK(f.dispatchTicks[3] - f.dispatchTicks[2] >= 2);
  CHECK(Apply{f.device}.alerts().pendingHigh);
  checkFresh(f, out, ReadKind::ONE_SHOT);
}

void lateContinuousTransfers() {
  for (const unsigned lateIndex : {1u, 2u}) {
    Fixture f;
    f.begin();
    Sample prior;
    CHECK(f.device.readLatest(prior).ok() && prior.raw == 0x8000);
    f.clearTrace();
    f.readsPerTick = 0;
    f.tick = 2000;  // The first continuous poll is already overdue.
    f.chip.words[1] = 0xA220;
    f.chip.words[0] = 0x0C80;
    f.beforeIo = [&](std::size_t index) {
      if (index == 0) f.tick = 2002;
      if (index == lateIndex) f.tick = 2004;
    };
    const auto before = f.device.healthSnapshot();
    auto out = sentinel;
    CHECK(f.device.readFreshBlocking(out, 4).is(Err::OPERATION_TIMEOUT));
    if (lateIndex == 1) f.checkTrace({4, 1});
    else f.checkTrace({4, 1, 0});
    CHECK(f.bus.trace.front().timeoutMs == 4 && f.bus.trace.back().timeoutMs == 2);
    CHECK(f.device.healthSnapshot().totalSuccess == before.totalSuccess + lateIndex + 1);
    CHECK(f.device.healthSnapshot().lastSuccessMs == 2004 && f.chip.words[1] == 0x0220);
    CHECK(Apply{f.device}.alerts().pendingHigh && !Apply{f.device}.readyPending());
    CHECK(out.raw == sentinel.raw && LatestRead{f.device}.cache->raw == prior.raw);
    CHECK(LatestRead{f.device}.cache->observedAtMs == prior.observedAtMs);
  }
}

void continuousPollingAcrossWrap() {
  Fixture f{Mode::CONTINUOUS, Model::TMP116};
  f.config.settings.cycle = Cycle::CODE_0;
  f.config.settings.averaging = Averaging::SAMPLES_1;
  f.config.clockResolutionMs = 2;
  f.tick = 0xFFFFFFE0u;
  f.begin();
  const auto baseline = Apply{f.device}.baselineMs().value();
  f.readsPerTick = 0;
  f.tick = baseline;
  f.beforeIo = [&](std::size_t index) {
    if (index == 0) f.tick += 22;  // Bus released; initial poll now overdue.
    if (index == 1) {
      f.tick += 2;  // Completion determines the next due tick.
      f.readsPerTick = 8;
      f.clockReads = 0;
    }
    if (index == 2) CHECK(f.chip.addContinuousConversion(3200));
  };
  Sample out;
  CHECK(f.device.readFreshBlocking(out, 50).ok());
  f.checkTrace({4, 1, 1, 0});
  CHECK(f.dispatchTicks[1] == baseline + 22);
  CHECK(f.dispatchTicks[2] == baseline + 24 + 18);  // q + ceil(15.5/q) * q.
  CHECK(f.dispatchTicks[2] < baseline);  // Wrap occurred.
  checkFresh(f, out, ReadKind::CONTINUOUS_FRESH);
}

void shortOneShotBudget() {
  Fixture f{Mode::SHUTDOWN, Model::TMP119};
  f.config.clockResolutionMs = 4;
  f.config.settings.averaging = Averaging::SAMPLES_64;
  f.config.settings.cycle = Cycle::CODE_0;
  f.begin();
  f.tick = 100;
  f.readsPerTick = 0;
  f.beforeIo = [&](std::size_t index) {
    if (index == 4) f.tick = 112;
  };
  auto out = sentinel;
  CHECK(f.device.readOneShotBlocking(out, 1120).is(Err::OPERATION_TIMEOUT));
  f.checkTrace({4, 1, -1, 1, -1});
  CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);
  CHECK(out.raw == sentinel.raw && !LatestRead{f.device}.cache);
}

// A cooperative owner can stop driving the same steps used by the blocking API.
Status stepOnce(Fixture& f, Acquisition& acquisition, Sample& out) {
  const auto before = f.bus.trace.size();
  const auto status = acquisition.step(out);
  CHECK(f.bus.trace.size() - before <= 1);
  return status;
}

void residualOwnedOneShot() {
  for (const bool lateTrigger : {false, true}) {
    Fixture f{Mode::SHUTDOWN};
    f.begin();
    f.tick = 100;
    f.readsPerTick = 0;
    auto out = sentinel;
    if (lateTrigger) {
      f.beforeIo = [&](std::size_t index) {
        if (index == 4) f.tick = 120;
      };
      CHECK(f.device.readOneShotBlocking(out, 20).is(Err::OPERATION_TIMEOUT));
    } else {
      Acquisition shot{f.device, ReadKind::ONE_SHOT};
      CHECK(shot.start(500).inProgress());
      f.checkTrace({});
      while (f.bus.trace.size() < 5) CHECK(stepOnce(f, shot, out).inProgress());
    }
    f.checkTrace({4, 1, -1, 1, -1});
    CHECK(f.chip.addOneShotConversion(99));  // Residual partial AVG=8 block.
    CHECK(f.chip.words[1] == 0x0E20 && out.raw == sentinel.raw);
    CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);
    f.clearTrace();
    f.readsPerTick = 4;
    f.beforeIo = [&](std::size_t index) {
      if (index == 5) {
        CHECK(f.chip.words[0] == 0x8000);
        for (unsigned i = 0; i < 8; ++i) CHECK(f.chip.addOneShotConversion(3200));
      }
    };
    CHECK(f.device.readOneShotBlocking(out, 500).ok());
    f.checkTrace({4, 1, -1, 1, -1, 1, 0});
    checkFresh(f, out, ReadKind::ONE_SHOT);
  }
}

void destructiveReadDuringShot() {
  // Insert the observation between serialized owner steps, never inside a callback.
  for (const unsigned diagnostic : {0u, 1u, 2u}) {
    Fixture f{Mode::SHUTDOWN};
    f.begin();
    auto out = sentinel;
    Acquisition shot{f.device, ReadKind::ONE_SHOT};
    CHECK(shot.start(180).inProgress());
    f.beforeIo = [&](std::size_t index) {
      if (index == 5) {
        for (unsigned i = 0; i < 8; ++i) CHECK(f.chip.addOneShotConversion(3200));
        f.chip.words[1] |= 0x8000;
      }
    };
    while (f.bus.trace.size() < 6) CHECK(stepOnce(f, shot, out).inProgress());
    f.checkTrace({4, 1, -1, 1, -1, 1});
    CHECK(Apply{f.device}.alerts().pendingHigh);
    if (diagnostic != 0) f.failAt(0, true);
    if (diagnostic == 2) {
      ConfigurationObservation observed;
      CHECK(f.device.readConfiguration(observed).is(Err::I2C_TIMEOUT));
    } else {
      Sample latest;
      const auto read = f.device.readLatest(latest);
      CHECK(read.is(diagnostic == 0 ? Err::OK : Err::I2C_TIMEOUT));
      if (read.ok()) CHECK(!latest.freshConversion && latest.raw == 0x0C80);
    }
    const auto diagnosticEnd = f.bus.trace.size();
    Status status;
    do { status = stepOnce(f, shot, out); } while (status.inProgress());
    CHECK(status.is(Err::MEASUREMENT_NOT_READY) || status.is(Err::OPERATION_TIMEOUT));
    for (auto i = diagnosticEnd; i < f.bus.trace.size(); ++i) {
      CHECK(f.bus.trace[i].combinedRead && f.bus.trace[i].tx[0] != 0);
    }
    CHECK(out.raw == sentinel.raw && Apply{f.device}.alerts().pendingHigh);
    CHECK(Apply{f.device}.alerts().possibleLoss == (diagnostic == 2));
    CHECK(Apply{f.device}.readyHistoryUncertain() == (diagnostic != 0));
  }
}

void missingReadyAndThermLevel() {
  Fixture f{Mode::SHUTDOWN};
  f.config.settings.alertMode = AlertMode::THERM;
  f.begin();
  // New block exists, but another observer acknowledged DATA_READY before the owner polled.
  f.beforeIo = [&](std::size_t index) {
    if (index == 5) {
      f.chip.words[0] = 0x0C80;
      f.chip.words[1] = 0x8E30;
    }
  };
  auto out = sentinel;
  CHECK(f.device.readOneShotBlocking(out, 164).is(Err::OPERATION_TIMEOUT));
  CHECK(f.bus.trace.size() >= 7 && out.raw == sentinel.raw && !LatestRead{f.device}.cache);
  for (std::size_t i = 5; i < f.bus.trace.size(); ++i) {
    CHECK(f.bus.trace[i].combinedRead && f.bus.trace[i].tx[0] == 1);
    if (i > 5) CHECK(f.dispatchTicks[i] - f.dispatchTicks[i - 1] >= 6);
  }
  const Apply evidence{f.device};
  CHECK(evidence.alerts().thermHigh.value() && !evidence.alerts().pendingHigh);
  ConfigurationObservation obs;
  f.chip.words[1] = 0x0630;
  CHECK(f.device.readConfiguration(obs).ok() && !evidence.alerts().thermHigh.value());
  CHECK(f.device.readLatest(out).ok() && out.raw == 0x0C80 && !out.freshConversion);
}

void diagnosticCapturesOwnedShot() {
  Fixture f{Mode::SHUTDOWN};
  f.config.settings.limits.highCounts = 3199;
  f.begin();
  auto out = sentinel;
  Acquisition shot{f.device, ReadKind::ONE_SHOT};
  CHECK(shot.start(300).inProgress());
  while (f.bus.trace.size() < 5) CHECK(stepOnce(f, shot, out).inProgress());
  for (unsigned i = 0; i < 8; ++i) CHECK(f.chip.addOneShotConversion(3200));
  ConfigurationObservation config;
  CHECK(f.device.readConfiguration(config).ok() && config.dataReady && config.high);
  const Apply evidence{f.device};
  CHECK(evidence.readyPending() && evidence.alerts().source == ::TMP11x::detail::ConfigReadSource::OBSERVATION);
  Status status;
  do { status = stepOnce(f, shot, out); } while (status.inProgress());
  CHECK(status.ok());
  f.checkTrace({4, 1, -1, 1, -1, 1, 1, 0});
  CHECK(f.bus.trace[5].rx == std::vector<std::uint8_t>({0xAE, 0x20}));
  CHECK(f.bus.trace[6].rx == std::vector<std::uint8_t>({0x0E, 0x20}));
  checkFresh(f, out, ReadKind::ONE_SHOT);
  CHECK(!evidence.readyPending() && evidence.alerts().pendingHigh);
  f.clearTrace();
  out = sentinel;
  CHECK(f.device.readOneShotBlocking(out, 150).is(Err::OPERATION_TIMEOUT));
  CHECK(out.raw == sentinel.raw && f.bus.trace.size() >= 6);
  for (std::size_t i = 5; i < f.bus.trace.size(); ++i) {
    CHECK(f.bus.trace[i].combinedRead && f.bus.trace[i].tx[0] == 1);
  }
}

void busyOneShotPoll() {
  for (const bool laterReady : {false, true}) {
    Fixture f{Mode::SHUTDOWN};
    f.begin();
    f.beforeIo = [&](std::size_t index) {
      if (index == 5) f.chip.words[1] = 0xBE20;  // HIGH and READY must not authorize TEMP with BUSY.
      if (index == 6) f.chip.words[1] = 0x0E20;
      if (index == 7 && laterReady) {
        for (unsigned i = 0; i < 8; ++i) CHECK(f.chip.addOneShotConversion(3200));
      }
    };
    auto out = sentinel;
    const auto status = f.device.readOneShotBlocking(out, 156);
    CHECK(f.dispatchTicks.size() >= 8);
    CHECK(f.dispatchTicks[6] - f.dispatchTicks[5] >= 2 && f.dispatchTicks[7] - f.dispatchTicks[5] >= 6);
    CHECK(Apply{f.device}.alerts().pendingHigh);
    if (laterReady) {
      CHECK(status.ok());
      f.checkTrace({4, 1, -1, 1, -1, 1, 4, 1, 0});
      checkFresh(f, out, ReadKind::ONE_SHOT);
    } else {
      CHECK(status.is(Err::OPERATION_TIMEOUT) && out.raw == sentinel.raw);
      CHECK(f.bus.trace[6].combinedRead && f.bus.trace[6].tx[0] == 4);
      for (std::size_t i = 7; i < f.bus.trace.size(); ++i) {
        CHECK(f.bus.trace[i].combinedRead && f.bus.trace[i].tx[0] == 1);
      }
    }
  }
}

void continuousUpdateBetweenReads() {
  Fixture f;
  f.begin();
  for (unsigned i = 0; i < 8; ++i) CHECK(f.chip.addContinuousConversion(100));
  f.beforeIo = [&](std::size_t index) {
    if (index == 2) {
      // A second full block arrives after CONFIG, before the combined TEMP transfer.
      // This interleaving makes no assumption about byte latching during TEMP.
      for (unsigned i = 0; i < 8; ++i) CHECK(f.chip.addContinuousConversion(3200));
    }
  };
  Sample out;
  CHECK(f.device.tryReadFresh(out).ok());
  f.checkTrace({4, 1, 0});
  checkFresh(f, out, ReadKind::CONTINUOUS_FRESH);
  f.clearTrace();
  CHECK(f.device.tryReadFresh(out).is(Err::MEASUREMENT_NOT_READY));
  f.checkTrace({4, 1});
}

}  // namespace

int main() {
  admissions();
  allContinuousProfiles();
  continuousGuardAndReadEffects();
  guardDenialsAndLimits();
  oneShotWireAndTiming();
  oneShotConfigFailures();
  renewedGuards();
  lateContinuousTransfers();
  continuousPollingAcrossWrap();
  shortOneShotBudget();
  residualOwnedOneShot();
  destructiveReadDuringShot();
  missingReadyAndThermLevel();
  diagnosticCapturesOwnedShot();
  busyOneShotPoll();
  continuousUpdateBetweenReads();
  return EXIT_SUCCESS;
}
