#include "Apply.h"
#include "DeviceEvidence.h"
#include "LatestRead.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::detail;
using namespace ::TMP11x::test;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);      \
      std::exit(EXIT_FAILURE);                                                  \
    }                                                                           \
  } while (false)

struct Fixture {
  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t now = 0;
  std::uint32_t tickIncrement = 1;
  std::function<void(bool, std::uint8_t)> beforeTransfer;

  explicit Fixture(Model model = Model::TMP117) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x49;
    config.i2cUser = this;
    config.timeUser = this;
    config.nowMs = [](void* user) {
      auto& f = *static_cast<Fixture*>(user);
      const auto tick = f.now;
      f.now += f.tickIncrement;
      return tick;
    };
    config.i2cWriteRead = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                             std::uint8_t* rx, std::size_t m, std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      CHECK(address == f.config.i2cAddress && n == 1 && m == 2 && ms != 0);
      CHECK(*tx <= 0x04 || *tx == 0x0F);
      if (f.beforeTransfer) f.beforeTransfer(true, *tx);
      return FakeTmp11xBus::writeRead(address, tx, n, rx, m, ms, &f.bus);
    };
    config.i2cWrite = [](std::uint8_t address, const std::uint8_t* tx, std::size_t n,
                         std::uint32_t ms, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      CHECK(address == f.config.i2cAddress && n == 3 && ms != 0);
      CHECK(*tx >= 0x01 && *tx <= 0x03);
      if (*tx == 0x01) CHECK((tx[1] & 0xF8) == 0 && (tx[2] & 0x03) == 0);
      if (f.beforeTransfer) f.beforeTransfer(false, *tx);
      return FakeTmp11xBus::write(address, tx, n, ms, &f.bus);
    };
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0xA117;
    chip.words[0x00] = 0x0C80;
    chip.words[0x07] = 0xA35C;
    chip.words[0x02] = 0x0500;
    config.settings.limits = {0, 0x0500};
    CHECK(bus.attach(config.i2cAddress, chip));
  }
};

std::string transcript(const Fixture& f) {
  std::string result;
  for (const auto& frame : f.bus.trace) {
    char token[16]{};
    if (frame.combinedRead) {
      std::snprintf(token, sizeof(token), "R%02X", static_cast<unsigned>(frame.tx[0]));
    } else {
      std::snprintf(token, sizeof(token), "W%02X=%02X%02X", static_cast<unsigned>(frame.tx[0]),
                    static_cast<unsigned>(frame.tx[1]), static_cast<unsigned>(frame.tx[2]));
    }
    if (!result.empty()) result += ' ';
    result += token;
  }
  return result;
}

constexpr char normal[] =
    "R0F R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 R01 W01=0220 R01";

void allModelsBeginRecoverLatest() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117, Model::TMP117M,
                           Model::TMP117N, Model::TMP119}) {
    Fixture f{model};
    f.chip.words[1] = 0xE220;  // Factory-style CC plus old HIGH/LOW/ready.
    CHECK(f.device.begin(f.config).ok());
    CHECK(f.device.isInitialized() && f.device.isOnline());
    CHECK(transcript(f) == normal);
    CHECK(f.device.identityInfo().compatible);
    CHECK(!LatestRead{f.device}.cache.has_value());
    Sample sample;
    CHECK(f.device.readLatest(sample).ok() && sample.raw == 0x0C80);
    CHECK(!sample.freshConversion && sample.acquisitionKind == ReadKind::LATEST);
    CHECK(sample.offset.state == OffsetState::UNKNOWN && !sample.offset.countsValid);
    CHECK(f.device.healthSnapshot().totalSuccess == f.bus.trace.size());
    CHECK(f.bus.trace.back().combinedRead && f.bus.trace.back().tx[0] == 0x00);
    const auto generation = sample.profileGeneration;
    f.bus.trace.clear();
    CHECK(f.device.recover().ok() && transcript(f) == normal);
    CHECK(f.device.readLatest(sample).ok() && sample.profileGeneration > generation);
    CHECK(!sample.freshConversion && f.chip.words[0x07] == 0xA35C);
  }
}

void admissionPreservesBinding() {
  Fixture f;
  CHECK(f.device.begin().is(Err::NOT_BOUND) && f.device.recover().is(Err::NOT_BOUND));
  CHECK(f.device.begin(f.config).ok());
  const auto transfers = f.bus.trace.size();
  for (unsigned i = 0; i < 7; ++i) {
    auto invalid = f.config;
    invalid.i2cAddress = 0x48;
    auto& s = invalid.settings;
    switch (i) {
      case 0: s.mode = static_cast<Mode>(2); break;
      case 1: s.cycle = static_cast<Cycle>(8); break;
      case 2: s.averaging = static_cast<Averaging>(4); break;
      case 3: s.alertMode = static_cast<AlertMode>(2); break;
      case 4: s.polarity = static_cast<Polarity>(2); break;
      case 5: s.pinFunction = static_cast<PinFunction>(2); break;
      case 6: s.alertMode = AlertMode::THERM; s.limits = {1, 0}; break;
    }
    CHECK(f.device.bind(invalid).is(Err::INVALID_CONFIG));
    CHECK(f.device.begin(invalid).is(Err::INVALID_CONFIG));
  }
  auto invalid = f.config;
  invalid.nowMs = nullptr;
  CHECK(f.device.begin(invalid).is(Err::CLOCK_REQUIRED));
  invalid = f.config;
  invalid.clockResolutionMs = 10;
  invalid.operationTimeoutMs = 9;
  CHECK(f.device.begin(invalid).is(Err::INVALID_CONFIG));
  CHECK(f.device.identityInfo().i2cAddress == 0x49 && f.device.identityInfo().rawValid);
  CHECK(f.device.isInitialized() && f.device.desiredSettings().limits.lowCounts == 0);
  CHECK(f.bus.trace.size() == transfers);

  auto replacement = f.config;
  replacement.settings.limits.lowCounts = -42;
  f.chip.words[0x0F] = 0x1116;
  CHECK(f.device.begin(replacement).is(Err::IDENTITY_MISMATCH));
  CHECK(f.bus.trace.size() == transfers + 1 && f.bus.trace.back().tx[0] == 0x0F);
  CHECK(!f.device.isInitialized() && f.device.desiredSettings().limits.lowCounts == -42);
  CHECK(f.device.healthSnapshot().totalSuccess == 1 && f.device.healthSnapshot().totalFailures == 0);
}

Status oneStep(Apply& apply, Fixture& f) {
  const auto count = f.bus.trace.size();
  const auto status = apply.step();
  CHECK(f.bus.trace.size() - count <= 1);
  return status;
}

// Step through any bus-silent transitions until the next callback or terminal result.
Status nextTransfer(Apply& apply, Fixture& f) {
  const auto count = f.bus.trace.size();
  for (unsigned i = 0; i < 32; ++i) {
    const auto status = oneStep(apply, f);
    if (!status.inProgress() || f.bus.trace.size() != count) return status;
  }
  CHECK(false);
  return {};
}

void startupFromCompatibleCompletion() {
  for (const std::uint32_t q : {1u, 10u}) {
    Fixture f{Model::TMP116};
    f.config.clockResolutionMs = q;
    f.tickIncrement = 0;
    CHECK(f.device.bind(f.config).ok());  // Device power can arrive long after bind.
    f.now = 30;
    CHECK(f.device.probe().ok());         // A diagnostic ACK does not start the wait.
    f.bus.trace.clear();
    f.now = 40;
    Apply apply{f.device, f.config.settings};
    CHECK(apply.start().inProgress() && f.bus.trace.empty());
    f.beforeTransfer = [&](bool, std::uint8_t pointer) {
      if (pointer == 0x0F) f.now = 50;
    };
    CHECK(nextTransfer(apply, f).inProgress() && transcript(f) == "R0F");
    const auto due = q == 1 ? 54u : 70u;
    f.now = due - q;
    for (unsigned i = 0; i < 4; ++i) CHECK(oneStep(apply, f).inProgress());
    CHECK(transcript(f) == "R0F" && f.chip.words[4] == 0);
    f.now = due;
    CHECK(nextTransfer(apply, f).inProgress() && transcript(f) == "R0F R04");
    auto status = oneStep(apply, f);
    while (status.inProgress()) status = oneStep(apply, f);
    CHECK(status.ok() && transcript(f) == normal);
    CHECK(f.device.end().ok());
    Apply renewed{f.device, f.config.settings};
    f.bus.trace.clear();
    f.now = 100;
    f.beforeTransfer = {};
    CHECK(renewed.start().inProgress() && nextTransfer(renewed, f).inProgress());
    f.now += q;
    CHECK(oneStep(renewed, f).inProgress() && transcript(f) == "R0F");
  }
}

void busyAndUnlockedGuards() {
  for (const auto word : {0x8000u, 0xC000u}) {
    Fixture f;
    f.config.settings.mode = Mode::SHUTDOWN;
    f.chip.words[4] = static_cast<std::uint16_t>(word);
    CHECK(f.device.begin(f.config).is(Err::EEPROM_UNSAFE));
    CHECK(transcript(f) == "R0F R04");
    f.chip.words[4] = 0;
    f.bus.trace.clear();
    CHECK(f.device.recover().is(Err::EEPROM_UNSAFE));
    CHECK(f.device.bind(f.config).ok());
    CHECK(f.device.begin().is(Err::EEPROM_UNSAFE));
    for (const auto& frame : f.bus.trace) CHECK(frame.combinedRead);
  }
  Fixture f;
  f.chip.words[4] = 0x4000;
  f.config.operationTimeoutMs = 45;
  CHECK(f.device.begin(f.config).is(Err::OPERATION_TIMEOUT));
  CHECK(f.bus.trace.size() >= 3 && f.bus.trace.front().tx[0] == 0x0F);
  for (std::size_t i = 1; i < f.bus.trace.size(); ++i) {
    CHECK(f.bus.trace[i].combinedRead && f.bus.trace[i].tx[0] == 4);
  }
  const auto& last = f.bus.trace.back();
  CHECK(last.timeoutMs < 45 && f.device.healthSnapshot().totalFailures == 0);
  f.bus.trace.clear();
  f.beforeTransfer = [&](bool read, std::uint8_t ptr) {
    if (read && ptr == 4) f.bus.script.push_back({Status::Error(Err::I2C_BUS, "guard", 17)});
  };
  CHECK(f.device.recover().is(Err::I2C_BUS) && transcript(f) == "R0F R04");
  CHECK(f.device.healthSnapshot().lastError.detail == 17);
}

void unlockedGuardAtDeadline() {
  Fixture f;
  const auto deadline = f.now + f.config.operationTimeoutMs;
  f.chip.words[4] = 0xC000;
  f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
    if (read && pointer == 0x04) f.now = deadline;
  };
  CHECK(f.device.begin(f.config).is(Err::OPERATION_TIMEOUT));
  CHECK(transcript(f) == "R0F R04" && !f.device.isInitialized());
  const auto health = f.device.healthSnapshot();
  CHECK(health.totalSuccess == 2 && health.totalFailures == 0);
  CHECK(health.lastSuccessMsValid && health.lastSuccessMs == deadline);

  f.chip.words[4] = 0;
  f.bus.trace.clear();
  CHECK(f.device.recover().is(Err::EEPROM_UNSAFE) && f.bus.trace.empty());
}

void recoveryStartupProof() {
  Fixture f;
  f.tickIncrement = 0;
  CHECK(f.device.bind(f.config).ok());
  Apply expired{f.device};
  CHECK(expired.start().inProgress() && oneStep(expired, f).inProgress());
  f.now = f.config.operationTimeoutMs;
  CHECK(oneStep(expired, f).is(Err::OPERATION_TIMEOUT) && transcript(f) == "R0F");

  f.bus.trace.clear();
  Apply retry{f.device};
  CHECK(retry.start().inProgress() && oneStep(retry, f).inProgress());
  f.now += 3;
  CHECK(oneStep(retry, f).inProgress() && transcript(f) == "R0F");
  ++f.now;
  CHECK(oneStep(retry, f).inProgress() && transcript(f) == "R0F R04");
  auto status = oneStep(retry, f);
  while (status.inProgress()) status = oneStep(retry, f);
  CHECK(status.ok() && transcript(f) == normal);

  // With startup proof established, recover can finish at the same tick.
  f.bus.trace.clear();
  CHECK(f.device.recover().ok() && transcript(f) == normal);
}

void recoveryAfterReportedReset() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  const auto generation = f.device.getSettingsSnapshot().verifiedGeneration;
  const Apply view{f.device};
  ConfigurationObservation observed;
  f.chip.words[1] |= 0x2000;
  CHECK(f.device.readConfiguration(observed).ok() && view.readyPending());
  f.bus.trace.clear();
  CHECK(f.device.invalidateDeviceState().ok());
  CHECK(!view.readyPending() && !view.baselineMs() && !f.device.isInitialized());
  Sample sample;
  CHECK(f.device.readLatest(sample).is(Err::NOT_INITIALIZED) && f.bus.trace.empty());

  f.chip.words[4] = 0x4000;  // Still loading after power returns.
  f.tickIncrement = 0;
  f.now += 100;
  Apply replay{f.device};
  CHECK(replay.start().inProgress() && f.bus.trace.empty());
  f.beforeTransfer = [&](bool, std::uint8_t pointer) {
    if (pointer == 0x0F) f.now += 10;  // FIRST_ACK is at completion.
  };
  CHECK(oneStep(replay, f).inProgress() && transcript(f) == "R0F");
  f.now += 3;
  CHECK(oneStep(replay, f).inProgress() && transcript(f) == "R0F");
  ++f.now;
  CHECK(oneStep(replay, f).inProgress() && transcript(f) == "R0F R04");
  f.chip.words[4] = 0;
  ++f.now;
  CHECK(oneStep(replay, f).inProgress() && transcript(f) == "R0F R04");
  ++f.now;
  CHECK(oneStep(replay, f).inProgress() && transcript(f) == "R0F R04 R04");
  auto status = oneStep(replay, f);
  while (status.inProgress()) status = oneStep(replay, f);
  CHECK(status.ok() && f.device.isInitialized());
  CHECK(transcript(f) ==
        "R0F R04 R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 R01 W01=0220 R01");
  CHECK(f.device.getSettingsSnapshot().verifiedGeneration == generation + 1);
}

void busyShutdownReadbackRequiresNewGuard() {
  Fixture f;
  f.beforeTransfer = [&](bool read, std::uint8_t ptr) {
    if (read && ptr == 1 && f.bus.trace.size() == 6) {
      f.chip.words[1] |= 0xB000;  // HIGH, DATA_READY and BUSY on SD verification.
      f.chip.words[4] = 0xC000;   // Renewal observes EUN and BUSY together.
    }
  };
  CHECK(f.device.begin(f.config).is(Err::EEPROM_UNSAFE));
  CHECK(transcript(f) == "R0F R04 R01 R03 R02 W01=0620 R01 R04");
  Apply view{f.device};
  CHECK(view.alerts().pendingHigh && !view.baselineMs());
  CHECK(!f.device.isInitialized() && f.device.configTrust() == ConfigTrust::DIRTY);
}

void busyRecheckAndFinalBusy() {
  Fixture f;
  unsigned guards = 0;
  bool afterFinalWrite = false;
  bool finalBusy = false;
  std::uint32_t firstBusyCompletion = 0;
  std::uint32_t finalBusyCompletion = 0;
  f.beforeTransfer = [&](bool read, std::uint8_t ptr) {
    if (read && ptr == 4) {
      ++guards;
      f.chip.words[4] = guards == 1 ? 0x4000 : 0x3FFF;
      if (guards == 1) firstBusyCompletion = f.now;
      if (guards == 2) CHECK(f.now - firstBusyCompletion >= 2);
      if (guards == 3) CHECK(f.now - finalBusyCompletion >= 2);
    }
    if (!read && ptr == 1 && f.chip.words[1] == 0x0620) afterFinalWrite = true;
    if (afterFinalWrite && read && ptr == 1) {
      if (!finalBusy) {
        f.chip.words[1] |= 0xB000;
        finalBusyCompletion = f.now;
        finalBusy = true;
      } else {
        f.chip.words[1] &= 0xEFFF;
      }
    }
  };
  CHECK(f.device.begin(f.config).ok() && guards == 3 && finalBusy);
  CHECK(transcript(f) ==
        "R0F R04 R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 R01 W01=0220 R01 R04 R01");
  Apply view{f.device, f.config.settings};
  CHECK(view.alerts().pendingHigh);
}

void comparatorTransitions() {
  Fixture up;
  up.chip.words[1] = 0x0230;
  up.config.settings.alertMode = AlertMode::THERM;
  up.config.settings.limits = {0x0780, 0x0A00};
  CHECK(up.device.begin(up.config).ok());
  CHECK(transcript(up) ==
        "R0F R04 R01 R03 R02 W01=0630 R01 W02=0A00 R02 W03=0780 R03 R01 W01=0230 R01");
  Fixture down;
  down.chip.words[1] = 0x0230;
  down.chip.words[2] = 0x0A00;
  down.chip.words[3] = 0x0780;
  down.config.settings.alertMode = AlertMode::THERM;
  CHECK(down.device.begin(down.config).ok());
  CHECK(transcript(down) ==
        "R0F R04 R01 R03 R02 W01=0630 R01 W03=0000 R03 W02=0500 R02 R01 W01=0230 R01");
  Fixture entering;
  entering.chip.words[1] = 0x0220;
  entering.chip.words[3] = 0x0A00;  // Inverted alert pair [20, 10].
  entering.config.settings.alertMode = AlertMode::THERM;
  CHECK(entering.device.begin(entering.config).ok());
  CHECK(transcript(entering) ==
        "R0F R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 W01=0630 R01 W01=0230 R01");

  for (const auto failure : {Err::OK, Err::I2C_SHORT_TRANSFER, Err::CONFIG_MISMATCH,
                             Err::OPERATION_TIMEOUT}) {
    Fixture leaving;
    leaving.chip.words[1] = 0x8230;
    leaving.config.settings.limits = {0x0A00, 0x0500};
    leaving.beforeTransfer = [&](bool, std::uint8_t) {
      if (leaving.bus.trace.size() == 5 && failure == Err::CONFIG_MISMATCH) {
        TransferOutcome ignored;
        ignored.ignoreSuccessfulWrite = true;
        leaving.bus.script.push_back(ignored);
      }
      if (leaving.bus.trace.size() == 6) {
        if (failure == Err::I2C_SHORT_TRANSFER) {
          leaving.bus.script.push_back({Status::Error(failure, "short SD verification"), 1, true});
        }
        if (failure == Err::OPERATION_TIMEOUT) leaving.now = leaving.config.operationTimeoutMs;
      }
    };
    CHECK(leaving.device.begin(leaving.config).is(failure));
    const std::string prefix = "R0F R04 R01 R03 R02 W01=0620 R01";
    if (failure == Err::OK) {
      CHECK(transcript(leaving) == prefix +
            " W02=0500 R02 W03=0A00 R03 R01 W01=0220 R01");
    } else {
      CHECK(transcript(leaving) == prefix);
      CHECK(leaving.chip.words[3] == 0 && leaving.device.configTrust() == ConfigTrust::DIRTY);
    }
  }
}

void signedThermLimitOrderAcrossZero() {
  Fixture f;
  f.chip.words[1] = 0x0230;
  f.chip.words[3] = 0xFF80;  // -128 counts
  f.chip.words[2] = 0xFFFF;  // -1 count
  f.config.settings.alertMode = AlertMode::THERM;
  f.config.settings.limits = {0, 1};
  CHECK(f.device.begin(f.config).ok());
  CHECK(transcript(f) ==
        "R0F R04 R01 R03 R02 W01=0630 R01 W02=0001 R02 W03=0000 R03 R01 W01=0230 R01");
  CHECK(f.chip.words[3] == 0 && f.chip.words[2] == 1);
}

void ignoredLimitCannotInvertThermPair() {
  for (const bool raising : {false, true}) {
    Fixture f;
    f.chip.words[1] = 0x8230;
    f.config.settings.alertMode = AlertMode::THERM;
    if (raising) {
      f.config.settings.limits = {0x0780, 0x0A00};
    } else {
      f.chip.words[2] = 0x0A00;
      f.chip.words[3] = 0x0780;
    }
    const auto previousLow = f.chip.words[3];
    const auto previousHigh = f.chip.words[2];
    f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
      if (!read && pointer != 1) {
        TransferOutcome ignored;
        ignored.ignoreSuccessfulWrite = true;
        f.bus.script.push_back(ignored);
      }
    };
    CHECK(f.device.begin(f.config).is(Err::CONFIG_MISMATCH));
    const std::string prefix = "R0F R04 R01 R03 R02 W01=0630 R01 ";
    CHECK(transcript(f) == prefix + (raising ? "W02=0A00 R02" : "W03=0000 R03"));
    CHECK(f.chip.words[3] == previousLow && f.chip.words[2] == previousHigh);
    Apply view{f.device, f.config.settings};
    CHECK(view.alerts().thermHigh == true && !view.alerts().pendingHigh);
  }
}

void ignoredLimitPreventsThermEntry() {
  for (const auto ignoredPointer : {0x03, 0x02}) {
    Fixture f;
    f.chip.words[1] = 0x0220;
    f.chip.words[3] = 0x0A00;  // ALERT permits the initial [20, 10] pair.
    f.config.settings.alertMode = AlertMode::THERM;
    f.config.settings.limits = {0, 0x0400};  // Both words must change.
    f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
      if (!read && pointer == ignoredPointer) {
        TransferOutcome ignored;
        ignored.ignoreSuccessfulWrite = true;
        f.bus.script.push_back(ignored);
      }
    };
    CHECK(f.device.begin(f.config).is(Err::CONFIG_MISMATCH));
    const std::string first = "R0F R04 R01 R03 R02 W01=0620 R01 W03=0000 R03";
    CHECK(transcript(f) == (ignoredPointer == 0x03 ? first : first + " W02=0400 R02"));
    CHECK(f.chip.words[1] == 0x0620 && f.chip.words[2] == 0x0500);
    CHECK(f.chip.words[3] == (ignoredPointer == 0x03 ? 0x0A00 : 0));
    CHECK(!f.device.isInitialized() && f.device.configTrust() == ConfigTrust::DIRTY);
  }
}

void stagedThermLimitsAndPartialRecovery() {
  Fixture f;
  f.config.settings.alertMode = AlertMode::THERM;
  CHECK(f.device.begin(f.config).ok());
  const auto verified = f.device.getSettingsSnapshot();
  auto candidate = f.config.settings;
  candidate.limits = {0x0780, 0x0A00};
  f.tickIncrement = 0;
  f.bus.trace.clear();
  Apply expiring{f.device, candidate};
  CHECK(expiring.start().inProgress());
  for (unsigned i = 0; i < 5; ++i) CHECK(nextTransfer(expiring, f).inProgress());
  f.now += f.config.operationTimeoutMs;
  CHECK(oneStep(expiring, f).is(Err::OPERATION_TIMEOUT));
  CHECK(transcript(f) == "R0F R04 R01 R03 R02");
  CHECK(f.device.desiredSettings().limits.lowCounts == 0 &&
        f.device.desiredSettings().limits.highCounts == 0x0500);
  CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);

  f.bus.trace.clear();
  Apply change{f.device, candidate};
  f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
    if (!read && pointer == 0x03) {
      TransferOutcome partial;
      partial.status = Status::Error(Err::I2C_NACK_DATA, "partial TLOW write");
      partial.failedWriteWord = 0x0700;
      f.bus.script.push_back(partial);
    }
  };
  auto status = change.start();
  while (status.inProgress()) status = oneStep(change, f);
  CHECK(status.is(Err::I2C_NACK_DATA));
  CHECK(transcript(f) == "R0F R04 R01 R03 R02 W01=0630 R01 W02=0A00 R02 W03=0780");
  CHECK(f.chip.words[1] == 0x0630 && f.chip.words[2] == 0x0A00 && f.chip.words[3] == 0x0700);
  CHECK(f.device.desiredSettings().limits.lowCounts == 0x0780 &&
        f.device.desiredSettings().limits.highCounts == 0x0A00);
  const auto dirty = f.device.getSettingsSnapshot();
  CHECK(dirty.trust == ConfigTrust::DIRTY && dirty.dirtyReason == DirtyReason::AMBIGUOUS_WRITE);
  CHECK(!dirty.low && dirty.high && dirty.high->raw == 0x0A00 && !change.baselineMs());
  CHECK(dirty.verifiedGeneration == verified.verifiedGeneration && dirty.verifiedAtMs == verified.verifiedAtMs);
  f.beforeTransfer = {};
  f.bus.trace.clear();
  CHECK(f.device.recover().ok());
  CHECK(transcript(f) ==
        "R0F R04 R01 R03 R02 W01=0630 R01 W03=0780 R03 W02=0A00 R02 R01 W01=0230 R01");
  CHECK(f.chip.words[3] == 0x0780 && f.device.configTrust() == ConfigTrust::VERIFIED);
  CHECK(f.device.getSettingsSnapshot().verifiedGeneration == verified.verifiedGeneration + 1);
}

void pinOnlyChangeUsesSameGuardedReplay() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  f.bus.trace.clear();
  auto candidate = f.config.settings;
  candidate.pinFunction = PinFunction::DATA_READY;
  candidate.polarity = Polarity::ACTIVE_HIGH;
  Apply change{f.device, candidate};
  auto status = change.start();
  CHECK(status.inProgress() && f.bus.trace.empty());
  while (status.inProgress()) status = oneStep(change, f);
  CHECK(status.ok() && transcript(f) ==
        "R0F R04 R01 R03 R02 W01=062C R01 W03=0000 R03 W02=0500 R02 R01 W01=022C R01");
}

void stagedIntentAndIndependentReadback() {
  for (const auto failureAt : {1u, 5u, 7u, 8u, 10u}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    CHECK(f.device.verifyConfiguration().ok());
    CHECK(f.device.getSettingsSnapshot().verificationReads == 7);
    f.bus.trace.clear();
    auto candidate = f.config.settings;
    candidate.limits = {0x0101, 0x0402};
    Apply staged{f.device, candidate};
    CHECK(staged.start().inProgress() && f.bus.trace.empty());
    f.beforeTransfer = [&](bool, std::uint8_t pointer) {
      if (f.bus.trace.size() != failureAt) return;
      if (failureAt < 8) {
        TransferOutcome error;
        error.status = Status::Error(Err::I2C_NACK_DATA, "partial transfer", -3);
        if (failureAt == 5) error.failedWriteWord = 0x0620;
        if (failureAt == 7) error.failedWriteWord = 0x0100;
        f.bus.script.push_back(error);
      } else {
        f.chip.words[pointer] ^= 0x0001;  // Low-bit drift requires full-word comparison.
      }
    };
    auto status = oneStep(staged, f);
    while (status.inProgress()) status = oneStep(staged, f);
    CHECK(status.is(failureAt < 8 ? Err::I2C_NACK_DATA : Err::CONFIG_MISMATCH));
    CHECK(f.bus.trace.size() == failureAt + 1);
    CHECK(f.device.desiredSettings().limits.lowCounts == (failureAt == 1 ? 0 : 0x0101));
    CHECK(f.device.getSettingsSnapshot().verificationReads == (failureAt == 1 ? 7 : 0));
    CHECK(f.device.isInitialized());
    f.beforeTransfer = {};
    if (failureAt != 1) {
      Sample sample;
      CHECK(f.device.configTrust() == ConfigTrust::DIRTY);
      CHECK(f.device.readLatest(sample).ok() && !sample.freshConversion &&
            sample.configTrust == ConfigTrust::DIRTY);
      f.bus.trace.clear();
      CHECK(f.device.recover().ok());
      CHECK(transcript(f) ==
            "R0F R04 R01 R03 R02 W01=0620 R01 W03=0101 R03 W02=0402 R02 R01 W01=0220 R01");
    }
  }
}

void finalVerificationAndAlertEvidence() {
  for (const auto failure : {Err::OK, Err::CONFIG_MISMATCH, Err::OPERATION_TIMEOUT,
                             Err::I2C_SHORT_TRANSFER}) {
    Fixture f;
    f.chip.words[1] = 0x4220;
    f.beforeTransfer = [&](bool read, std::uint8_t ptr) {
      if (f.bus.trace.size() != 13) return;
      CHECK(read && ptr == 1);
      f.chip.words[1] |= 0xA000;  // HIGH and ready at the final observation.
      if (failure == Err::CONFIG_MISMATCH) f.chip.words[1] ^= 0x0004;
      if (failure == Err::OPERATION_TIMEOUT) f.now = f.config.operationTimeoutMs;
      if (failure == Err::I2C_SHORT_TRANSFER) {
        f.bus.script.push_back({Status::Error(failure, "short final CONFIG"), 1, true});
      }
      if (failure == Err::OK) f.chip.words[1] |= 0x0800;  // Observed CC alias.
    };
    CHECK(f.device.begin(f.config).is(failure));
    CHECK(transcript(f) == normal && !LatestRead{f.device}.cache);
    Apply view{f.device, f.config.settings};
    const auto snapshot = f.device.getSettingsSnapshot();
    CHECK(snapshot.verifiedAtMs.has_value() == (failure == Err::OK));
    if (failure == Err::I2C_SHORT_TRANSFER) {
      CHECK(!snapshot.config);
    } else {
      CHECK(snapshot.config && snapshot.config->timestampValid);
      CHECK(snapshot.config->observedAtMs == f.device.healthSnapshot().lastSuccessMs);
      CHECK(snapshot.config->dataReady && snapshot.config->high);
    }
    CHECK(view.alerts().pendingLow);
    CHECK(view.alerts().source == ConfigReadSource::APPLY &&
          view.alerts().mode == ObservedMode::CONTINUOUS && view.alerts().mixedOrUnknown);
    CHECK(view.alerts().pendingHigh == (failure != Err::I2C_SHORT_TRANSFER));
    CHECK(view.alerts().possibleLoss == (failure == Err::I2C_SHORT_TRANSFER));
    CHECK(!view.readyPending());
    CHECK(view.readyHistoryUncertain() == (failure == Err::I2C_SHORT_TRANSFER));
    CHECK(f.device.healthSnapshot().totalSuccess == (failure == Err::I2C_SHORT_TRANSFER ? 13u : 14u));
    CHECK(f.device.healthSnapshot().totalFailures == (failure == Err::I2C_SHORT_TRANSFER ? 1u : 0u));
    CHECK((f.chip.words[1] & 0xE000) == 0);
    Sample sample;
    if (failure == Err::OK) {
      CHECK(view.baselineMs() == f.device.healthSnapshot().lastSuccessMs);
      CHECK(f.device.readLatest(sample).ok() && !sample.freshConversion);
    } else {
      CHECK(!f.device.isInitialized() && f.device.configTrust() == ConfigTrust::DIRTY);
      CHECK(!view.baselineMs() && f.device.readLatest(sample).is(Err::NOT_INITIALIZED));
      CHECK(f.bus.trace.size() == 14);
      if (failure == Err::OPERATION_TIMEOUT) {
        CHECK(f.device.healthSnapshot().lastSuccessMs == f.config.operationTimeoutMs);
        CHECK(f.bus.trace.back().rx == std::vector<std::uint8_t>({0xA2, 0x20}));
      }
    }
  }
}

void finalThermReadAfterVerifiedAlert() {
  for (const auto failure : {Err::OPERATION_TIMEOUT, Err::I2C_SHORT_TRANSFER}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    Sample original;
    CHECK(f.device.readLatest(original).ok() && original.configTrust == ConfigTrust::VERIFIED);
    f.chip.words[0x00] = 0x0100;  // A new word is available only to an explicit TEMP read.
    f.chip.words[0x01] |= 0x4000;  // LOW in the verified ALERT profile.
    f.bus.trace.clear();

    auto candidate = f.config.settings;
    candidate.alertMode = AlertMode::THERM;
    Apply change{f.device, candidate};
    const auto deadline = f.now + f.config.operationTimeoutMs;
    CHECK(change.start().inProgress());
    for (unsigned i = 0; i < 3; ++i) CHECK(nextTransfer(change, f).inProgress());
    const auto lowTime = f.device.healthSnapshot().lastSuccessMs;
    const auto& alerts = change.alerts();
    CHECK(alerts.pendingLow && alerts.observedAtMs == lowTime);
    f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
      if (!read || pointer != 0x01 || f.chip.words[0x01] != 0x0230) return;
      f.chip.words[0x01] |= 0xA000;  // THERM HIGH and DATA_READY at final readback.
      if (failure == Err::OPERATION_TIMEOUT) f.now = deadline;
      else f.bus.script.push_back({Status::Error(failure, "short THERM readback"), 1, true});
    };
    auto status = oneStep(change, f);
    while (status.inProgress()) status = oneStep(change, f);
    CHECK(status.is(failure));
    CHECK(transcript(f) ==
          "R0F R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 W01=0630 R01 W01=0230 R01");
    CHECK(f.chip.words[0x01] == 0x8230 && f.device.desiredSettings().alertMode == AlertMode::THERM);
    CHECK(f.device.isInitialized() && f.device.configTrust() == ConfigTrust::DIRTY && !change.baselineMs());
    CHECK(alerts.pendingLow && !alerts.pendingHigh && !alerts.mixedOrUnknown);
    CHECK(alerts.observedAtMs == lowTime && alerts.profileGeneration == original.profileGeneration);
    const bool shortRead = failure == Err::I2C_SHORT_TRANSFER;
    CHECK(alerts.possibleLoss == shortRead);
    if (shortRead) CHECK(!alerts.thermHigh.has_value());
    else CHECK(alerts.thermHigh == true && f.device.healthSnapshot().lastSuccessMs == deadline);

    const auto& cache = LatestRead{f.device}.cache;
    CHECK(cache && cache->raw == original.raw && cache->observedAtMs == original.observedAtMs);
    CHECK(cache->profileGeneration == original.profileGeneration && cache->configTrust == ConfigTrust::VERIFIED);
    Sample dirty;
    CHECK(f.device.readLatest(dirty).ok() && dirty.raw == 0x0100 && dirty.configTrust == ConfigTrust::DIRTY);
    CHECK(dirty.profileGeneration == original.profileGeneration && dirty.acquisitionKind == ReadKind::LATEST);
    CHECK(!dirty.freshConversion && dirty.offset.state == OffsetState::UNKNOWN && !dirty.offset.countsValid);

    f.beforeTransfer = {};
    f.chip.words[0x01] = 0x0230;  // The next observation sees THERM HIGH clear.
    f.bus.trace.clear();
    CHECK(f.device.recover().ok());
    CHECK(transcript(f) ==
          "R0F R04 R01 R03 R02 W01=0630 R01 W03=0000 R03 W02=0500 R02 R01 W01=0230 R01");
    CHECK(change.baselineMs() == f.device.healthSnapshot().lastSuccessMs && alerts.thermHigh == false);
    CHECK(alerts.pendingLow && !alerts.pendingHigh && !alerts.mixedOrUnknown && alerts.possibleLoss == shortRead);
    CHECK(alerts.observedAtMs == lowTime && alerts.profileGeneration == original.profileGeneration);
    CHECK(cache && cache->raw == dirty.raw && cache->observedAtMs == dirty.observedAtMs);
    CHECK(cache->profileGeneration == dirty.profileGeneration && cache->configTrust == ConfigTrust::DIRTY);
    Sample recovered;
    CHECK(f.device.readLatest(recovered).ok() && recovered.raw == dirty.raw);
    CHECK(recovered.configTrust == ConfigTrust::VERIFIED && recovered.profileGeneration == original.profileGeneration + 1);
    CHECK(!recovered.freshConversion && recovered.acquisitionKind == ReadKind::LATEST);
    CHECK(recovered.offset.state == OffsetState::UNKNOWN && !recovered.offset.countsValid);
  }
}

void replayAndStagedChangeOnDrift() {
  for (const bool configDrift : {false, true}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    const auto pointer = configDrift ? 1 : 3;
    f.chip.words[pointer] ^= 0x0004;
    f.bus.trace.clear();
    CHECK(f.device.recover().ok() && transcript(f) == normal);
    CHECK(f.chip.words[1] == 0x0220 && f.chip.words[3] == 0);

    f.chip.words[pointer] ^= 0x0004;
    auto candidate = f.config.settings;
    candidate.limits.lowCounts = -128;
    Apply change{f.device, candidate};
    f.bus.trace.clear();
    auto status = change.start();
    while (status.inProgress()) status = oneStep(change, f);
    CHECK(status.is(Err::CONFIG_MISMATCH) && f.device.configTrust() == ConfigTrust::DIRTY);
    CHECK(transcript(f) == (configDrift ? "R0F R04 R01" : "R0F R04 R01 R03"));
    CHECK(f.device.desiredSettings().limits.lowCounts == 0);
  }
}

void preflightDriftAtDeadline() {
  for (const auto pointer : {0x01, 0x03}) {
    Fixture f;
    CHECK(f.device.begin(f.config).ok());
    CHECK(f.device.verifyConfiguration().ok());
    const auto verified = f.device.getSettingsSnapshot();
    const auto health = f.device.healthSnapshot();
    const bool config = pointer == 0x01;
    const std::uint16_t changedWord = config ? 0xA224 : 0x0001;
    f.chip.words[pointer] = changedWord;
    f.bus.trace.clear();
    const auto deadline = f.now + f.config.operationTimeoutMs;
    f.beforeTransfer = [&](bool read, std::uint8_t reg) {
      if (read && reg == pointer) f.now = deadline;
    };

    CHECK(f.device.recover().is(Err::OPERATION_TIMEOUT));
    CHECK(transcript(f) == (config ? "R0F R04 R01" : "R0F R04 R01 R03"));
    const auto snapshot = f.device.getSettingsSnapshot();
    CHECK(snapshot.trust == ConfigTrust::DIRTY && snapshot.dirtyReason == DirtyReason::READBACK_MISMATCH);
    const auto field = config ? SettingsField::CONFIG : SettingsField::LOW_LIMIT;
    CHECK(snapshot.affectedFields == static_cast<std::uint8_t>(field));
    CHECK(snapshot.verificationReads == verified.verificationReads && snapshot.verificationReads == 7);
    CHECK(snapshot.verifiedAtMs == verified.verifiedAtMs && snapshot.verifiedGeneration == verified.verifiedGeneration);
    if (config) {
      CHECK(snapshot.config && snapshot.config->raw == changedWord && snapshot.config->observedAtMs == deadline);
      const Apply evidence{f.device};
      CHECK(evidence.alerts().pendingHigh && !evidence.readyPending());
    } else {
      CHECK(snapshot.low && snapshot.low->raw == changedWord && snapshot.low->observedAtMs == deadline);
    }
    const auto after = f.device.healthSnapshot();
    CHECK(after.totalFailures == health.totalFailures);
    CHECK(after.totalSuccess == health.totalSuccess + f.bus.trace.size());
    CHECK(after.lastSuccessMs == deadline && after.initialized);
  }
}

void retainedAlertsAcrossGenerations() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  Apply view{f.device};
  ConfigurationObservation observed;
  f.bus.trace.clear();
  f.chip.words[1] = 0xA220;
  CHECK(f.device.readConfiguration(observed).ok());
  CHECK(observed.dataReady && observed.high && !observed.low);
  const auto first = view.alerts();
  CHECK(first.pendingHigh && !first.pendingLow && !first.mixedOrUnknown);
  CHECK(first.source == ConfigReadSource::OBSERVATION && first.mode == ObservedMode::CONTINUOUS);
  CHECK(first.observedAtMs == observed.observedAtMs);
  CHECK(first.profileGeneration == f.device.getSettingsSnapshot().verifiedGeneration);
  CHECK(f.device.readConfiguration(observed).ok() && !observed.high && !observed.dataReady);
  CHECK(view.readyPending());
  CHECK(transcript(f) == "R01 R01" && view.alerts().observedAtMs == first.observedAtMs);

  CHECK(f.device.recover().ok());
  CHECK(f.device.getSettingsSnapshot().verifiedGeneration != first.profileGeneration);
  CHECK(!view.readyPending() && !view.readyHistoryUncertain());
  CHECK(view.alerts().pendingHigh && view.alerts().profileGeneration == first.profileGeneration);
  f.chip.words[1] |= 0x4000;
  CHECK(f.device.verifyConfiguration().ok());
  CHECK(view.alerts().pendingHigh && view.alerts().pendingLow && view.alerts().mixedOrUnknown);
  CHECK(view.alerts().source == first.source && view.alerts().mode == first.mode);
  CHECK(view.alerts().observedAtMs == first.observedAtMs && view.alerts().profileGeneration == first.profileGeneration);
  CHECK(!view.alerts().possibleLoss);

  Sample historical;
  f.chip.words[0] = 0xF380;
  CHECK(f.device.readLatest(historical).ok());
  const auto history = view.alerts();
  const auto calls = f.bus.trace.size();
  CHECK(f.device.invalidateDeviceState().ok());
  CHECK(f.bus.trace.size() == calls && !view.baselineMs() && !view.alerts().thermHigh);
  CHECK(view.alerts().pendingHigh && view.alerts().pendingLow && view.alerts().mixedOrUnknown);
  CHECK(view.alerts().observedAtMs == history.observedAtMs &&
        view.alerts().profileGeneration == history.profileGeneration);
  const auto saved = LatestRead{f.device}.cache;
  CHECK(saved && saved->raw == 0xF380 && saved->configTrust == ConfigTrust::VERIFIED);
  CHECK(saved->profileGeneration == historical.profileGeneration && saved->observedAtMs == historical.observedAtMs);
}

void busyAndAmbiguousReadEvidence() {
  Fixture f;
  CHECK(f.device.begin(f.config).ok());
  Apply view{f.device};
  ConfigurationObservation out;
  f.bus.trace.clear();
  f.chip.words[1] = 0xA220;  // Alert HIGH and eligible readiness.
  CHECK(f.device.readConfiguration(out).ok() && view.readyPending() && view.alerts().pendingHigh);
  f.chip.words[1] = 0x3A20;  // BUSY, READY and the continuous alias.
  CHECK(f.device.readConfiguration(out).ok() && out.eepromBusy && out.dataReady);
  CHECK(!view.readyPending() && f.device.configTrust() == ConfigTrust::VERIFIED);
  f.chip.words[1] = 0x3224;  // BUSY and READY with an externally changed stable selector.
  CHECK(f.device.readConfiguration(out).ok() && out.raw == 0x3224);
  CHECK(!view.readyPending() && !view.baselineMs() && f.device.configTrust() == ConfigTrust::DIRTY);
  f.chip.words[1] = 0x6220;
  f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "CONFIG short"), 1, true});
  CHECK(f.device.readConfiguration(out).is(Err::I2C_SHORT_TRANSFER));
  CHECK(out.raw == 0x3224 && f.chip.words[1] == 0x0220);
  CHECK(!view.readyPending() && view.readyHistoryUncertain() && view.alerts().possibleLoss);
  CHECK(view.alerts().pendingHigh && !view.alerts().pendingLow);
  CHECK(transcript(f) == "R01 R01 R01 R01");
}

void physicalTriggerEvidence() {
  for (const bool succeeded : {false, true}) {
    Fixture f;
    f.config.settings.mode = Mode::SHUTDOWN;
    CHECK(f.device.begin(f.config).ok());
    // Inject only the terminal physical outcome. Stage 21 owns guarded trigger
    // dispatch and its deadline. An owner can already have timed out at this tick.
    const TransferResult trigger{succeeded ? Status::Ok() : Status::Error(Err::I2C_TIMEOUT, "trigger"),
                                 true, f.now};
    DeviceEvidence evidence{f.device};
    evidence.observeWrite(0x01, 0x0E20, trigger);
    f.chip.words[1] = 0x0E20;
    if (!succeeded) {
      CHECK(f.device.configTrust() == ConfigTrust::DIRTY);
      CHECK(f.device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
      continue;
    }
    CHECK(f.device.verifyConfiguration().ok());
    evidence.observeWrite(0x01, 0x0620, {Status::Error(Err::OPERATION_TIMEOUT, "no SD callback"), false});
    CHECK(f.device.configTrust() == ConfigTrust::VERIFIED);
    CHECK(f.device.verifyConfiguration().ok());  // Refused SD retained the trigger evidence.
    f.chip.words[1] = 0x0620;
    CHECK(f.device.verifyConfiguration().ok());
    f.chip.words[1] = 0x0E20;
    CHECK(f.device.verifyConfiguration().ok());  // Observed SD did not end ownership.

    f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
      if (!read && pointer == 0x01) {
        f.bus.script.push_back({Status::Error(Err::I2C_NACK_DATA, "ambiguous SD")});
      }
    };
    CHECK(f.device.recover().is(Err::I2C_NACK_DATA));
    CHECK(f.device.configTrust() == ConfigTrust::DIRTY);
    f.beforeTransfer = {};
    CHECK(f.chip.words[1] == 0x0E20 && f.device.verifyConfiguration().is(Err::CONFIG_DIRTY));

    f.chip.words[1] ^= 0x0004;
    CHECK(f.device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
    CHECK(f.device.configTrust() == ConfigTrust::DIRTY);
    f.chip.words[1] = 0x0E20;
    const auto deadline = f.now + f.config.operationTimeoutMs;
    f.beforeTransfer = [&](bool read, std::uint8_t pointer) {
      if (!read && pointer == 0x01) f.now = deadline;
    };
    CHECK(f.device.recover().is(Err::OPERATION_TIMEOUT));
    CHECK(f.chip.words[1] == 0x0620);  // Physically successful SD at deadline ends ownership.
    f.beforeTransfer = {};
    f.chip.words[1] = 0x0E20;
    CHECK(f.device.verifyConfiguration().is(Err::CONFIG_MISMATCH));
  }
}

void workLimitsWrapAndLoadSuppression() {
  Fixture frozen;
  frozen.tickIncrement = 0;
  CHECK(frozen.device.bind(frozen.config).ok());
  Apply waiting{frozen.device, frozen.config.settings};
  CHECK(waiting.start().inProgress() && nextTransfer(waiting, frozen).inProgress());
  CHECK(oneStep(waiting, frozen).inProgress());
  ++frozen.now;  // A changed tick resets the same-tick allowance while still parked.
  CHECK(oneStep(waiting, frozen).inProgress());
  for (unsigned i = 0; i < 1000000u; ++i) CHECK(oneStep(waiting, frozen).inProgress());
  CHECK(oneStep(waiting, frozen).is(Err::WORK_LIMIT) && transcript(frozen) == "R0F");
  frozen.bus.trace.clear();
  CHECK(frozen.device.begin().is(Err::WORK_LIMIT) && transcript(frozen) == "R0F");
  Fixture wrapped;
  wrapped.now = 0xFFFFFFFCu;
  CHECK(wrapped.device.begin(wrapped.config).ok() && transcript(wrapped) == normal && wrapped.now < 100);

  Fixture busy;
  busy.config.operationTimeoutMs = 1000000;
  busy.chip.words[4] = 0x4000;
  CHECK(busy.device.begin(busy.config).is(Err::WORK_LIMIT));
  CHECK(busy.bus.trace.size() == 256);
  for (const auto& frame : busy.bus.trace) CHECK(frame.combinedRead);

  Fixture coarse{Model::TMP116};
  coarse.now = 40;
  coarse.tickIncrement = 0;
  coarse.config.operationTimeoutMs = 39;
  coarse.config.clockResolutionMs = 10;
  CHECK(coarse.device.bind(coarse.config).ok());
  Apply apply{coarse.device, coarse.config.settings};
  CHECK(apply.start().inProgress());
  coarse.now = 50;
  CHECK(nextTransfer(apply, coarse).is(Err::OPERATION_TIMEOUT));
  CHECK(coarse.bus.trace.back().timeoutMs == 20);
  coarse.now = 70;
  CHECK(oneStep(apply, coarse).is(Err::OPERATION_TIMEOUT) && transcript(coarse) == "R0F");

  // SBOS740A p.17 permits BUSY clear and ignored writes during loading.
  // An unusually long load can outlast the project's 3 ms policy interval.
  Fixture loading{Model::TMP116};
  loading.beforeTransfer = [&](bool read, std::uint8_t) {
    if (!read) {
      TransferOutcome ignored;
      ignored.ignoreSuccessfulWrite = true;
      loading.bus.script.push_back(ignored);
    }
  };
  CHECK(loading.device.begin(loading.config).is(Err::CONFIG_MISMATCH));
  CHECK(transcript(loading) == "R0F R04 R01 R03 R02 W01=0620 R01");
  CHECK(loading.chip.words[1] == 0 && !loading.device.isInitialized());
}

}  // namespace

int main() {
  allModelsBeginRecoverLatest();
  admissionPreservesBinding();
  startupFromCompatibleCompletion();
  recoveryStartupProof();
  recoveryAfterReportedReset();
  busyShutdownReadbackRequiresNewGuard();
  busyAndUnlockedGuards();
  unlockedGuardAtDeadline();
  busyRecheckAndFinalBusy();
  comparatorTransitions();
  signedThermLimitOrderAcrossZero();
  ignoredLimitCannotInvertThermPair();
  ignoredLimitPreventsThermEntry();
  stagedThermLimitsAndPartialRecovery();
  pinOnlyChangeUsesSameGuardedReplay();
  stagedIntentAndIndependentReadback();
  finalVerificationAndAlertEvidence();
  finalThermReadAfterVerifiedAlert();
  retainedAlertsAcrossGenerations();
  busyAndAmbiguousReadEvidence();
  physicalTriggerEvidence();
  replayAndStagedChangeOnDrift();
  preflightDriftAtDeadline();
  workLimitsWrapAndLoadSuppression();
  return EXIT_SUCCESS;
}
