#include "Apply.h"
#include "LatestRead.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
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
  explicit Fixture(Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x4B;
    config.operationTimeoutMs = 64;
    config.i2cUser = this;
    config.timeUser = this;
    config.nowMs = [](void* user) {
      auto& f = *static_cast<Fixture*>(user);
      f.lastTick = f.floorTick();
      if (!f.clockFrozen) ++f.clockReads;
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
      if (f.afterTransfer) f.afterTransfer(f.bus.trace.back());
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
      if (f.afterTransfer) f.afterTransfer(f.bus.trace.back());
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

  std::uint32_t floorTick() const {
    const auto q = config.clockResolutionMs;
    return static_cast<std::uint32_t>((clockReads / 8 / q) * q);
  }

  void checkFrames(std::size_t guards, bool command, const std::vector<std::uint8_t>& post = {}) const {
    CHECK(bus.trace.size() == guards + (command ? 1u : 0u) + post.size());
    for (std::size_t i = 0; i < bus.trace.size(); ++i) {
      const auto& frame = bus.trace[i];
      CHECK(frame.address == 0x4B && frame.timeoutMs > 0 && frame.timeoutMs <= config.i2cTimeoutMs);
      if (i < guards) {
        CHECK(frame.combinedRead && frame.tx == std::vector<std::uint8_t>{4} && frame.rxLen == 2);
      } else if (i == guards && command) {
        const std::uint8_t msb = config.settings.mode == Mode::SHUTDOWN ? 0x06 : 0x02;
        const std::vector<std::uint8_t> expected{1, msb, 0x22};
        CHECK(!frame.combinedRead && frame.tx == expected && frame.rxLen == 0);
      } else {
        CHECK(frame.combinedRead && frame.rxLen == 2);
        CHECK(frame.tx == std::vector<std::uint8_t>{post[i - guards - 1]});
      }
    }
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint64_t clockReads = 0;  // Each clock sample advances 1/8 ms before quantization.
  bool clockFrozen = false;
  std::uint32_t lastTick = 0;
  std::uint32_t idTick = 0;
  std::vector<std::uint32_t> guardTicks;
  bool clearSecondGuard = false;
  bool lateGuard = false;
  bool lateCommand = false;
  const AlertStatus* alertsAtCommand = nullptr;
  std::function<void(const TransferTrace&)> afterTransfer;
};

void preflightRefusals() {
  Fixture f;
  CHECK(f.device.reset().is(Err::NOT_BOUND));
  CHECK(f.device.bind(f.config).ok());
  CHECK(f.device.reset().is(Err::NOT_INITIALIZED));
  f.checkFrames(0, false);
  for (const auto model : {Model::TMP116, Model::TMP116N}) {
    Fixture unsupported{model};
    CHECK(unsupported.device.bind(unsupported.config).ok());
    CHECK(unsupported.device.reset().is(Err::UNSUPPORTED));
    unsupported.begin();
    CHECK(unsupported.device.reset().is(Err::UNSUPPORTED));
    unsupported.checkFrames(0, false);
  }
  f.begin();
  f.chip.words[2] ^= 1;
  CHECK(f.device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
  f.bus.trace.clear();
  CHECK(f.device.reset().is(Err::CONFIG_DIRTY));
  f.checkFrames(0, false);
}

void lockedIdlePreflight() {
  for (const std::uint16_t raw : {0x8000, 0xC000}) {
    Fixture f;
    f.begin();
    f.chip.words[4] = raw;
    CHECK(f.device.reset().is(Err::EEPROM_UNSAFE));
    f.chip.words[4] = 0;
    CHECK(f.device.reset().is(Err::EEPROM_UNSAFE));
    f.checkFrames(1, false);
    CHECK(f.device.isInitialized());
  }
  Fixture f;
  f.begin();
  f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short guard"), 1});
  CHECK(f.device.reset().is(Err::I2C_SHORT_TRANSFER));
  f.checkFrames(1, false);
  CHECK(f.device.isInitialized() && f.device.configTrust() == ConfigTrust::VERIFIED);
  f.bus.trace.clear();
  f.chip.words[4] = 0x4000;
  CHECK(f.device.reset().is(Err::OPERATION_TIMEOUT));
  CHECK(!f.bus.trace.empty());
  f.checkFrames(f.bus.trace.size(), false);
  CHECK(f.device.isInitialized() && f.device.configTrust() == ConfigTrust::VERIFIED);
  f.bus.trace.clear();
  f.chip.words[4] = 0;
  f.lateGuard = true;
  const auto health = f.device.healthSnapshot();
  CHECK(f.device.reset().is(Err::OPERATION_TIMEOUT));
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
      CHECK(f.device.reset().ok());
      f.checkFrames(2, true, {0x0F, 4, 1, 3, 2, 7});
      CHECK(f.guardTicks[1] - f.guardTicks[0] >= 2);
      CHECK(!f.device.isInitialized());
    }
  }
}

void resetUsesVerifiedSettings() {
  Fixture f;
  f.begin();
  CHECK(f.device.setCycle(Cycle::CODE_7).ok());
  CHECK(f.device.setAveraging(Averaging::SAMPLES_64).ok());
  CHECK(f.device.setAlert(AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY).ok());
  // The alternate continuous MOD (10), HIGH and DATA_READY appear in the last read.
  f.chip.words[1] |= 0xA800;
  ConfigurationObservation observed;
  CHECK(f.device.readConfiguration(observed).ok() && observed.raw == 0xABFC);
  CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);
  f.bus.trace.clear();
  CHECK(f.device.reset().ok());
  CHECK(f.bus.trace.size() == 8);
  const auto& command = f.bus.trace[1];
  CHECK(!command.combinedRead && command.address == 0x4B);
  CHECK((command.tx == std::vector<std::uint8_t>{0x01, 0x03, 0xFE}));
}

void resetAfterOwnedOneShot() {
  Fixture f;
  f.config.settings.mode = Mode::SHUTDOWN;
  f.begin();
  f.afterTransfer = [&](const TransferTrace& frame) {
    if (!frame.combinedRead && frame.tx == std::vector<std::uint8_t>{1, 0x0E, 0x20}) {
      for (int i = 0; i < 8; ++i) CHECK(f.chip.addOneShotConversion(128));
    }
  };
  Sample sample;
  CHECK(f.device.readOneShotBlocking(sample, 500).ok() && sample.freshConversion);
  const auto before = f.device.getSettingsSnapshot();
  CHECK(before.config && before.config->mode == ObservedMode::ONE_SHOT && before.trust == ConfigTrust::VERIFIED);
  f.afterTransfer = {};
  f.bus.trace.clear();
  // This addressed-reset scenario preserves the register image. The old trigger
  // no longer qualifies MOD=11 as a match for the desired shutdown profile.
  const auto status = f.device.reset();
  CHECK(status.is(Err::CONFIG_MISMATCH) && status.detail == 1);
  f.checkFrames(1, true, {0x0F, 4, 1, 3, 2, 7});
  CHECK(f.device.configTrust() == ConfigTrust::DIRTY && !f.device.isInitialized());
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
    const auto result = f.device.reset();
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
    CHECK(f.device.reset().is(Err::NOT_INITIALIZED));
    f.checkFrames(1, true);
    f.alertsAtCommand = nullptr;
    f.bus.trace.clear();
    f.guardTicks.clear();
    const auto commandCompleted = late ? after.lastSuccessMs : after.lastErrorMs;
    if (!late) {
      const auto clockReads = f.clockReads;
      CHECK(f.device.end().ok() && f.device.isBound());
      CHECK(f.clockReads == clockReads && f.bus.trace.empty());
    }
    f.clockFrozen = true;
    CHECK(f.device.recover().is(Err::WORK_LIMIT));
    CHECK(f.bus.trace.empty());
    f.clockFrozen = false;
    std::uint32_t idCompleted = 0;
    f.afterTransfer = [&](const TransferTrace& frame) {
      if (frame.tx[0] == 0x0F) {
        f.clockReads += 2 * 8;
        idCompleted = f.floorTick();
      }
    };
    CHECK(f.device.recover().ok());
    CHECK(f.bus.trace[0].tx[0] == 0x0F && f.bus.trace[1].tx[0] == 4);
    CHECK(f.idTick - commandCompleted >= 3);
    CHECK(!f.guardTicks.empty() && f.guardTicks.front() - idCompleted >= 4);
  }
}

void postCommandTiming() {
  for (const auto q : {1u, 2u}) {
    Fixture f;
    f.config.clockResolutionMs = q;
    f.config.i2cTimeoutMs = f.config.operationTimeoutMs;
    // Both operations and callback completion ticks remain in one clock epoch.
    f.clockReads = (0x100000000ULL - 16) * 8;
    f.begin();
    const auto before = f.device.getSettingsSnapshot();
    const auto health = f.device.healthSnapshot();
    std::uint32_t commandCompleted = 0, idCompleted = 0;
    f.afterTransfer = [&](const TransferTrace& transfer) {
      if (!transfer.combinedRead || transfer.tx[0] == 0x0F) {
        // The command and ID take multiple ticks. Start each wait at return.
        f.clockReads += 3 * q * 8;
        const auto tick = f.floorTick();
        if (transfer.combinedRead) idCompleted = tick;
        else commandCompleted = tick;
      }
    };
    CHECK(f.device.reset().ok());
    f.checkFrames(1, true, {0x0F, 4, 1, 3, 2, 7});
    CHECK(f.idTick - commandCompleted == (q == 1 ? 3u : 4u));
    CHECK(f.guardTicks[1] - idCompleted == (q == 1 ? 4u : 6u));
    CHECK(f.bus.trace[2].timeoutMs < f.bus.trace[1].timeoutMs);
    CHECK(f.bus.trace[3].timeoutMs < f.bus.trace[2].timeoutMs);
    const auto observed = f.device.getSettingsSnapshot();
    CHECK(observed.config && observed.config->raw == f.chip.words[1]);
    CHECK(observed.low && observed.low->raw == f.chip.words[3]);
    CHECK(observed.high && observed.high->raw == f.chip.words[2]);
    CHECK(observed.offset && observed.offset->raw == f.chip.words[7]);
    CHECK(observed.verifiedAtMs == before.verifiedAtMs && observed.verifiedGeneration == before.verifiedGeneration);
    CHECK(!f.device.isInitialized() && observed.trust == ConfigTrust::UNKNOWN);
    CHECK(observed.verificationReads == 0x0F);
    CHECK(f.device.healthSnapshot().totalSuccess == health.totalSuccess + 8);
  }
}

void resetWaitAfterRebind() {
  for (const bool unbind : {false, true}) {
    Fixture f;
    f.begin();
    f.bus.script = {{}, {Status::Error(Err::I2C_NACK_DATA, "ambiguous reset")}};
    CHECK(f.device.reset().is(Err::I2C_NACK_DATA));
    f.bus.trace.clear();
    if (unbind) {
      CHECK(f.device.unbind().ok());
      // Another address can be probed by begin() while the first part is resetting.
      auto other = f.config;
      other.i2cAddress = 0x48;
      f.clockFrozen = true;
      CHECK(f.device.begin(other).is(Err::I2C_NACK_ADDR));
      CHECK(f.bus.trace.size() == 1 && f.bus.trace[0].address == 0x48);
      f.bus.trace.clear();
    }
    // Explicit rebind can replace the clock epoch, even with the same callback.
    f.config.clockResolutionMs = 2;
    f.clockReads = 0;
    f.clockFrozen = true;
    CHECK(f.device.begin(f.config).is(Err::WORK_LIMIT));
    CHECK(f.bus.trace.empty());
    f.clockFrozen = false;
    f.guardTicks.clear();
    CHECK(f.device.recover().ok());
    CHECK(f.idTick >= 4 && !f.guardTicks.empty() && f.guardTicks[0] - f.idTick >= 6);
  }
}

void waitAtDeadline() {
  for (const bool atId : {false, true}) {
    Fixture f;
    f.config.clockResolutionMs = 2;
    f.begin();
    const auto origin = ((f.clockReads + 15) / 16) * 2;
    f.clockReads = origin * 8;
    const auto end = origin + f.config.operationTimeoutMs;
    f.afterTransfer = [&](const TransferTrace& frame) {
      // Leave precisely the required rounded wait on the original budget.
      if (!atId && !frame.combinedRead) f.clockReads = (end - 4) * 8;
      if (atId && frame.tx[0] == 0x0F) f.clockReads = (end - 6) * 8;
    };
    CHECK(f.device.reset().is(Err::OPERATION_TIMEOUT));
    f.checkFrames(1, true, atId ? std::vector<std::uint8_t>{0x0F} : std::vector<std::uint8_t>{});
    CHECK(!f.device.isInitialized());
    // An unfinished wait leaves no startup proof for an explicit recovery.
    std::uint32_t idCompleted = 0;
    f.afterTransfer = [&](const TransferTrace& frame) {
      if (frame.tx[0] == 0x0F) {
        f.clockReads += 2 * 8;
        idCompleted = f.floorTick();
      }
    };
    f.bus.trace.clear();
    f.guardTicks.clear();
    CHECK(f.device.recover().ok());
    CHECK(f.bus.trace[0].combinedRead && f.bus.trace[0].tx[0] == 0x0F);
    if (!atId) {
      const auto commandCompleted = static_cast<std::uint32_t>(end - 4);
      CHECK(f.idTick - commandCompleted >= 4);
    }
    CHECK(!f.guardTicks.empty() && f.guardTicks.front() - idCompleted >= 6);
  }
}

void postCommandSafety() {
  Fixture mismatch;
  mismatch.begin();
  mismatch.chip.words[0x0F] = 0x1116;
  const auto identity = mismatch.device.reset();
  CHECK(identity.is(Err::IDENTITY_MISMATCH) && identity.detail == 0x1116);
  mismatch.checkFrames(1, true, {0x0F});
  CHECK(!mismatch.device.isInitialized());

  for (const std::uint16_t unsafe : {0x8000, 0xC000}) {
    Fixture f;
    f.begin();
    f.afterTransfer = [&](const TransferTrace& frame) {
      if (!frame.combinedRead) f.chip.words[4] = unsafe;
    };
    CHECK(f.device.reset().is(Err::EEPROM_UNSAFE));
    CHECK(!f.device.isInitialized());
    f.chip.words[4] = 0;
    CHECK(f.device.recover().is(Err::EEPROM_UNSAFE));
    f.checkFrames(1, true, {0x0F, 4});
  }

  Fixture busy;
  busy.begin();
  busy.afterTransfer = [&](const TransferTrace& frame) {
    if (!frame.combinedRead) busy.chip.words[4] = 0x4000;
  };
  CHECK(busy.device.reset().is(Err::OPERATION_TIMEOUT));
  CHECK(busy.guardTicks.size() > 2);
  std::vector<std::uint8_t> postBusy(busy.guardTicks.size(), 4);
  postBusy.front() = 0x0F;
  busy.checkFrames(1, true, postBusy);
  CHECK(!busy.device.isInitialized() && !busy.device.getSettingsSnapshot().config);

  Fixture f;
  f.begin();
  std::uint32_t configTick = 0;
  f.afterTransfer = [&](const TransferTrace& frame) {
    if (!frame.combinedRead) f.chip.words[4] = 0x4000;
    if (f.bus.trace.size() == 4) f.chip.words[4] = 0;
    if (f.bus.trace.size() == 5) f.chip.words[1] |= 0xB000;  // HIGH, READY and CONFIG BUSY.
    if (frame.combinedRead && frame.tx[0] == 1 && (f.chip.words[1] & 0x1000)) {
      configTick = f.lastTick;
      f.chip.words[1] &= 0xEFFF;
    }
  };
  CHECK(f.device.reset().ok());
  f.checkFrames(1, true, {0x0F, 4, 4, 1, 4, 1, 3, 2, 7});
  CHECK(f.guardTicks[2] - f.guardTicks[1] >= 2 && f.guardTicks[3] - configTick >= 2);
  const auto alerts = f.device.getAlertStatus();
  CHECK(alerts.pendingHigh && alerts.firstEvent && alerts.mixedOrUnknown);
  CHECK(alerts.firstEvent->source == ConfigReadSource::RESET && !Apply{f.device}.readyPending());
}

void partialPostCommandReads() {
  const std::vector<std::uint8_t> post{0x0F, 4, 1, 3, 2, 7};
  for (std::size_t step = 0; step < post.size(); ++step) {
    for (const bool late : {false, true}) {
      Fixture f;
      f.begin();
      const auto health = f.device.healthSnapshot();
      f.chip.words[1] |= 0xA000;
      if (late) {
        f.afterTransfer = [&](const TransferTrace&) {
          if (f.bus.trace.size() == 3 + step) f.clockReads += f.config.operationTimeoutMs * 8;
        };
      } else {
        f.bus.script.resize(2 + step);
        f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "partial observation", 1), 1, true});
      }
      const auto result = f.device.reset();
      CHECK(late ? result.is(Err::OPERATION_TIMEOUT) : result.is(Err::I2C_SHORT_TRANSFER) && result.detail == 1);
      f.checkFrames(1, true, {post.begin(), post.begin() + step + 1});
      const auto after = f.device.healthSnapshot();
      CHECK(after.totalSuccess == health.totalSuccess + 2 + step + (late ? 1u : 0u));
      CHECK(after.totalFailures == health.totalFailures + (late ? 0u : 1u));
      const auto observed = f.device.getSettingsSnapshot();
      const auto completed = step + (late ? 1u : 0u);
      CHECK(observed.config.has_value() == (completed > 2));
      CHECK(observed.low.has_value() == (completed > 3));
      CHECK(observed.high.has_value() == (completed > 4));
      CHECK(observed.offset.has_value() == (completed > 5));
      constexpr std::uint8_t verificationMask[] = {0, 0, 0, 1, 3, 7, 15};
      CHECK(observed.verificationReads == verificationMask[completed]);
      CHECK(!f.device.isInitialized() && observed.trust != ConfigTrust::VERIFIED);
      if (observed.config) {
        CHECK(observed.config->high && observed.config->dataReady && observed.config->timestampValid);
        CHECK(f.device.getAlertStatus().pendingHigh && !Apply{f.device}.readyPending());
      }
    }
  }
}

void resetDispatchLimit() {
  // With 249 preflight guards, OFFSET is the 256th and final dispatch.
  // With 255 guards the command itself consumes the last dispatch.
  for (const std::size_t guards : {249, 250, 255}) {
    Fixture f;
    f.config.operationTimeoutMs = 2000;
    f.begin();
    f.chip.words[4] = 0x4000;
    f.afterTransfer = [&](const TransferTrace&) {
      if (f.bus.trace.size() == guards - 1) f.chip.words[4] = 0;
    };
    const auto result = f.device.reset();
    CHECK(guards == 249 ? result.ok() : result.is(Err::WORK_LIMIT));
    std::vector<std::uint8_t> post{0x0F, 4, 1, 3, 2, 7};
    post.resize(256 - guards - 1);
    f.checkFrames(guards, true, post);
    CHECK(!f.device.isInitialized());
  }
}

}  // namespace

int main() {
  preflightRefusals();
  lockedIdlePreflight();
  commandAfterBusy();
  resetUsesVerifiedSettings();
  resetAfterOwnedOneShot();
  invalidationAtCommand();
  postCommandTiming();
  resetWaitAfterRebind();
  waitAtDeadline();
  postCommandSafety();
  partialPostCommandReads();
  resetDispatchLimit();
  return EXIT_SUCCESS;
}
