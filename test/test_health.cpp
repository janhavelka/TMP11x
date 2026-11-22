#include "Identity.h"
#include "RegisterTransport.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {

using Device = ::TMP11x::TMP11x;
using namespace ::TMP11x;
using namespace ::TMP11x::detail;
using namespace ::TMP11x::test;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);     \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

constexpr auto maxCount = std::numeric_limits<std::uint32_t>::max();
constexpr WriteAccess limit{WritePurpose::LIMIT, true, true};

struct Fixture {
  FakeTmp11xBus bus;
  FakeTmp11xDevice target;
  Config cfg;
  Device sensor;
  std::uint32_t clockOrigin = 0;
  unsigned clockCalls = 0;

  Fixture(std::uint8_t threshold, bool timed, Model model = Model::TMP117) : target{model} {
    cfg.model = model;
    cfg.i2cAddress = 0x48;
    cfg.i2cWrite = FakeTmp11xBus::write;
    cfg.i2cWriteRead = FakeTmp11xBus::writeRead;
    cfg.i2cUser = &bus;
    cfg.offlineThreshold = threshold;
    if (timed) {
      cfg.timeUser = this;
      cfg.nowMs = [](void* user) {
        auto& f = *static_cast<Fixture*>(user);
        ++f.clockCalls;
        // A sample taken before callback completion sees a different tick.
        return f.clockOrigin + static_cast<std::uint32_t>(f.bus.trace.size());
      };
    }
    CHECK(bus.attach(cfg.i2cAddress, target));
    CHECK(sensor.bind(cfg).ok());
  }
};

void checkCounts(const HealthSnapshot& h, DriverState state, std::uint32_t successes,
                 std::uint32_t failures, std::uint8_t streak) {
  CHECK(h.state == state && h.totalSuccess == successes);
  CHECK(h.totalFailures == failures && h.consecutiveFailures == streak);
  CHECK(!h.initialized);
}

void checkFault(const Status& observed, const Status& expected) {
  CHECK(observed.code == expected.code && observed.detail == expected.detail);
  CHECK(std::strcmp(observed.msg, expected.msg) == 0);
}

void checkReset(const Device& device) {
  const auto h = device.healthSnapshot();
  checkCounts(h, DriverState::UNINIT, 0, 0, 0);
  CHECK(h.lastError.ok() && !h.lastErrorMsValid && !h.lastSuccessMsValid);
  CHECK(!device.isInitialized() && !device.isOnline());
}

void checkPreserved(const Device& device, const HealthSnapshot& saved) {
  const auto h = device.healthSnapshot();
  checkCounts(h, saved.state, saved.totalSuccess, saved.totalFailures, saved.consecutiveFailures);
  checkFault(h.lastError, saved.lastError);
  CHECK(h.lastSuccessMsValid == saved.lastSuccessMsValid && h.lastSuccessMs == saved.lastSuccessMs);
  CHECK(h.lastErrorMsValid == saved.lastErrorMsValid && h.lastErrorMs == saved.lastErrorMs);
  CHECK(!device.isOnline());
}

void disappearanceAndClockWrap() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    Fixture f{2, true, model};
    f.clockOrigin = maxCount - 3;
    const auto tracked = RegisterTransport::tracked(f.sensor);
    std::uint16_t word = 0;
    checkReset(f.sensor);
    CHECK(readWord(tracked, 0, word, 10).status.ok());
    const auto first = f.sensor.healthSnapshot();
    checkCounts(first, DriverState::READY, 1, 0, 0);
    CHECK(first.lastSuccessMsValid && first.lastSuccessMs == maxCount - 2);
    CHECK(!first.lastErrorMsValid && first.lastError.ok());
    CHECK(!f.sensor.isOnline() && f.sensor.configTrust() == ConfigTrust::UNKNOWN);

    const auto nack = Status::Error(Err::I2C_NACK_ADDR, "sensor absent", -7);
    for (std::uint8_t i = 1; i <= 3; ++i) {
      const auto fault = i == 3 ? Status::Error(Err::I2C_BUS, "line stuck", -81) : nack;
      f.bus.script.push_back({fault});
      checkFault(readWord(tracked, 0, word, 10).status, fault);
      const auto h = f.sensor.healthSnapshot();
      checkCounts(h, i == 1 ? DriverState::DEGRADED : DriverState::OFFLINE, 1, i, i);
      CHECK(h.lastErrorMsValid && h.lastErrorMs == f.clockOrigin + 1u + i);
      CHECK(h.lastSuccessMsValid && h.lastSuccessMs == first.lastSuccessMs);
      checkFault(h.lastError, fault);
    }

    const auto offline = f.sensor.healthSnapshot();
    CHECK(offline.lastErrorMsValid && offline.lastErrorMs == 0);
    CHECK(readWord(tracked, 0, word, 10).status.ok());
    const auto recovered = f.sensor.healthSnapshot();
    checkCounts(recovered, DriverState::READY, 2, 3, 0);
    CHECK(recovered.lastSuccessMsValid && recovered.lastSuccessMs == 1);
    CHECK(recovered.lastErrorMsValid && recovered.lastErrorMs == 0);
    checkFault(recovered.lastError, offline.lastError);
    CHECK(f.bus.trace.size() == 5 && f.clockCalls == 5 && !f.sensor.isOnline());
  }
}

void mixedTransfersAndRefusals() {
  Fixture f{2, false};
  const auto tracked = RegisterTransport::tracked(f.sensor);
  // Exercise the constituent callbacks of a guarded limit write and readback.
  std::uint16_t word = 0xFFFF;
  CHECK(readWord(tracked, 4, word, 10).status.ok() && word == 0);
  CHECK(f.bus.trace.size() == 1 && f.bus.trace.back().combinedRead);
  checkCounts(f.sensor.healthSnapshot(), DriverState::READY, 1, 0, 0);
  CHECK(writeWord(tracked, 2, 0x1267, limit, 10).status.ok());
  CHECK(f.bus.trace.size() == 2 && !f.bus.trace.back().combinedRead);
  checkCounts(f.sensor.healthSnapshot(), DriverState::READY, 2, 0, 0);
  f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "one byte", 1), 1});
  word = 0xAAAA;
  CHECK(readWord(tracked, 2, word, 10).status.is(Err::I2C_SHORT_TRANSFER) && word == 0xAAAA);
  CHECK(f.bus.trace.size() == 3 && f.bus.trace.back().rx.size() == 1);
  checkCounts(f.sensor.healthSnapshot(), DriverState::DEGRADED, 2, 1, 1);

  const auto timeout = Status::Error(Err::I2C_TIMEOUT, "unknown completion", -32);
  f.bus.script.push_back({timeout, 0, false, 0xAB67});
  checkFault(writeWord(tracked, 2, 0xABCD, limit, 10).status, timeout);
  CHECK(f.bus.trace.size() == 4 && f.target.words[2] == 0xAB67);
  auto h = f.sensor.healthSnapshot();
  checkCounts(h, DriverState::OFFLINE, 2, 2, 2);
  checkFault(h.lastError, timeout);
  f.bus.script.push_back({Status::Error(Err::IN_PROGRESS, "invalid adapter result", -73)});
  const auto incoherent = readWord(tracked, 2, word, 10);
  CHECK(incoherent.attempted && incoherent.status.is(Err::I2C_ERROR) && incoherent.status.detail == -73);
  CHECK(f.bus.trace.size() == 5 && f.bus.trace.back().result.is(Err::IN_PROGRESS));
  h = f.sensor.healthSnapshot();
  checkCounts(h, DriverState::OFFLINE, 2, 3, 3);
  checkFault(h.lastError, incoherent.status);
  CHECK(!h.lastErrorMsValid && !h.lastSuccessMsValid);

  CHECK(readWord(tracked, 0x0B, word, 10).status.is(Err::INVALID_PARAM));
  CHECK(writeWord(tracked, 4, 0x8000, limit, 10).status.is(Err::UNSUPPORTED));
  CHECK(readWord(tracked, 2, word, 0).status.is(Err::OPERATION_TIMEOUT));
  checkPreserved(f.sensor, h);
  auto invalid = f.cfg;
  invalid.model = Model::UNSELECTED;
  HealthSnapshot separate{};
  const auto refusal = readWord(RegisterTransport{invalid, &separate}, 0, word, 10);
  CHECK(!refusal.attempted && refusal.status.is(Err::INVALID_CONFIG));
  checkCounts(separate, DriverState::UNINIT, 0, 0, 0);
  CHECK(f.bus.trace.size() == 5);

  f.target.words[0x0F] = 0x1116;
  IdentityInfo id;
  const auto mismatch = readIdentity(tracked, id, 10);
  CHECK(mismatch.status.is(Err::IDENTITY_MISMATCH) && mismatch.transfer.status.ok());
  CHECK(mismatch.transfer.attempted && id.rawValid && id.raw == 0x1116);
  checkCounts(f.sensor.healthSnapshot(), DriverState::READY, 3, 3, 0);
  checkFault(f.sensor.healthSnapshot().lastError, incoherent.status);
  CHECK(f.bus.trace.size() == 6);
}

void guardedWriteTimestamps() {
  Fixture f{1, true};
  const auto tracked = RegisterTransport::tracked(f.sensor);
  constexpr std::uint8_t highLimit = 0x02;

  const auto first = writeWord(tracked, highLimit, 0x1200, limit, 10);
  CHECK(first.attempted && first.status.ok());
  const auto ready = f.sensor.healthSnapshot();
  checkCounts(ready, DriverState::READY, 1, 0, 0);
  CHECK(ready.lastSuccessMsValid && ready.lastSuccessMs == 1 && !ready.lastErrorMsValid);
  CHECK(f.bus.trace.size() == 1 && f.clockCalls == 1);

  const auto nack = Status::Error(Err::I2C_NACK_DATA, "limit write rejected", -8);
  f.bus.script.push_back({nack});
  const auto failure = writeWord(tracked, highLimit, 0x2300, limit, 10);
  CHECK(failure.attempted);
  checkFault(failure.status, nack);
  const auto offline = f.sensor.healthSnapshot();
  checkCounts(offline, DriverState::OFFLINE, 1, 1, 1);
  checkFault(offline.lastError, nack);
  CHECK(offline.lastErrorMsValid && offline.lastErrorMs == 2);
  CHECK(offline.lastSuccessMsValid && offline.lastSuccessMs == 1);
  CHECK(f.bus.trace.size() == 2 && f.clockCalls == 2);

  for (const auto access : {WriteAccess{WritePurpose::LIMIT, false, true},
                            WriteAccess{WritePurpose::LIMIT, true, false}}) {
    const auto refused = writeWord(tracked, highLimit, 0x2300, access, 10);
    CHECK(!refused.attempted && refused.status.is(Err::INVALID_CONFIG));
    checkPreserved(f.sensor, offline);
    CHECK(f.bus.trace.size() == 2 && f.clockCalls == 2);
  }

  const auto recovery = writeWord(tracked, highLimit, 0x2300, limit, 10);
  CHECK(recovery.attempted && recovery.status.ok());
  const auto recovered = f.sensor.healthSnapshot();
  checkCounts(recovered, DriverState::READY, 2, 1, 0);
  CHECK(recovered.lastSuccessMsValid && recovered.lastSuccessMs == 3);
  CHECK(recovered.lastErrorMsValid && recovered.lastErrorMs == 2);
  checkFault(recovered.lastError, nack);
  CHECK(f.bus.trace.size() == 3 && f.clockCalls == 3);
}

void neutralDiagnosticsAndLifecycle() {
  Fixture f{1, true};
  f.clockOrigin = maxCount;
  const auto tracked = RegisterTransport::tracked(f.sensor);
  std::uint16_t word = 0;
  CHECK(readWord(tracked, 0, word, 10).status.ok());
  f.bus.script.push_back({Status::Error(Err::I2C_BUS, "previous failure", -82)});
  CHECK(readWord(tracked, 0, word, 10).status.is(Err::I2C_BUS));
  const auto offline = f.sensor.healthSnapshot();
  checkCounts(offline, DriverState::OFFLINE, 1, 1, 1);
  CHECK(offline.lastSuccessMsValid && offline.lastSuccessMs == 0 && offline.lastErrorMsValid);

  CHECK(readWord(tracked, 0, word, 0).status.is(Err::OPERATION_TIMEOUT));
  Device peer;
  CHECK(peer.bind(f.cfg).ok());
  checkReset(peer);
  checkPreserved(f.sensor, offline);
  for (bool probe : {false, true}) {
    for (const auto expected : {Err::OK, Err::IDENTITY_MISMATCH, Err::I2C_TIMEOUT}) {
      f.target.words[0x0F] = expected == Err::IDENTITY_MISMATCH ? 0x1116 : 0x0117;
      if (expected == Err::I2C_TIMEOUT) {
        f.bus.script.push_back({Status::Error(expected, "diagnostic timeout", -13)});
      }
      const auto before = f.bus.trace.size();
      IdentityInfo id;
      const auto result = probe ? f.sensor.probe() : f.sensor.readIdentity(id);
      CHECK(result.is(expected) && f.bus.trace.size() == before + 1);
      CHECK(f.bus.trace.back().combinedRead && f.bus.trace.back().tx[0] == 0x0F);
      if (expected == Err::I2C_TIMEOUT) CHECK(result.detail == -13);
      checkPreserved(f.sensor, offline);
    }
  }
  auto rejected = f.cfg;
  rejected.offlineThreshold = 0;
  CHECK(f.sensor.bind(rejected).is(Err::INVALID_CONFIG));
  checkPreserved(f.sensor, offline);
  CHECK(f.sensor.bind(f.cfg).ok());
  checkReset(f.sensor);
  CHECK(f.bus.trace.size() == 8 && f.clockCalls == 2);

  for (const bool unbind : {false, true}) {
    f.bus.script.push_back({Status::Error(Err::I2C_NACK, "first attempt", -3)});
    CHECK(readWord(RegisterTransport::tracked(f.sensor), 0, word, 10).status.is(Err::I2C_NACK));
    checkCounts(f.sensor.healthSnapshot(), DriverState::OFFLINE, 0, 1, 1);
    const auto before = f.bus.trace.size();
    const auto samples = f.clockCalls;
    CHECK((unbind ? f.sensor.unbind() : f.sensor.end()).ok());
    CHECK(f.sensor.isBound() == !unbind);
    checkReset(f.sensor);
    CHECK(f.bus.trace.size() == before && f.clockCalls == samples);
  }
}

void saturatingCounters() {
  Fixture f{255, false};
  HealthSnapshot history{};
  history.totalFailures = maxCount - 2;
  history.totalSuccess = maxCount - 1;
  history.consecutiveFailures = 253;
  history.state = DriverState::DEGRADED;
  const RegisterTransport tracked{f.cfg, &history};
  std::uint16_t word = 0;
  for (unsigned i = 0; i < 3; ++i) {
    f.bus.script.push_back({Status::Error(Err::I2C_NACK_ADDR, "address NACK")});
    CHECK(readWord(tracked, 0, word, 10).status.is(Err::I2C_NACK_ADDR));
    if (i == 0) checkCounts(history, DriverState::DEGRADED, maxCount - 1, maxCount - 1, 254);
    else checkCounts(history, DriverState::OFFLINE, maxCount - 1, maxCount, 255);
  }
  for (unsigned i = 0; i < 2; ++i) {
    CHECK(readWord(tracked, 0, word, 10).status.ok());
    checkCounts(history, DriverState::READY, maxCount, maxCount, 0);
  }
  CHECK(f.bus.trace.size() == 5);
  checkReset(f.sensor);
}

void onlinePredicate() {
  for (const bool initialized : {false, true}) {
    CHECK(!detail::isOnline(initialized, DriverState::UNINIT));
    CHECK(detail::isOnline(initialized, DriverState::READY) == initialized);
    CHECK(detail::isOnline(initialized, DriverState::DEGRADED) == initialized);
    CHECK(!detail::isOnline(initialized, DriverState::OFFLINE));
  }
}

}  // namespace

int main() {
  disappearanceAndClockWrap();
  mixedTransfersAndRefusals();
  guardedWriteTimestamps();
  neutralDiagnosticsAndLifecycle();
  saturatingCounters();
  onlinePredicate();
  return EXIT_SUCCESS;
}
