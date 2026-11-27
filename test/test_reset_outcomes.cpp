#include "TMP11x/TMP11x.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

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

using Writes = std::vector<std::vector<std::uint8_t>>;
const Writes resetCommand{{1, 0x02, 0x22}};

struct Fixture {
  Fixture(Model model, bool manageOffset) : chip(model) {
    config.model = model;
    config.i2cAddress = 0x49;
    config.i2cWrite = FakeTmp11xBus::write;
    config.i2cWriteRead = FakeTmp11xBus::writeRead;
    config.i2cUser = &bus;
    config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
    config.timeUser = &clock;
    config.settings.limits = {-1280, 4736};
    if (manageOffset) config.settings.managedOffsetCounts = 64;
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0xB117;
    chip.words[5] = 0xA1B2;
    chip.words[6] = 0xC3D4;
    chip.words[7] = 0x0040;
    chip.words[8] = 0xE5F6;
    CHECK(bus.attach(config.i2cAddress, chip));
    CHECK(device.begin(config).ok());
    checkReplayWrites();
    bus.trace.clear();
  }

  Writes writes() const {
    Writes result;
    for (const auto& frame : bus.trace) {
      CHECK(frame.address == 0x49);
      if (!frame.combinedRead) result.push_back(frame.tx);
    }
    return result;
  }

  void checkReplayWrites() const {
    Writes expected{{1, 0x06, 0x20}, {3, 0xFB, 0}, {2, 0x12, 0x80}};
    if (config.settings.managedOffsetCounts) expected.push_back({7, 0, 0x40});
    expected.push_back({1, 0x02, 0x20});
    CHECK(writes() == expected);
  }

  void scriptReset(const FakeTmp11xDevice::RegisterWords& image) {
    TransferOutcome command;
    command.addressedResetWords = image;
    bus.script = {{}, command};  // Locked-idle preflight, then the addressed command.
  }

  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Config config;
  Device device;
  std::uint32_t clock = 0;
};

enum class AddressedOutcome { UNCHANGED, DIFFERENT_IMAGE, OFFSET_ONLY };

void addressedOutcomeAndRecovery(Model model, bool managed, AddressedOutcome outcome) {
  Fixture f{model, managed};
  const auto before = f.device.getSettingsSnapshot();
  auto expected = f.chip.words;
  if (outcome == AddressedOutcome::DIFFERENT_IMAGE) {
    expected[1] = 0x0464;
    expected[2] = 0x0A00;
    expected[3] = 0xFF80;
    expected[5] = 0x090A;
    expected[6] = 0x0000;
    expected[8] = 0xFADE;
  }
  if (outcome != AddressedOutcome::UNCHANGED) {
    expected[7] = 0xFF90;
    f.scriptReset(expected);
  }

  const auto status = f.device.reset();
  const bool mismatch = outcome == AddressedOutcome::DIFFERENT_IMAGE ||
                        (managed && outcome == AddressedOutcome::OFFSET_ONLY);
  CHECK(mismatch ? status.is(Err::CONFIG_MISMATCH) : status.ok());
  if (mismatch) CHECK(status.detail == (outcome == AddressedOutcome::OFFSET_ONLY ? 7 : 1));
  CHECK(!f.device.isInitialized() && f.writes() == resetCommand);
  const auto observed = f.device.getSettingsSnapshot();
  CHECK(observed.config && observed.config->raw == expected[1]);
  CHECK(observed.low && observed.low->raw == expected[3]);
  CHECK(observed.high && observed.high->raw == expected[2]);
  CHECK(observed.offset && observed.offset->raw == expected[7]);
  CHECK(observed.verifiedGeneration == before.verifiedGeneration && observed.verifiedAtMs == before.verifiedAtMs);
  CHECK(observed.desired.limits.lowCounts == -1280 && observed.desired.limits.highCounts == 4736);
  CHECK(observed.desired.managedOffsetCounts == before.desired.managedOffsetCounts);
  CHECK(observed.trust == (mismatch ? ConfigTrust::DIRTY : ConfigTrust::UNKNOWN));

  EepromSnapshot shadows;
  CHECK(f.device.readEepromSnapshot(shadows).ok() && shadows.wordCount == 3);
  CHECK(shadows.words[0].raw == expected[5] && shadows.words[1].raw == expected[6]);
  CHECK(shadows.words[2].raw == expected[8] && f.writes() == resetCommand);

  f.bus.trace.clear();
  CHECK(f.device.recover().ok() && f.device.isInitialized());
  f.checkReplayWrites();
  CHECK(f.chip.words[1] == 0x0220 && f.chip.words[3] == 0xFB00 && f.chip.words[2] == 0x1280);
  CHECK(f.chip.words[5] == expected[5] && f.chip.words[6] == expected[6] && f.chip.words[8] == expected[8]);
  CHECK(f.chip.words[7] == (managed ? 0x0040 : expected[7]));
  const auto recovered = f.device.getSettingsSnapshot();
  CHECK(recovered.offset && recovered.offset->raw == f.chip.words[7]);
  CHECK(recovered.trust == ConfigTrust::VERIFIED && recovered.verifiedGeneration > before.verifiedGeneration);
}

void independentCommandAndObservationOutcomes() {
  struct Stop {
    std::size_t callbacks;
    Err reason;
  };
  const Stop stops[] = {{2, Err::I2C_NACK_DATA}, {3, Err::I2C_NACK_ADDR}, {4, Err::EEPROM_UNSAFE}};
  for (const bool changed : {false, true}) {
    for (const auto& stop : stops) {
      Fixture f{Model::TMP117, false};
      auto image = f.chip.words;
      if (changed) image[3] = 0xED80;
      image[4] = 0xC000;  // A possible EUN+BUSY observation after safe preflight.
      f.scriptReset(image);
      if (stop.reason != Err::EEPROM_UNSAFE) {
        f.bus.script.resize(stop.callbacks);
        f.bus.script.back().status = Status::Error(stop.reason, "reset transfer", 9);
      }
      const auto status = f.device.reset();
      CHECK(status.is(stop.reason));
      if (stop.reason != Err::EEPROM_UNSAFE) CHECK(status.detail == 9);
      CHECK(f.bus.trace.size() == stop.callbacks && f.writes() == resetCommand);
      CHECK(!f.device.isInitialized() && !f.device.getSettingsSnapshot().low);
      CHECK(f.chip.words[3] == image[3] && f.chip.words[4] == 0xC000);
    }
  }
}

void driftBeforeReadFailure() {
  Fixture f{Model::TMP117, false};
  auto image = f.chip.words;
  image[3] = 0xED80;
  f.scriptReset(image);
  // Guard, command, ID, guard, CONFIG and the changed TLOW all complete.
  f.bus.script.resize(6);
  f.bus.script.push_back({Status::Error(Err::I2C_SHORT_TRANSFER, "short THIGH", 1), 1});
  const auto status = f.device.reset();
  CHECK(status.is(Err::I2C_SHORT_TRANSFER) && status.detail == 1);
  const auto observed = f.device.getSettingsSnapshot();
  CHECK(observed.trust == ConfigTrust::DIRTY && observed.dirtyReason == DirtyReason::READBACK_MISMATCH);
  CHECK(observed.affectedFields == static_cast<std::uint8_t>(SettingsField::LOW_LIMIT));
  CHECK(observed.verificationReads == 3 && observed.low && observed.low->raw == image[3]);
  CHECK(!observed.high && !observed.offset && !f.device.isInitialized());
  CHECK(f.bus.trace.size() == 7 && f.bus.trace.back().tx == std::vector<std::uint8_t>{2});
  CHECK(f.writes() == resetCommand);
}

// POR and an externally issued general call share this settled EEPROM load.
// Neither action requires the driver to emit a command.
void reportedProgrammedReload(Model model) {
  Fixture f{model, false};
  auto programmed = f.chip.words;
  programmed[0] = 0x1111;
  programmed[1] = 0xFEAF;  // Poison all status, MOD1, command and reserved bits.
  programmed[2] = 0x2580;
  programmed[3] = 0xF600;
  programmed[4] = 0xC000;
  programmed[5] = 0xBEEF;
  programmed[6] = 0x0000;
  programmed[7] = 0xFF90;
  programmed[8] = 0x2345;
  programmed[0x0F] = 0xFFFF;
  const auto id = f.chip.words[0x0F];
  f.chip.words[4] = 0x8000;  // Previous volatile EUN.
  f.chip.loadProgrammedStartup(programmed);
  CHECK(f.device.invalidateDeviceState().ok() && f.bus.trace.empty());
  CHECK(!f.device.isInitialized() && f.device.configTrust() == ConfigTrust::UNKNOWN);
  CHECK(f.chip.words[0] == 0x8000 && f.chip.words[1] == 0x06AC);
  CHECK(f.chip.words[4] == 0 && f.chip.words[0x0F] == id);
  CHECK(f.chip.words[2] == programmed[2] && f.chip.words[3] == programmed[3]);
  for (unsigned i = 5; i <= 8; ++i) CHECK(f.chip.words[i] == programmed[i]);

  EepromSnapshot snapshot;
  CHECK(f.device.readEepromSnapshot(snapshot).ok());
  const bool fourShadows = model == Model::TMP116 || model == Model::TMP116N;
  CHECK(snapshot.wordCount == (fourShadows ? 4 : 3));
  CHECK(snapshot.words[0].raw == 0xBEEF && snapshot.words[1].raw == 0x0000);
  if (fourShadows) CHECK(snapshot.words[2].raw == 0xFF90);
  CHECK(snapshot.words[snapshot.wordCount - 1].raw == 0x2345);
  CHECK(f.writes().empty());
  f.bus.trace.clear();
  CHECK(f.device.recover().ok() && f.device.isInitialized());
  f.checkReplayWrites();
  CHECK(f.chip.words[1] == 0x0220 && f.chip.words[2] == 0x1280 && f.chip.words[3] == 0xFB00);
  for (unsigned i = 5; i <= 8; ++i) CHECK(f.chip.words[i] == programmed[i]);
}

}  // namespace

int main() {
  for (const auto model : {Model::TMP117, Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    for (const bool managed : {false, true}) {
      for (const auto outcome : {AddressedOutcome::UNCHANGED, AddressedOutcome::DIFFERENT_IMAGE,
                                 AddressedOutcome::OFFSET_ONLY}) {
        addressedOutcomeAndRecovery(model, managed, outcome);
      }
    }
  }
  independentCommandAndObservationOutcomes();
  driftBeforeReadFailure();
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117, Model::TMP117M,
                           Model::TMP117N, Model::TMP119}) {
    reportedProgrammedReload(model);
  }
  return EXIT_SUCCESS;
}
