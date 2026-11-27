#include "Apply.h"
#include "LatestRead.h"
#include "Reset.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using ::TMP11x::detail::Apply;
using ::TMP11x::detail::LatestRead;
using ::TMP11x::detail::Reset;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);      \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                          \
  } while (false)

struct Fixture {
  explicit Fixture(Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x4B;
    config.operationTimeoutMs = 64;
    config.i2cUser = this;
    config.timeUser = this;
    config.nowMs = [](void* user) {
      auto& f = *static_cast<Fixture*>(user);
      f.lastTick = f.clockReads++ / 8;
      return f.lastTick;
    };
    config.i2cWriteRead = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                             std::uint8_t* rx, std::size_t m, std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      if (*tx == 0x0F) f.idTick = f.lastTick;
      if (*tx == 4) {
        f.guardTicks.push_back(f.lastTick);
        if (f.clearSecondGuard && f.guardTicks.size() == 2) f.chip.words[4] = 0;
      }
      const auto result = FakeTmp11xBus::writeRead(address, tx, n, rx, m, ms, &f.bus);
      if (*tx == 4 && f.lateGuard) f.clockReads += f.config.operationTimeoutMs * 8;
      return result;
    };
    config.i2cWrite = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                         std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      if (f.alertsAtCommand) {
        // Observe the evidence at callback entry without re-entering the driver.
        CHECK(f.alertsAtCommand->pendingHigh && f.alertsAtCommand->possibleLoss && f.alertsAtCommand->mixedOrUnknown);
      }
      const auto result = FakeTmp11xBus::write(address, tx, n, ms, &f.bus);
      if (f.alertsAtCommand) {
        if (!result.ok()) f.chip.words[3] = 0xF000;  // One possible post-command image.
        if (f.lateCommand) f.clockReads += f.config.operationTimeoutMs * 8;
      }
      return result;
    };
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0xB117;
    CHECK(bus.attach(config.i2cAddress, chip));
  }

  void begin() {
    CHECK(device.begin(config).ok());
    bus.trace.clear();
    guardTicks.clear();
  }

  void checkFrames(std::size_t guards, bool command) const {
    CHECK(bus.trace.size() == guards + (command ? 1u : 0u));
    for (std::size_t i = 0; i < bus.trace.size(); ++i) {
      const auto& frame = bus.trace[i];
      CHECK(frame.address == 0x4B && frame.timeoutMs > 0 && frame.timeoutMs <= config.i2cTimeoutMs);
      if (i < guards) {
        CHECK(frame.combinedRead && frame.tx == std::vector<std::uint8_t>{4} && frame.rxLen == 2);
      } else {
        const std::uint8_t msb = config.settings.mode == Mode::SHUTDOWN ? 0x06 : 0x02;
        const std::vector<std::uint8_t> expected{1, msb, 0x22};
        CHECK(!frame.combinedRead && frame.tx == expected && frame.rxLen == 0);
      }
    }
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t clockReads = 0;
  std::uint32_t lastTick = 0;
  std::uint32_t idTick = 0;
  std::vector<std::uint32_t> guardTicks;
  bool clearSecondGuard = false;
  bool lateGuard = false;
  bool lateCommand = false;
  const AlertStatus* alertsAtCommand = nullptr;
};

void preflightRefusals() {
  Fixture f;
  CHECK(Reset{f.device}.issueCommand().is(Err::NOT_BOUND));
  CHECK(f.device.bind(f.config).ok());
  CHECK(Reset{f.device}.issueCommand().is(Err::NOT_INITIALIZED));
  f.checkFrames(0, false);
  for (const auto model : {Model::TMP116, Model::TMP116N}) {
    Fixture unsupported{model};
    CHECK(unsupported.device.bind(unsupported.config).ok());
    CHECK(Reset{unsupported.device}.issueCommand().is(Err::UNSUPPORTED));
    unsupported.begin();
    CHECK(Reset{unsupported.device}.issueCommand().is(Err::UNSUPPORTED));
    unsupported.checkFrames(0, false);
  }
  f.begin();
  f.chip.words[2] ^= 1;
  CHECK(f.device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
  f.bus.trace.clear();
  CHECK(Reset{f.device}.issueCommand().is(Err::CONFIG_DIRTY));
  f.checkFrames(0, false);
}

void lockedIdlePreflight() {
  for (const std::uint16_t raw : {0x8000, 0xC000}) {
    Fixture f;
    f.begin();
    f.chip.words[4] = raw;
    CHECK(Reset{f.device}.issueCommand().is(Err::EEPROM_UNSAFE));
    f.chip.words[4] = 0;
    CHECK(Reset{f.device}.issueCommand().is(Err::EEPROM_UNSAFE));
    f.checkFrames(1, false);
    CHECK(f.device.isInitialized());
  }
  Fixture f;
  f.begin();
  f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short guard"), 1});
  CHECK(Reset{f.device}.issueCommand().is(Err::I2C_SHORT_TRANSFER));
  f.checkFrames(1, false);
  CHECK(f.device.isInitialized() && f.device.configTrust() == ConfigTrust::VERIFIED);
  f.bus.trace.clear();
  f.chip.words[4] = 0x4000;
  CHECK(Reset{f.device}.issueCommand().is(Err::OPERATION_TIMEOUT));
  CHECK(!f.bus.trace.empty());
  f.checkFrames(f.bus.trace.size(), false);
  CHECK(f.device.isInitialized() && f.device.configTrust() == ConfigTrust::VERIFIED);
  f.bus.trace.clear();
  f.chip.words[4] = 0;
  f.lateGuard = true;
  const auto health = f.device.healthSnapshot();
  CHECK(Reset{f.device}.issueCommand().is(Err::OPERATION_TIMEOUT));
  f.checkFrames(1, false);
  CHECK(f.bus.trace[0].result.ok() && f.device.healthSnapshot().totalSuccess == health.totalSuccess + 1);
  CHECK(f.device.isInitialized() && f.device.configTrust() == ConfigTrust::VERIFIED);
}

void commandAfterBusy() {
  for (const auto model : {Model::TMP117, Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    for (const auto mode : {Mode::CONTINUOUS, Mode::SHUTDOWN}) {
      Fixture f{model};
      f.config.settings.mode = mode;
      f.begin();
      f.chip.words[4] = 0x4000;
      f.clearSecondGuard = true;
      CHECK(Reset{f.device}.issueCommand().ok());
      f.checkFrames(2, true);
      CHECK(f.guardTicks[1] - f.guardTicks[0] >= 2);
      CHECK(!f.device.isInitialized());
    }
  }
}

void invalidationAtCommand() {
  for (const bool late : {false, true}) {
    Fixture f;
    f.config.settings.limits = {-1280, 4736};
    f.config.settings.managedOffsetCounts = 64;
    f.begin();
    f.chip.words[0] = 0x1280;
    Sample sample;
    CHECK(f.device.readLatest(sample).ok());
    f.chip.words[1] |= 0xA000;  // HIGH and DATA_READY with verified stable settings.
    ConfigurationObservation config;
    CHECK(f.device.readConfiguration(config).ok() && config.high && config.dataReady);
    Apply evidence{f.device};
    CHECK(evidence.readyPending());
    const auto history = f.device.getAlertStatus();
    CHECK(history.pendingHigh && history.firstEvent && !history.possibleLoss && !history.mixedOrUnknown);
    f.alertsAtCommand = &evidence.alerts();
    const auto verified = f.device.getSettingsSnapshot();
    const auto health = f.device.healthSnapshot();
    f.bus.trace.clear();
    f.lateCommand = late;
    if (!late) f.bus.script = {{}, {Status::Error(Err::I2C_NACK_DATA, "ambiguous reset", 7)}};
    const auto result = Reset{f.device}.issueCommand();
    CHECK(late ? result.is(Err::OPERATION_TIMEOUT) : result.is(Err::I2C_NACK_DATA) && result.detail == 7);
    if (!late) CHECK(f.chip.words[3] == 0xF000);
    f.checkFrames(1, true);
    const auto invalid = f.device.getSettingsSnapshot();
    CHECK(f.device.isBound() && !f.device.isInitialized() && invalid.trust == ConfigTrust::UNKNOWN);
    CHECK(!invalid.config && !invalid.high && !invalid.low && !invalid.offset);
    CHECK(invalid.desired.limits.lowCounts == -1280 && invalid.desired.limits.highCounts == 4736);
    CHECK(invalid.desired.managedOffsetCounts == 64 && !evidence.readyPending() && !evidence.baselineMs());
    CHECK(invalid.verifiedAtMs == verified.verifiedAtMs && invalid.verifiedGeneration == verified.verifiedGeneration);
    const auto alerts = f.device.getAlertStatus();
    CHECK(alerts.pendingHigh && alerts.possibleLoss && alerts.firstEvent && !alerts.thermHigh);
    CHECK(alerts.firstEvent->profileGeneration == verified.verifiedGeneration);
    CHECK(alerts.firstEvent->observedAtMs == history.firstEvent->observedAtMs);
    const auto& retained = LatestRead{f.device}.cache;
    CHECK(retained && retained->raw == sample.raw && retained->observedAtMs == sample.observedAtMs);
    CHECK(retained->profileGeneration == verified.verifiedGeneration);
    const auto after = f.device.healthSnapshot();
    CHECK(after.totalSuccess == health.totalSuccess + (late ? 2u : 1u));
    CHECK(after.totalFailures == health.totalFailures + (late ? 0u : 1u));
    CHECK(Reset{f.device}.issueCommand().is(Err::NOT_INITIALIZED));
    f.checkFrames(1, true);
    f.alertsAtCommand = nullptr;
    f.bus.trace.clear();
    f.guardTicks.clear();
    CHECK(f.device.recover().ok());
    CHECK(f.bus.trace[0].tx[0] == 0x0F && f.bus.trace[1].tx[0] == 4);
    CHECK(!f.guardTicks.empty() && f.guardTicks.front() - f.idTick >= 4);
  }
}

}  // namespace

int main() {
  preflightRefusals();
  lockedIdlePreflight();
  commandAfterBusy();
  invalidationAtCommand();
  return EXIT_SUCCESS;
}
