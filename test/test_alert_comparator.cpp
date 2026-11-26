#include "TMP11x/TMP11x.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdio>
#include <cstdlib>

namespace {

using namespace ::TMP11x;
using namespace ::TMP11x::test;
using Device = ::TMP11x::TMP11x;

#define CHECK(condition)                                                      \
  do {                                                                        \
    if (!(condition)) {                                                       \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);    \
      std::exit(EXIT_FAILURE);                                                \
    }                                                                         \
  } while (false)

struct Fixture {
  FakeTmp11xDevice chip;
  FakeTmp11xBus bus;
  Device device;
  Config config;
  std::uint32_t tick = 0;
  unsigned gpioCalls = 0;
  Status gpioResult{};

  explicit Fixture(Model model) : chip{model} {
    chip.words[0x0F] = model == Model::TMP116 || model == Model::TMP116N ? 0x1116 : 0x0117;
    config.model = model;
    config.i2cAddress = 0x49;
    config.i2cWrite = FakeTmp11xBus::write;
    config.i2cWriteRead = FakeTmp11xBus::writeRead;
    config.i2cUser = &bus;
    config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
    config.timeUser = &tick;
    config.readAlertPin = [](bool& high, void* user) {
      auto& f = *static_cast<Fixture*>(user);
      ++f.gpioCalls;
      // A single open-drain device and a working pullup, with no outside ARA.
      const auto word = f.chip.words[1];
      const bool asserted = (word & 0x0004u) ? (word & 0x2000u) != 0 : (word & 0xC000u) != 0;
      const bool activeHigh = (word & 0x0008u) != 0;
      high = asserted == activeHigh;  // High means released into the pullup.
      return f.gpioResult;
    };
    config.gpioUser = this;
    config.settings.averaging = Averaging::SAMPLES_1;
    config.settings.limits = {-1344, -832};  // -10.5 and -6.5 degrees Celsius.
    CHECK(bus.attach(config.i2cAddress, chip));
  }

  void checkPin(bool expectedHigh) {
    const auto calls = gpioCalls;
    const auto frames = bus.trace.size();
    const auto before = tick;
    bool high = !expectedHigh;
    CHECK(device.readAlertPinLevel(high).ok() && high == expectedHigh);
    CHECK(gpioCalls == calls + 1 && bus.trace.size() == frames && tick == before);
  }

  void checkConfig(bool high, bool low, bool ready) {
    ConfigurationObservation observed;
    CHECK(device.readConfiguration(observed).ok());
    CHECK(observed.high == high && observed.low == low && observed.dataReady == ready);
    CHECK(observed.readClearMask == (observed.alertMode == AlertMode::THERM ? 0x2000u : 0xE000u));
  }

  void checkAddressedTransfers() const {
    for (const auto& frame : bus.trace) CHECK(frame.address == config.i2cAddress);
  }
};

void alertWindowAckAndRearm(Model model) {
  Fixture f{model};
  CHECK(f.device.begin(f.config).ok());
  CHECK(f.chip.words[3] == 0xFAC0 && f.chip.words[2] == 0xFCC0);

  for (const std::int16_t equal : {-1344, -832}) {
    CHECK(f.chip.addContinuousConversion(equal));
    f.checkConfig(false, false, true);
  }
  CHECK(f.chip.addContinuousConversion(-831));  // 0xFCC1, one count above THIGH.
  Sample sample;
  CHECK(f.device.readLatest(sample).ok() && sample.raw == 0xFCC1);
  CHECK((f.chip.words[1] & 0xE000u) == 0x8000u);
  CHECK(f.chip.addContinuousConversion(-1280));  // 0xFB00, returned inside the window.
  f.checkConfig(true, false, true);
  f.checkConfig(false, false, false);

  for (int crossing = 0; crossing < 2; ++crossing) {
    CHECK(f.chip.addContinuousConversion(-1345));  // 0xFABF, one count below TLOW.
    f.checkPin(false);
    f.checkConfig(false, true, true);
    f.checkPin(true);
  }
  const auto retained = f.device.getAlertStatus();
  CHECK(retained.pendingHigh && retained.pendingLow && !retained.thermHigh.has_value());
  f.checkAddressedTransfers();
}

void thermHysteresisAndShutdown(Model model) {
  Fixture f{model};
  f.config.settings.alertMode = AlertMode::THERM;
  f.config.settings.averaging = Averaging::SAMPLES_8;
  CHECK(f.device.begin(f.config).ok());
  const auto block = [&](std::int16_t counts, bool wasHigh, bool becomesHigh) {
    // Every constituent can be over a threshold. Only the eighth changes the result.
    for (int i = 1; i < 8; ++i) {
      CHECK(f.chip.addContinuousConversion(counts));
      CHECK((f.chip.words[1] & 0xE000u) == (wasHigh ? 0x8000u : 0u));
    }
    CHECK(f.chip.addContinuousConversion(counts));
    f.checkConfig(becomesHigh, false, true);
    f.checkConfig(becomesHigh, false, false);
    const auto status = f.device.getAlertStatus();
    CHECK(status.thermHigh == becomesHigh && !status.pendingHigh && !status.pendingLow);
  };
  block(-832, false, false);
  block(-831, false, true);
  block(-1280, true, true);
  for (int i = 0; i < 4; ++i) CHECK(f.chip.addContinuousConversion(-1345));
  CHECK(f.device.setMode(Mode::SHUTDOWN).ok());
  CHECK(!f.chip.addContinuousConversion(-1345) && f.chip.words[0] == 0xFB00);
  f.checkConfig(true, false, false);
  f.checkPin(false);  // Inferred for fixed POL and threshold routing in shutdown.
  CHECK(f.device.setMode(Mode::CONTINUOUS).ok());
  CHECK(f.device.setAlert(AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY).ok());
  f.checkConfig(true, false, false);
  block(-1344, true, true);  // Equality at TLOW retains the set level.
  block(-1345, true, false);
  f.checkAddressedTransfers();
}

void eightPinRoutes(Model model) {
  // Literal pin_routes vectors. H is released/pulled high and L is sinking.
  constexpr struct {
    AlertMode mode;
    Polarity polarity;
    PinFunction route;
    bool pinA, pinB, pinC, highAfterC;
  } routes[] = {
    {AlertMode::ALERT, Polarity::ACTIVE_LOW,  PinFunction::THRESHOLD,  true,  false, true,  false},
    {AlertMode::ALERT, Polarity::ACTIVE_LOW,  PinFunction::DATA_READY, false, true,  true,  false},
    {AlertMode::ALERT, Polarity::ACTIVE_HIGH, PinFunction::THRESHOLD,  false, true,  false, false},
    {AlertMode::ALERT, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY, true,  false, false, false},
    {AlertMode::THERM, Polarity::ACTIVE_LOW,  PinFunction::THRESHOLD,  true,  false, false, true},
    {AlertMode::THERM, Polarity::ACTIVE_LOW,  PinFunction::DATA_READY, false, true,  true,  true},
    {AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::THRESHOLD,  false, true,  true,  true},
    {AlertMode::THERM, Polarity::ACTIVE_HIGH, PinFunction::DATA_READY, true,  false, false, true},
  };
  for (const auto& row : routes) {
    Fixture f{model};
    CHECK(f.device.begin(f.config).ok());
    CHECK(f.device.setAlert(row.mode, row.polarity, row.route).ok());
    CHECK(f.chip.addContinuousConversion(-1280));
    f.checkPin(row.pinA);
    f.checkConfig(false, false, true);

    CHECK(f.chip.addContinuousConversion(-831));
    Sample sample;
    CHECK(f.device.readLatest(sample).ok() && sample.raw == 0xFCC1);
    f.checkPin(row.pinB);
    f.checkConfig(true, false, false);
    f.checkPin(row.pinC);
    f.checkConfig(row.highAfterC, false, false);
    const auto status = f.device.getAlertStatus();
    CHECK(status.pendingHigh == (row.mode == AlertMode::ALERT) && !status.pendingLow);
    CHECK(status.thermHigh.has_value() == (row.mode == AlertMode::THERM));
    f.checkAddressedTransfers();
  }
}

void gpioObservationOnly() {
  Fixture f{Model::TMP117};
  bool high = false;
  CHECK(f.device.readAlertPinLevel(high).is(Err::NOT_BOUND) && !high);
  auto withoutPin = f.config;
  withoutPin.readAlertPin = nullptr;
  CHECK(f.device.bind(withoutPin).ok());
  CHECK(f.device.readAlertPinLevel(high).is(Err::UNSUPPORTED) && !high);
  CHECK(f.gpioCalls == 0 && f.bus.trace.empty() && f.tick == 0);

  CHECK(f.device.bind(f.config).ok());
  f.checkPin(true);
  f.gpioResult = Status::Error(Err::INVALID_PARAM, "GPIO read failed", -19);
  const auto status = f.device.readAlertPinLevel(high);
  CHECK(status.is(Err::INVALID_PARAM) && status.detail == -19 && !high);
  CHECK(f.gpioCalls == 2 && f.bus.trace.empty() && f.tick == 0);
  f.gpioResult = Status::Error(Err::IN_PROGRESS, "nonterminal GPIO callback");
  CHECK(f.device.readAlertPinLevel(high).is(Err::INVALID_CONFIG) && !high);
  CHECK(f.gpioCalls == 3 && f.bus.trace.empty() && f.tick == 0);
  const auto health = f.device.healthSnapshot();
  CHECK(health.totalSuccess == 0 && health.totalFailures == 0);

  f.gpioResult = {};
  CHECK(f.device.begin().ok());
  CHECK(f.chip.addContinuousConversion(-831));
  f.checkPin(false);
  CHECK(!f.device.getAlertStatus().pendingHigh);  // GPIO alone observed no register flags.
  Sample sample;
  CHECK(f.device.tryReadFresh(sample).ok() && sample.raw == 0xFCC1);
  CHECK(f.device.getAlertStatus().pendingHigh);
  f.checkPin(true);
  CHECK(f.device.getAlertStatus().pendingHigh);
  CHECK(f.device.tryReadFresh(sample).is(Err::MEASUREMENT_NOT_READY));

  CHECK(f.chip.addContinuousConversion(-1345));
  f.tick = 0;  // The wrapping observation clock may complete a CONFIG read at zero.
  f.checkConfig(false, true, true);
  const auto frames = f.bus.trace.size();
  AlertStatus events;
  CHECK(f.device.takeAlertEvents(events).ok() && events.pendingHigh && events.pendingLow);
  CHECK(events.lastConfigAtMs == 0 && f.tick == 1 && f.bus.trace.size() == frames);
  CHECK(f.device.tryReadFresh(sample).ok() && sample.counts == -1345);
  CHECK(f.bus.trace.size() == frames + 2 && f.bus.trace.back().tx[0] == 0);
  f.checkAddressedTransfers();
}

void externalAlertResponse(Model model) {
  Fixture f{model};
  bool lineHigh = false;
  f.config.gpioUser = &lineHigh;
  f.config.readAlertPin = [](bool& high, void* user) {
    high = *static_cast<bool*>(user);
    return Status::Ok();
  };
  CHECK(f.device.begin(f.config).ok());
  CHECK(f.chip.addContinuousConversion(-1345));  // LOW latched, threshold route, active low.
  f.bus.trace.clear();
  bool high = true;
  CHECK(f.device.readAlertPinLevel(high).ok() && !high);

  // The sensor wins arbitration on an outside controller's ARA. LOW stays latched.
  lineHigh = true;
  CHECK(f.device.invalidateDeviceState().ok());
  CHECK(f.device.readAlertPinLevel(high).ok() && high && f.bus.trace.empty());
  CHECK(f.device.getAlertStatus().possibleLoss && !f.device.getAlertStatus().pendingLow);
  CHECK(f.device.recover().ok());
  CHECK(f.bus.trace.at(2).combinedRead && f.bus.trace[2].tx[0] == 0x01);
  CHECK((f.bus.trace[2].rx[0] & 0xC0u) == 0x40u && (f.chip.words[1] & 0xC000u) == 0);
  AlertStatus events;
  CHECK(f.device.takeAlertEvents(events).ok() && events.pendingLow && events.possibleLoss);
  CHECK(!events.pendingHigh && events.firstEvent && events.firstEvent->source == ConfigReadSource::APPLY);
  CHECK(f.device.takeAlertEvents(events).ok() && !events.pendingLow && !events.possibleLoss);
  f.checkAddressedTransfers();
}

void identityMismatchBreaksAlertHistory() {
  for (const bool managed : {false, true}) {
    Fixture f{Model::TMP117};
    CHECK(f.device.begin(f.config).ok());
    CHECK(f.chip.addContinuousConversion(-831));
    f.checkConfig(true, false, true);
    const auto captured = f.device.getAlertStatus();
    CHECK(captured.firstEvent && !captured.possibleLoss);
    f.chip.words[0x0F] = 0x1116;  // Incompatible with the selected TMP117.
    f.bus.trace.clear();
    const auto status = managed ? f.device.recover() : f.device.probe();
    CHECK(status.is(Err::IDENTITY_MISMATCH) && !f.device.isInitialized());
    CHECK(f.bus.trace.size() == 1 && f.bus.trace[0].tx[0] == 0x0F);
    AlertStatus events;
    CHECK(f.device.takeAlertEvents(events).ok() && events.possibleLoss && events.pendingHigh);
    CHECK(events.firstEvent && events.firstEvent->profileGeneration == captured.firstEvent->profileGeneration);
    CHECK(events.firstEvent->observedAtMs == captured.firstEvent->observedAtMs);
    CHECK(f.device.probe().is(Err::IDENTITY_MISMATCH));
    CHECK(f.device.takeAlertEvents(events).ok() && !events.possibleLoss && !events.pendingHigh);
  }
}

}  // namespace

int main() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    alertWindowAckAndRearm(model);
    thermHysteresisAndShutdown(model);
    eightPinRoutes(model);
    externalAlertResponse(model);
  }
  gpioObservationOnly();
  identityMismatchBreaksAlertHistory();
  return EXIT_SUCCESS;
}
