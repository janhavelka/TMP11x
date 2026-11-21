#include "TMP11x/TMP11x.h"

#include "ModelDescriptor.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>

namespace {

bool forbidAllocation = false;

void* allocate(std::size_t size) {
  if (forbidAllocation) {
    std::fputs("A value operation attempted C++ heap allocation\n", stderr);
    std::abort();
  }
  if (void* memory = std::malloc(size == 0 ? 1 : size)) {
    return memory;
  }
  throw std::bad_alloc{};
}

}  // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace {

using namespace TMP11x;

#define CHECK(condition)                                                       \
  do {                                                                        \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);        \
      std::exit(EXIT_FAILURE);                                                 \
    }                                                                         \
  } while (false)

static_assert(std::is_same_v<decltype(Sample::raw), std::uint16_t>);
static_assert(std::is_same_v<decltype(Sample::counts), std::int16_t>);
static_assert(std::is_same_v<decltype(Sample::celsius), double>);
static_assert(std::is_same_v<decltype(Sample::observedAtMs), std::uint32_t>);
static_assert(std::is_same_v<decltype(Sample::profileGeneration), std::uint32_t>);
static_assert(std::is_same_v<decltype(OffsetProvenance::counts), std::int16_t>);
static_assert(std::is_same_v<decltype(IdentityInfo::raw), std::uint16_t>);
static_assert(std::is_trivially_copyable_v<Sample>);
static_assert(std::is_trivially_copyable_v<IdentityInfo>);

void statusNamesAndPredicates() {
  constexpr struct {
    Err code;
    const char* name;
  } expected[] = {
      {Err::OK, "OK"},
      {Err::IN_PROGRESS, "IN_PROGRESS"},
      {Err::INVALID_PARAM, "INVALID_PARAM"},
      {Err::INVALID_CONFIG, "INVALID_CONFIG"},
      {Err::NOT_BOUND, "NOT_BOUND"},
      {Err::NOT_INITIALIZED, "NOT_INITIALIZED"},
      {Err::UNSUPPORTED, "UNSUPPORTED"},
      {Err::CLOCK_REQUIRED, "CLOCK_REQUIRED"},
      {Err::BUSY, "BUSY"},
      {Err::IDENTITY_MISMATCH, "IDENTITY_MISMATCH"},
      {Err::EEPROM_UNSAFE, "EEPROM_UNSAFE"},
      {Err::CONFIG_DIRTY, "CONFIG_DIRTY"},
      {Err::CONFIG_MISMATCH, "CONFIG_MISMATCH"},
      {Err::MEASUREMENT_NOT_READY, "MEASUREMENT_NOT_READY"},
      {Err::OPERATION_TIMEOUT, "OPERATION_TIMEOUT"},
      {Err::WORK_LIMIT, "WORK_LIMIT"},
      {Err::CANCELLED, "CANCELLED"},
      {Err::RESULT_NOT_AVAILABLE, "RESULT_NOT_AVAILABLE"},
      {Err::TOKEN_MISMATCH, "TOKEN_MISMATCH"},
      {Err::I2C_ERROR, "I2C_ERROR"},
      {Err::I2C_NACK, "I2C_NACK"},
      {Err::I2C_NACK_ADDR, "I2C_NACK_ADDR"},
      {Err::I2C_NACK_DATA, "I2C_NACK_DATA"},
      {Err::I2C_SHORT_TRANSFER, "I2C_SHORT_TRANSFER"},
      {Err::I2C_TIMEOUT, "I2C_TIMEOUT"},
      {Err::I2C_BUS, "I2C_BUS"},
  };

  static constexpr char adapterMessage[] = "controller refused transfer";
  for (const auto& entry : expected) {
    const Status value{entry.code, -37, adapterMessage};
    CHECK(std::strcmp(errorName(entry.code), entry.name) == 0);
    CHECK(value.is(entry.code));
    CHECK(value.ok() == (entry.code == Err::OK));
    CHECK(value.inProgress() == (entry.code == Err::IN_PROGRESS));
    CHECK(value.detail == -37 && value.msg == adapterMessage);
  }

  const auto unknown = static_cast<Err>(0xFF);
  CHECK(std::strcmp(errorName(unknown), "UNKNOWN") == 0);
  const auto error = Status::Error(Err::I2C_NACK, adapterMessage, -38);
  CHECK(error.is(Err::I2C_NACK) && !error.is(Err::I2C_NACK_ADDR));
  CHECK(error.detail == -38 && error.msg == adapterMessage);
  CHECK(Status::Error(Err::I2C_BUS, adapterMessage).detail == 0);
  const Status successes[] = {Status{}, Status::Ok()};
  for (const Status success : successes) {
    CHECK(success.ok() && success.detail == 0);
    CHECK(std::strcmp(success.msg, "OK") == 0);
  }
}

void modelCapabilitiesAndIdentityPolicy() {
  constexpr struct {
    Model model;
    bool offsetAndReset;
    IdentityGroup group;
    std::uint16_t mask;
    std::uint16_t deviceId;
    std::uint16_t revisionMask;
    std::uint8_t revisionShift;
    std::uint8_t shadowCount;
    std::uint8_t shadows[4];
  } expected[] = {
      {Model::TMP116,  false, IdentityGroup::TMP116,      0xFFFF, 0x1116, 0, 0, 4, {5, 6, 7, 8}},
      {Model::TMP116N, false, IdentityGroup::TMP116,      0xFFFF, 0x1116, 0, 0, 4, {5, 6, 7, 8}},
      {Model::TMP117,  true,  IdentityGroup::TMP117_LIKE, 0x0FFF, 0x0117, 0xF000, 12, 3, {5, 6, 8}},
      {Model::TMP117M, true,  IdentityGroup::TMP117_LIKE, 0x0FFF, 0x0117, 0xF000, 12, 3, {5, 6, 8}},
      {Model::TMP117N, true,  IdentityGroup::TMP117_LIKE, 0x0FFF, 0x0117, 0xF000, 12, 3, {5, 6, 8}},
      {Model::TMP119,  true,  IdentityGroup::TMP117_LIKE, 0x0FFF, 0x0117, 0xF000, 12, 3, {5, 6, 8}},
  };

  for (const auto& row : expected) {
    CHECK(isValidModel(row.model));
    Capabilities caps{};
    CHECK(getCapabilities(row.model, caps).ok());
    CHECK(caps.hasOffset == row.offsetAndReset);
    CHECK(caps.hasAddressedReset == row.offsetAndReset);
    CHECK(caps.eepromWordCount == row.shadowCount);

    const auto* descriptor = detail::findModelDescriptor(row.model);
    CHECK(descriptor != nullptr);
    CHECK(descriptor->identityGroup == row.group);
    CHECK(descriptor->idMask == row.mask && descriptor->idValue == row.deviceId);
    CHECK(descriptor->revisionMask == row.revisionMask);
    CHECK(descriptor->revisionShift == row.revisionShift);
    for (std::size_t index = 0; index < row.shadowCount; ++index) {
      CHECK(descriptor->eepromPointers[index] == row.shadows[index]);
    }
  }

  unsigned recognized = 0;
  for (unsigned raw = 0; raw < 256; ++raw) {
    const auto model = static_cast<Model>(raw);
    bool selected = false;
    for (const auto& row : expected) {
      selected = selected || model == row.model;
    }
    CHECK(isValidModel(model) == selected);
    if (selected) {
      ++recognized;
      continue;
    }
    CHECK(detail::findModelDescriptor(model) == nullptr);
    Capabilities caps{};
    caps.hasOffset = true;
    caps.hasAddressedReset = true;
    caps.eepromWordCount = 0xFF;
    CHECK(getCapabilities(model, caps).is(Err::INVALID_PARAM));
    CHECK(caps.hasOffset && caps.hasAddressedReset && caps.eepromWordCount == 0xFF);
  }
  CHECK(recognized == 6);
  CHECK(!isValidModel(Model::UNSELECTED));
}

void addressesAndSelectors() {
  constexpr Cycle cycles[] = {Cycle::CODE_0, Cycle::CODE_1, Cycle::CODE_2,
                             Cycle::CODE_3, Cycle::CODE_4, Cycle::CODE_5,
                             Cycle::CODE_6, Cycle::CODE_7};
  constexpr Averaging averages[] = {Averaging::SAMPLES_1, Averaging::SAMPLES_8,
                                    Averaging::SAMPLES_32, Averaging::SAMPLES_64};
  for (unsigned raw = 0; raw < 256; ++raw) {
    const bool address = raw == 0x48 || raw == 0x49 || raw == 0x4A || raw == 0x4B;
    CHECK(isValidAddress(static_cast<std::uint8_t>(raw)) == address);
    CHECK(isValidCycle(static_cast<Cycle>(raw)) == (raw < 8));
    CHECK(isValidAveraging(static_cast<Averaging>(raw)) == (raw < 4));
    CHECK(isValidMode(static_cast<Mode>(raw)) == (raw < 2));
    CHECK(isValidAlertMode(static_cast<AlertMode>(raw)) == (raw < 2));
    CHECK(isValidPolarity(static_cast<Polarity>(raw)) == (raw < 2));
    CHECK(isValidPinFunction(static_cast<PinFunction>(raw)) == (raw < 2));
  }
  for (std::size_t index = 0; index < 8; ++index) {
    CHECK(cycles[index] == static_cast<Cycle>(index));
  }
  for (std::size_t index = 0; index < 4; ++index) {
    CHECK(averages[index] == static_cast<Averaging>(index));
  }
}

void defaultAndCopiedEvidence() {
  const Config cfg{};
  CHECK(cfg.model == Model::UNSELECTED && cfg.i2cAddress == 0);
  CHECK(cfg.i2cWrite == nullptr && cfg.i2cWriteRead == nullptr && cfg.i2cUser == nullptr);
  CHECK(cfg.nowMs == nullptr && cfg.timeUser == nullptr && cfg.clockResolutionMs == 1);
  CHECK(cfg.readAlertPin == nullptr && cfg.gpioUser == nullptr);
  CHECK(cfg.i2cTimeoutMs == 50 && cfg.operationTimeoutMs == 1000 && cfg.offlineThreshold == 5);
  CHECK(cfg.settings.mode == Mode::CONTINUOUS && cfg.settings.cycle == Cycle::CODE_4);
  CHECK(cfg.settings.averaging == Averaging::SAMPLES_8);
  CHECK(cfg.settings.alertMode == AlertMode::ALERT && cfg.settings.polarity == Polarity::ACTIVE_LOW);
  CHECK(cfg.settings.pinFunction == PinFunction::THRESHOLD);
  CHECK(cfg.settings.limits.lowCounts == -32768 && cfg.settings.limits.highCounts == 24576);
  auto copy = cfg;
  copy.settings.limits.highCounts = 20000;
  CHECK(copy.settings.limits.highCounts == 20000 && cfg.settings.limits.highCounts == 24576);

  const IdentityInfo identity{};
  CHECK(identity.model == Model::UNSELECTED && identity.i2cAddress == 0);
  CHECK(identity.raw == 0 && !identity.rawValid && identity.group == IdentityGroup::UNKNOWN);
  CHECK(identity.revision == 0 && !identity.revisionValid && !identity.compatible);

  Sample latest{};
  CHECK(latest.raw == 0 && latest.counts == 0 && latest.celsius == 0.0);
  CHECK(latest.observedAtMs == 0 && !latest.timestampValid && !latest.freshConversion);
  CHECK(latest.acquisitionKind == ReadKind::LATEST && latest.profileGeneration == 0);
  CHECK(latest.configTrust == ConfigTrust::UNKNOWN);
  CHECK(latest.offset.state == OffsetState::UNKNOWN && latest.offset.counts == 0);
  CHECK(!latest.offset.countsValid);
  latest.raw = 0x8000;
  latest.counts = -32768;
  latest.celsius = -256.0;
  CHECK(!latest.timestampValid && !latest.freshConversion && latest.configTrust == ConfigTrust::UNKNOWN);

  auto fresh = latest;
  fresh.timestampValid = true;
  fresh.freshConversion = true;
  fresh.acquisitionKind = ReadKind::ONE_SHOT;
  fresh.profileGeneration = 7;
  fresh.configTrust = ConfigTrust::VERIFIED;
  CHECK(fresh.raw == latest.raw && fresh.counts == latest.counts && fresh.celsius == latest.celsius);
  CHECK(fresh.observedAtMs == 0 && fresh.timestampValid && !latest.timestampValid);
  CHECK(fresh.freshConversion && !latest.freshConversion && latest.profileGeneration == 0);
  CHECK(!fresh.offset.countsValid && fresh.offset.state == OffsetState::UNKNOWN);
}

}  // namespace

int main() {
  forbidAllocation = true;
  statusNamesAndPredicates();
  modelCapabilitiesAndIdentityPolicy();
  addressesAndSelectors();
  defaultAndCopiedEvidence();
  forbidAllocation = false;
  return EXIT_SUCCESS;
}
