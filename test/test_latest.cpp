#include "Apply.h"
#include "LatestRead.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <deque>

namespace {

using Device = ::TMP11x::TMP11x;
using namespace ::TMP11x;
using namespace ::TMP11x::detail;
using namespace ::TMP11x::test;

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);      \
      std::exit(EXIT_FAILURE);                                                  \
    }                                                                           \
  } while (false)

constexpr Model models[]{Model::TMP116, Model::TMP116N, Model::TMP117,
                         Model::TMP117M, Model::TMP117N, Model::TMP119};

struct Fixture {
  FakeTmp11xBus bus;
  FakeTmp11xDevice chip;
  Config config;
  Device device;
  std::deque<std::uint32_t> ticks;

  Fixture(Model model, Mode mode, bool withClock) : chip(model) {
    config.model = model;
    config.settings.mode = mode;
    config.i2cAddress = 0x49;
    config.i2cUser = &bus;
    config.i2cWriteRead = FakeTmp11xBus::writeRead;
    config.i2cWrite = FakeTmp11xBus::write;
    if (withClock) {
      config.timeUser = &ticks;
      config.nowMs = [](void* context) {
        auto& times = *static_cast<std::deque<std::uint32_t>*>(context);
        CHECK(!times.empty());
        const auto now = times.front();
        times.pop_front();
        return now;
      };
    }
    CHECK(bus.attach(config.i2cAddress, chip));
    CHECK(device.bind(config).ok());
  }
};

void sameSample(const Sample& actual, const Sample& expected) {
  CHECK(actual.raw == expected.raw && actual.counts == expected.counts);
  CHECK(actual.celsius == expected.celsius && actual.observedAtMs == expected.observedAtMs);
  CHECK(actual.timestampValid == expected.timestampValid);
  CHECK(actual.acquisitionKind == expected.acquisitionKind && actual.freshConversion == expected.freshConversion);
  CHECK(actual.profileGeneration == expected.profileGeneration && actual.configTrust == expected.configTrust);
  CHECK(actual.offset.state == expected.offset.state && actual.offset.counts == expected.offset.counts);
  CHECK(actual.offset.countsValid == expected.offset.countsValid);
}

// Different from both the seeded cache and a default-initialized sample.
constexpr Sample callerValue{0x0100, 256, 2.0, 42, true, ReadKind::ONE_SHOT, true,
                             9, ConfigTrust::VERIFIED, {OffsetState::VERIFIED_MANAGED, 3, true}};

void publicGuards() {
  Device unbound;
  auto sample = callerValue;
  double scalar = 23.5;
  CHECK(unbound.readLatest(sample).is(Err::NOT_BOUND));
  CHECK(unbound.readTemperature(scalar).is(Err::NOT_BOUND));
  sameSample(sample, callerValue);
  CHECK(scalar == 23.5);

  for (const auto model : models) {
    Fixture f{model, Mode::CONTINUOUS, true};
    f.chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0xA117;
    for (const bool probe : {false, true}) {
      if (probe) CHECK(f.device.probe().ok());
      const auto before = f.bus.trace.size();
      CHECK(f.device.readLatest(sample).is(Err::NOT_INITIALIZED));
      CHECK(f.device.readTemperature(scalar).is(Err::NOT_INITIALIZED));
      sameSample(sample, callerValue);
      CHECK(scalar == 23.5 && f.bus.trace.size() == before);
      CHECK(!LatestRead{f.device}.cache);
      const auto health = f.device.healthSnapshot();
      CHECK(health.totalSuccess == 0 && health.totalFailures == 0);
    }
  }
}

void checkTempFrame(const Fixture& f, std::uint32_t grant) {
  const auto& frame = f.bus.trace.back();
  CHECK(frame.combinedRead && frame.address == 0x49);
  CHECK(frame.tx.size() == 1 && frame.tx[0] == 0x00 && frame.rxLen == 2);
  CHECK(frame.timeoutMs == grant);
}

void clocklessLatest() {
  for (const auto model : models) {
    for (const auto mode : {Mode::CONTINUOUS, Mode::SHUTDOWN}) {
      Fixture f{model, mode, false};
      auto view = LatestRead{f.device};
      CHECK(!view.cache);
      f.chip.words[0] = 0x8000;
      f.chip.words[1] = 0xE000;
      auto out = callerValue;
      for (const auto trust : {ConfigTrust::VERIFIED, ConfigTrust::DIRTY}) {
        view.trust = trust;
        view.generation = 7;
        const auto before = f.bus.trace.size();
        const auto result = view.read(out);
        CHECK(result.status.ok() && result.transfer.attempted && result.transfer.status.ok());
        CHECK(result.raw == 0x8000 && !result.transfer.completedMs);
        CHECK(f.bus.trace.size() == before + 1);
        checkTempFrame(f, f.config.i2cTimeoutMs);
        CHECK(f.chip.words[1] == 0xC000);
        CHECK(out.raw == 0x8000 && out.counts == -32768 && out.celsius == -256.0);
        CHECK(!out.timestampValid && !out.freshConversion && out.acquisitionKind == ReadKind::LATEST);
        CHECK(out.configTrust == trust && out.profileGeneration == 7);
        CHECK(out.offset.state == OffsetState::UNKNOWN && !out.offset.countsValid);
        CHECK(view.cache.has_value());
        sameSample(*view.cache, out);
        out = callerValue;
        CHECK(view.cache->raw == 0x8000);
      }
      const auto health = f.device.healthSnapshot();
      CHECK(health.totalSuccess == 2 && health.totalFailures == 0 && !health.lastSuccessMsValid);
      const auto saved = view.cache.value();
      const auto refused = view.read(out, ReadBudget{0, 10});
      CHECK(refused.status.is(Err::CLOCK_REQUIRED) && !refused.transfer.attempted);
      CHECK(!refused.raw && !refused.transfer.completedMs && f.bus.trace.size() == 2);
      sameSample(out, callerValue);
      sameSample(view.cache.value(), saved);
    }
  }
}

void failedReadsAndLifecycle() {
  Fixture f{Model::TMP117, Mode::SHUTDOWN, true};
  auto view = LatestRead{f.device};
  view.trust = ConfigTrust::DIRTY;
  view.generation = 6;
  f.chip.words[0] = 0xFFFF;
  f.ticks = {19};
  Sample saved;
  CHECK(view.read(saved).status.ok());
  CHECK(saved.counts == -1 && saved.celsius == -0.0078125);
  CHECK(saved.timestampValid && saved.observedAtMs == 19);
  CHECK(view.cache.has_value());
  auto out = callerValue;
  for (const bool consume : {false, true}) {
    f.chip.words[1] = 0xE000;
    f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short TEMP", 1), 1, consume});
    f.ticks = {20};
    const auto result = view.read(out);
    CHECK(result.status.is(Err::I2C_SHORT_TRANSFER) && result.transfer.status.detail == 1);
    CHECK(result.transfer.attempted && result.transfer.completedMs == 20 && !result.raw);
    CHECK(f.chip.words[1] == (consume ? 0xC000 : 0xE000));
    CHECK(f.bus.trace.back().rx.size() == 1);
    sameSample(out, callerValue);
    sameSample(view.cache.value(), saved);
  }
  f.ticks = {21};
  f.bus.script.push_back({Status::Error(Err::I2C_NACK_ADDR, "address NACK", -2)});
  const auto nack = view.read(out);
  CHECK(nack.status.is(Err::I2C_NACK_ADDR) && nack.transfer.attempted && !nack.raw);
  sameSample(out, callerValue);
  sameSample(view.cache.value(), saved);
  const auto health = f.device.healthSnapshot();
  CHECK(health.totalSuccess == 1 && health.totalFailures == 3);

  auto invalid = f.config;
  invalid.model = Model::UNSELECTED;
  CHECK(f.device.bind(invalid).is(Err::INVALID_CONFIG));
  sameSample(LatestRead{f.device}.cache.value(), saved);
  f.chip.words[0x0F] = 0x1116;
  CHECK(f.device.probe().is(Err::IDENTITY_MISMATCH));
  sameSample(LatestRead{f.device}.cache.value(), saved);
  CHECK(saved.profileGeneration == 6 && saved.configTrust == ConfigTrust::DIRTY);
  CHECK(f.device.bind(f.config).ok() && !LatestRead{f.device}.cache);

  for (const bool unbind : {false, true}) {
    f.ticks = {28};
    CHECK(LatestRead{f.device}.read(out).status.ok());
    CHECK(LatestRead{f.device}.cache.has_value());
    const auto before = f.bus.trace.size();
    CHECK((unbind ? f.device.unbind() : f.device.end()).ok());
    CHECK(!LatestRead{f.device}.cache && f.bus.trace.size() == before);
  }
}

void wrappedDeadline() {
  constexpr ReadBudget budget{0xFFFFFFF8u, 12};
  constexpr struct {
    std::uint32_t before;
    std::uint32_t completed;
    std::uint32_t grant;
    bool expired;
  } cases[] = {
      {0xFFFFFFFFu, 0, 5, false}, {0xFFFFFFFFu, 4, 5, true},  // Wrap during TEMP.
      {1, 3, 3, false}, {1, 4, 3, true}, {1, 5, 3, true},    // Wrap before TEMP.
      {1, 0xFFFFFFFCu, 3, true},  // More than one full clock cycle since admission.
  };
  for (const auto& row : cases) {
    for (const bool shortRead : {false, true}) {
      Fixture f{Model::TMP119, Mode::CONTINUOUS, true};
      auto view = LatestRead{f.device};
      f.chip.words[0] = 0x0D00;
      f.ticks = {0xFFFFFFF7u};
      Sample prior;
      CHECK(view.read(prior).status.ok() && prior.raw == 0x0D00);

      f.ticks = {row.before, row.completed};
      f.chip.words[0] = 0x0C80;
      f.chip.words[1] = 0xE000;
      if (shortRead) f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short", 1), 1, true});
      auto out = callerValue;
      const auto result = view.read(out, budget);
      CHECK(f.ticks.empty() && f.bus.trace.size() == 2);
      checkTempFrame(f, row.grant);
      CHECK(result.transfer.attempted && result.transfer.completedMs == row.completed);
      CHECK(f.chip.words[1] == 0xC000);
      const auto health = f.device.healthSnapshot();
      if (shortRead) {
        CHECK(result.transfer.status.is(Err::I2C_SHORT_TRANSFER) && result.transfer.status.detail == 1);
        CHECK(!result.raw && health.totalSuccess == 1 && health.totalFailures == 1);
        CHECK(health.lastErrorMsValid && health.lastErrorMs == row.completed);
      } else {
        CHECK(result.transfer.status.ok() && result.raw == 0x0C80);
        CHECK(health.totalSuccess == 2 && health.totalFailures == 0);
        CHECK(health.lastSuccessMsValid && health.lastSuccessMs == row.completed);
      }
      if (row.expired) CHECK(result.status.is(Err::OPERATION_TIMEOUT));
      else CHECK(result.status.is(shortRead ? Err::I2C_SHORT_TRANSFER : Err::OK));
      if (result.status.ok()) {
        CHECK(out.raw == 0x0C80 && out.celsius == 25.0 && !out.freshConversion);
        CHECK(out.timestampValid && out.observedAtMs == row.completed);
        sameSample(view.cache.value(), out);
      } else {
        sameSample(out, callerValue);
        sameSample(view.cache.value(), prior);
      }
    }
  }

  Fixture f{Model::TMP117, Mode::CONTINUOUS, true};
  auto view = LatestRead{f.device};
  Sample prior;
  f.ticks = {0xFFFFFFF7u};
  f.chip.words[0] = 0x0D00;
  CHECK(view.read(prior).status.ok());
  auto out = callerValue;
  f.chip.words[1] = 0xE000;
  f.ticks = {4};
  const auto expired = view.read(out, budget);
  CHECK(expired.status.is(Err::OPERATION_TIMEOUT) && !expired.transfer.attempted);
  CHECK(!expired.raw && !expired.transfer.completedMs);
  sameSample(out, callerValue);
  sameSample(view.cache.value(), prior);
  CHECK(f.bus.trace.size() == 1 && f.ticks.empty() && f.chip.words[1] == 0xE000);
  const auto health = f.device.healthSnapshot();
  CHECK(health.totalSuccess == 1 && health.totalFailures == 0);
}

void tempConsumesRetainedReadiness() {
  constexpr ReadBudget budget{1000, 5};
  constexpr struct {
    std::uint32_t before;
    std::uint32_t completed;
    bool shortRead;
    Err expected;
  } cases[] = {
      {1005, 0, false, Err::OPERATION_TIMEOUT},     // No callback at admission.
      {1000, 1001, false, Err::OK},
      {1000, 1001, true, Err::I2C_SHORT_TRANSFER},
      {1000, 1005, false, Err::OPERATION_TIMEOUT},  // Late complete word.
      {1000, 1005, true, Err::OPERATION_TIMEOUT},   // Late ambiguous read.
  };
  for (const auto& row : cases) {
    Fixture f{Model::TMP117, Mode::CONTINUOUS, true};
    f.chip.words[0x0F] = 0xA117;
    for (unsigned i = 0; i < 100; ++i) f.ticks.push_back(i);
    CHECK(f.device.begin().ok());
    f.ticks = {995, 996, 997, 998, 999};
    f.chip.words[1] |= 0xA000;
    ConfigurationObservation observed;
    CHECK(f.device.readConfiguration(observed).ok() && observed.raw == 0xA220);
    Apply evidence{f.device};
    CHECK(evidence.readyPending() && !evidence.readyHistoryUncertain());
    CHECK(evidence.alerts().pendingHigh && !evidence.alerts().possibleLoss);

    f.chip.words[1] |= 0x6000;  // New LOW/READY since the captured HIGH.
    f.ticks = {row.before};
    const bool attempted = row.before < 1005;
    if (attempted) f.ticks.push_back(row.completed);
    if (row.shortRead) f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "TEMP short"), 1, true});
    auto out = callerValue;
    const auto result = LatestRead{f.device}.read(out, budget);
    CHECK(result.status.is(row.expected) && result.transfer.attempted == attempted);
    CHECK(evidence.readyPending() == !attempted);
    CHECK(evidence.readyHistoryUncertain() == row.shortRead);
    CHECK(evidence.baselineMs().has_value() == !row.shortRead);
    CHECK(evidence.alerts().pendingHigh && !evidence.alerts().pendingLow && !evidence.alerts().possibleLoss);
    CHECK(f.chip.words[1] == (attempted ? 0x4220 : 0x6220));
    if (!result.status.ok()) {
      sameSample(out, callerValue);
      CHECK(!LatestRead{f.device}.cache);
    }
  }
}

}  // namespace

int main() {
  publicGuards();
  clocklessLatest();
  failedReadsAndLifecycle();
  wrappedDeadline();
  tempConsumesRetainedReadiness();
  return EXIT_SUCCESS;
}
