#include "TMP11x/TMP11x.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>
#include <vector>

namespace {

using Device = ::TMP11x::TMP11x;
using namespace ::TMP11x;
using namespace ::TMP11x::test;

static_assert(!std::is_copy_constructible_v<Device> && !std::is_copy_assignable_v<Device>);
static_assert(!std::is_move_constructible_v<Device> && !std::is_move_assignable_v<Device>);

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);   \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

constexpr auto maxInterval = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
constexpr IdentityInfo sentinel{Model::TMP117M, 0x4B, 0xC117, true,
                                IdentityGroup::TMP117_LIKE, 12, true, true};

Config binding(FakeTmp11xBus& route, Model model, std::uint8_t address) {
  Config cfg;
  cfg.model = model;
  cfg.i2cAddress = address;
  cfg.i2cWrite = FakeTmp11xBus::write;
  cfg.i2cWriteRead = FakeTmp11xBus::writeRead;
  cfg.i2cUser = &route;
  cfg.i2cTimeoutMs = 17;
  return cfg;
}

void checkIdentity(const IdentityInfo& actual, const IdentityInfo& expected) {
  CHECK(actual.rawValid == expected.rawValid && actual.raw == expected.raw);
  CHECK(actual.model == expected.model && actual.i2cAddress == expected.i2cAddress);
  CHECK(actual.group == expected.group && actual.compatible == expected.compatible);
  CHECK(actual.revision == expected.revision && actual.revisionValid == expected.revisionValid);
}

void checkSettings(const SensorSettings& actual, const SensorSettings& expected) {
  CHECK(actual.mode == expected.mode && actual.cycle == expected.cycle);
  CHECK(actual.averaging == expected.averaging && actual.alertMode == expected.alertMode);
  CHECK(actual.polarity == expected.polarity && actual.pinFunction == expected.pinFunction);
  CHECK(actual.limits.lowCounts == expected.limits.lowCounts);
  CHECK(actual.limits.highCounts == expected.limits.highCounts);
}

void checkNoObservation(const Device& sensor, const Config& cfg) {
  CHECK(sensor.isBound() && !sensor.isInitialized());
  CHECK(sensor.configTrust() == ConfigTrust::UNKNOWN);
  const auto id = sensor.identityInfo();
  CHECK(!id.rawValid && !id.compatible && !id.revisionValid);
  CHECK(id.model == cfg.model && id.i2cAddress == cfg.i2cAddress);
  checkSettings(sensor.desiredSettings(), cfg.settings);
}

void checkRead(const FakeTmp11xBus& route, std::size_t count, const Config& cfg,
               const std::vector<std::uint8_t>& received) {
  CHECK(route.trace.size() == count);
  const auto& frame = route.trace.back();
  CHECK(frame.combinedRead && frame.address == cfg.i2cAddress);
  CHECK(frame.timeoutMs == cfg.i2cTimeoutMs && frame.txLen == 1 && frame.rxLen == 2);
  CHECK(frame.tx == std::vector<std::uint8_t>{0x0F} && frame.rx == received);
}

void emptyAndStatelessBinding() {
  Device sensor;
  auto out = sentinel;
  for (int i = 0; i < 2; ++i) {
    CHECK(!sensor.isBound() && !sensor.isInitialized());
    CHECK(sensor.readIdentity(out).is(Err::NOT_BOUND));
    checkIdentity(out, sentinel);
    CHECK(sensor.probe().is(Err::NOT_BOUND));
    CHECK(sensor.end().ok() && sensor.unbind().ok());
  }

  // These callbacks explicitly support null contexts. Bind/teardown must
  // leave them untouched; the explicit read alone takes the combined path.
  Config cfg;
  cfg.model = Model::TMP117;
  cfg.i2cAddress = 0x48;
  cfg.i2cTimeoutMs = cfg.operationTimeoutMs = cfg.clockResolutionMs = 1;
  cfg.offlineThreshold = 1;
  cfg.i2cWrite = [](std::uint8_t, const std::uint8_t*, std::size_t, std::uint32_t, void*) -> Status {
    std::abort();
  };
  cfg.i2cWriteRead = [](std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                        std::uint8_t* rx, std::size_t rxLen, std::uint32_t timeoutMs, void* user) {
    CHECK(user == nullptr && address == 0x48 && timeoutMs == 1);
    CHECK(txLen == 1 && tx[0] == 0x0F && rxLen == 2);
    rx[0] = 0x01;
    rx[1] = 0x17;
    return Status::Ok();
  };
  cfg.nowMs = [](void*) -> std::uint32_t { std::abort(); };
  cfg.readAlertPin = [](bool&, void*) -> Status { std::abort(); };
  CHECK(sensor.bind(cfg).ok());
  checkNoObservation(sensor, cfg);
  CHECK(sensor.readIdentity(out).ok() && out.raw == 0x0117);
  CHECK(sensor.end().ok() && sensor.unbind().ok());
}

void clocklessFamilyChecks() {
  for (auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                     Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    for (std::uint8_t address = 0x48; address <= 0x4B; ++address) {
      FakeTmp11xDevice target{model};
      FakeTmp11xBus route;
      CHECK(route.attach(address, target));
      auto cfg = binding(route, model, address);
      cfg.i2cTimeoutMs = cfg.operationTimeoutMs = cfg.clockResolutionMs = maxInterval;
      cfg.offlineThreshold = 255;
      Device sensor;
      CHECK(sensor.bind(cfg).ok() && route.trace.empty());
      checkNoObservation(sensor, cfg);
      std::size_t count = 0;
      constexpr std::uint16_t words[] = {0x0117, 0x2117, 0x1116, 0x0116};
      for (auto word : words) {
        target.words[0x0F] = word;
        const bool selects116 = model == Model::TMP116 || model == Model::TMP116N;
        const bool did117 = word == 0x0117 || word == 0x2117;
        const bool compatible = selects116 ? word == 0x1116 : did117;
        const Err expected = compatible ? Err::OK : Err::IDENTITY_MISMATCH;
        const std::vector<std::uint8_t> bytes{static_cast<std::uint8_t>(word >> 8),
                                               static_cast<std::uint8_t>(word)};
        auto out = sentinel;
        CHECK(sensor.readIdentity(out).is(expected));
        checkRead(route, ++count, cfg, bytes);
        CHECK(out.model == model && out.i2cAddress == address);
        CHECK(out.rawValid && out.raw == word && out.compatible == compatible);
        CHECK(out.revisionValid == did117);
        if (did117) CHECK(out.revision == (word >> 12));
        checkIdentity(sensor.identityInfo(), out);
        CHECK(sensor.probe().is(expected));
        checkRead(route, ++count, cfg, bytes);
        checkIdentity(sensor.identityInfo(), out);
        CHECK(!sensor.isInitialized() && sensor.configTrust() == ConfigTrust::UNKNOWN);
      }
    }
  }
}

void rejectedRebindPreservesTarget() {
  FakeTmp11xBus routeA, routeB;
  FakeTmp11xDevice target{Model::TMP117};
  target.words[0x0F] = 0x2117;
  CHECK(routeA.attach(0x4A, target));
  auto original = binding(routeA, Model::TMP117, 0x4A);
  original.settings = {Mode::SHUTDOWN, Cycle::CODE_7, Averaging::SAMPLES_64,
                       AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY, {-99, 22}};
  Device sensor;
  CHECK(sensor.bind(original).ok() && sensor.probe().ok());
  const auto saved = sensor.identityInfo();
  auto replacement = binding(routeB, Model::TMP116, 0x49);
  replacement.i2cTimeoutMs = 33;
  auto rejects = [&](auto field, auto invalid) {
    auto trial = replacement;
    trial.*field = invalid;
    const auto before = routeA.trace.size();
    CHECK(sensor.bind(trial).is(Err::INVALID_CONFIG));
    CHECK(routeA.trace.size() == before && routeB.trace.empty());
    CHECK(sensor.isBound() && !sensor.isInitialized());
    checkIdentity(sensor.identityInfo(), saved);
    checkSettings(sensor.desiredSettings(), original.settings);
    CHECK(sensor.probe().ok());
    checkRead(routeA, before + 1, original, {0x21, 0x17});
  };
  rejects(&Config::model, Model::UNSELECTED);
  rejects(&Config::model, static_cast<Model>(0xFE));
  constexpr std::uint8_t badAddresses[] = {0x47, 0x4C, 0x90};
  for (auto invalid : badAddresses) rejects(&Config::i2cAddress, invalid);
  rejects(&Config::i2cWrite, nullptr);
  rejects(&Config::i2cWriteRead, nullptr);
  for (auto field : {&Config::i2cTimeoutMs, &Config::operationTimeoutMs, &Config::clockResolutionMs}) {
    rejects(field, 0u);
    rejects(field, maxInterval + 1);
  }
  rejects(&Config::offlineThreshold, std::uint8_t{0});

  FakeTmp11xDevice targetB{Model::TMP116};
  targetB.words[0x0F] = 0x1116;
  CHECK(routeB.attach(0x49, targetB));
  const auto oldCount = routeA.trace.size();
  CHECK(sensor.bind(replacement).ok());
  checkNoObservation(sensor, replacement);
  CHECK(routeB.trace.empty());
  CHECK(sensor.probe().ok());
  checkRead(routeB, 1, replacement, {0x11, 0x16});
  CHECK(routeA.trace.size() == oldCount);
}

void failedReadsPreserveEvidence() {
  FakeTmp11xBus route;
  FakeTmp11xDevice target{Model::TMP119};
  auto cfg = binding(route, Model::TMP119, 0x49);
  // Untimed diagnostics get the callback cap even with a shorter apply budget.
  cfg.operationTimeoutMs = 3;
  Device sensor;
  CHECK(route.attach(0x49, target) && sensor.bind(cfg).ok());
  const auto empty = sensor.identityInfo();
  auto out = sentinel;
  route.script.push_back({Status::Error(Err::I2C_NACK_ADDR, "NACK", -5)});
  CHECK(sensor.readIdentity(out).is(Err::I2C_NACK_ADDR));
  checkRead(route, 1, cfg, {});
  checkIdentity(out, sentinel);
  checkIdentity(sensor.identityInfo(), empty);

  target.words[0x0F] = 0x1116;
  CHECK(sensor.probe().is(Err::IDENTITY_MISMATCH));
  checkRead(route, 2, cfg, {0x11, 0x16});
  const auto mismatch = sensor.identityInfo();
  CHECK(mismatch.raw == 0x1116 && mismatch.rawValid && !mismatch.compatible);
  target.words[0x0F] = 0x2117;
  route.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short", 1), 1});
  const auto shortRead = sensor.readIdentity(out);
  CHECK(shortRead.is(Err::I2C_SHORT_TRANSFER) && shortRead.detail == 1);
  checkRead(route, 3, cfg, {0x21});
  checkIdentity(out, sentinel);
  checkIdentity(sensor.identityInfo(), mismatch);
  route.script.push_back({Status::Error(Err::I2C_NACK_ADDR, "NACK", -5)});
  const auto nack = sensor.probe();
  CHECK(nack.is(Err::I2C_NACK_ADDR) && nack.detail == -5);
  checkRead(route, 4, cfg, {});
  checkIdentity(sensor.identityInfo(), mismatch);
  CHECK(sensor.probe().ok());
  checkRead(route, 5, cfg, {0x21, 0x17});
  const auto compatible = sensor.identityInfo();
  CHECK(compatible.compatible && compatible.raw == 0x2117);
  target.words[0x0F] = 0x1116;
  route.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short", 1), 1});
  CHECK(sensor.readIdentity(out).is(Err::I2C_SHORT_TRANSFER));
  checkRead(route, 6, cfg, {0x11});
  checkIdentity(sensor.identityInfo(), compatible);
  checkIdentity(out, sentinel);
  CHECK(!sensor.isInitialized() && sensor.configTrust() == ConfigTrust::UNKNOWN);
}

void independentRoutesAndTeardown() {
  FakeTmp11xBus routeA, routeB;
  FakeTmp11xDevice targetA{Model::TMP116}, targetB{Model::TMP117M};
  targetA.words[0x0F] = 0x1116;
  targetB.words[0x0F] = 0xA117;
  CHECK(routeA.attach(0x48, targetA) && routeB.attach(0x48, targetB));
  auto cfgA = binding(routeA, Model::TMP116, 0x48);
  auto cfgB = binding(routeB, Model::TMP117M, 0x48);
  cfgB.settings.mode = Mode::SHUTDOWN;
  cfgB.i2cTimeoutMs = 31;
  Device a, b;
  CHECK(a.bind(cfgA).ok() && b.bind(cfgB).ok());
  CHECK(routeA.trace.empty() && routeB.trace.empty());
  CHECK(b.probe().ok() && a.probe().ok());
  checkRead(routeA, 1, cfgA, {0x11, 0x16});
  checkRead(routeB, 1, cfgB, {0xA1, 0x17});
  const auto savedB = b.identityInfo();
  CHECK(a.identityInfo().raw == 0x1116 && savedB.raw == 0xA117);
  CHECK(a.bind(cfgA).ok());
  checkNoObservation(a, cfgA);
  CHECK(a.probe().ok());
  checkRead(routeA, 2, cfgA, {0x11, 0x16});

  // Replace A's target, then reuse the caller's Config for a different route.
  auto candidate = cfgB;
  CHECK(a.bind(candidate).ok());
  candidate = cfgA;
  checkNoObservation(a, cfgB);
  CHECK(a.probe().ok());
  checkRead(routeB, 2, cfgB, {0xA1, 0x17});
  for (int i = 0; i < 2; ++i) {
    CHECK(a.end().ok());
    checkNoObservation(a, cfgB);
    CHECK(routeA.trace.size() == 2 && routeB.trace.size() == 2);
  }
  CHECK(a.probe().ok());
  checkRead(routeB, 3, cfgB, {0xA1, 0x17});
  CHECK(a.unbind().ok() && a.unbind().ok());
  CHECK(!a.isBound() && !a.isInitialized() && a.configTrust() == ConfigTrust::UNKNOWN);
  checkIdentity(a.identityInfo(), IdentityInfo{});
  checkSettings(a.desiredSettings(), SensorSettings{});
  auto out = sentinel;
  CHECK(a.readIdentity(out).is(Err::NOT_BOUND) && a.probe().is(Err::NOT_BOUND));
  checkIdentity(out, sentinel);
  checkIdentity(b.identityInfo(), savedB);
  CHECK(b.readIdentity(out).ok());
  checkIdentity(out, savedB);
  checkRead(routeB, 4, cfgB, {0xA1, 0x17});
  CHECK(routeA.trace.size() == 2);
}

}  // namespace

int main() {
  emptyAndStatelessBinding();
  clocklessFamilyChecks();
  rejectedRebindPreservesTarget();
  failedReadsPreserveEvidence();
  independentRoutesAndTeardown();
  return EXIT_SUCCESS;
}
