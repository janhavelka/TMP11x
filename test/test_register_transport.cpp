#include "RegisterTransport.h"
#include "support/FakeTmp11xDevice.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>

void testCommandTable();

namespace {

using namespace TMP11x;
using namespace TMP11x::detail;
using namespace TMP11x::test;

#define CHECK(condition)                                                            \
  do {                                                                              \
    if (!(condition)) {                                                             \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);          \
      std::exit(EXIT_FAILURE);                                                      \
    }                                                                               \
  } while (false)

Config configFor(FakeTmp11xBus& bus, Model model = Model::TMP117, std::uint8_t address = 0x48) {
  Config config;
  config.model = model;
  config.i2cAddress = address;
  config.i2cWrite = FakeTmp11xBus::write;
  config.i2cWriteRead = FakeTmp11xBus::writeRead;
  config.i2cUser = &bus;
  config.i2cTimeoutMs = 50;
  return config;
}

constexpr WriteAccess limit{WritePurpose::LIMIT, true, true};
constexpr WriteAccess stable{WritePurpose::STABLE_CONFIG, true, true};
constexpr WriteAccess offset{WritePurpose::MANAGED_OFFSET, true, true};
constexpr WriteAccess oneShot{WritePurpose::ONE_SHOT, true, true};
constexpr WriteAccess addressedReset{WritePurpose::ADDRESSED_RESET, true, true};

// A rejected transfer cannot account for a physical success or failure.
void checkRefused(TransferResult result, const FakeTmp11xBus& bus, std::size_t before) {
  CHECK(!result.status.ok());
  CHECK(!result.attempted);
  CHECK(bus.trace.size() == before);
}

void protectedWritesDenyEveryPayload() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    FakeTmp11xDevice chip{model};
    FakeTmp11xBus bus;
    CHECK(bus.attach(0x48, chip));
    const auto config = configFor(bus, model);
    for (std::uint8_t pointer = 0x04; pointer <= 0x08; ++pointer) {
      if (pointer == 0x07 && model != Model::TMP116 && model != Model::TMP116N) continue;
      for (const auto access : {stable, limit, offset, oneShot, addressedReset}) {
        // Includes relock, EUN-only, each reserved bit, and all bits set.
        for (std::uint32_t word = 0; word <= 0xFFFF; ++word) {
          const auto result = writeWord(config, pointer, static_cast<std::uint16_t>(word), access, 50);
          CHECK(result.status.is(Err::UNSUPPORTED));
          checkRefused(result, bus, 0);
        }
      }
    }
  }
}

void publicRoutesPreserveEeprom() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    const bool fourShadows = model == Model::TMP116 || model == Model::TMP116N;
    FakeTmp11xDevice chip{model};
    FakeTmp11xBus bus;
    auto config = configFor(bus, model, 0x4B);
    CHECK(bus.attach(config.i2cAddress, chip));
    std::uint32_t tick = 0;
    config.timeUser = &tick;
    config.nowMs = [](void* user) { return (*static_cast<std::uint32_t*>(user))++; };
    chip.words[0x0F] = fourShadows ? 0x1116 : 0xA117;
    chip.words[0x05] = 0x4719;
    chip.words[0x06] = 0xCAF3;
    chip.words[0x07] = 0x039B;  // EEPROM3 on TMP116/N, unmanaged calibration otherwise.
    chip.words[0x08] = 0xD5B6;
    const auto initialWords = chip.words;

    auto checkTrace = [&](bool initialization) {
      CHECK(!bus.trace.empty());
      for (const auto& frame : bus.trace) {
        CHECK(frame.address == config.i2cAddress);
        if (frame.combinedRead) {
          CHECK(frame.txLen == 1 && frame.rxLen == 2);
        } else {
          CHECK(initialization && frame.txLen == 3 && frame.rxLen == 0);
          const auto pointer = frame.tx[0];
          CHECK(pointer == 0x01 || pointer == 0x02 || pointer == 0x03);
          if (pointer == 0x01) CHECK((frame.tx[2] & 0x02) == 0);  // Soft_Reset
        }
      }
      for (std::uint8_t pointer = 0x04; pointer <= 0x08; ++pointer) {
        CHECK(chip.words[pointer] == initialWords[pointer]);
      }
      bus.trace.clear();
    };

    ::TMP11x::TMP11x device;
    CHECK(device.begin(config).ok());
    checkTrace(true);
    Sample latest;
    CHECK(device.readLatest(latest).ok());
    checkTrace(false);
    std::uint16_t word = 0;
    for (std::uint8_t index = 1; index <= (fourShadows ? 4 : 3); ++index) {
      CHECK(device.readEepromWord(index, word).ok());
      checkTrace(false);
    }
    EepromSnapshot snapshot;
    CHECK(device.readEepromSnapshot(snapshot).ok());
    checkTrace(false);
  }
}

void registerAccessByModel() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    const bool hasOffset = model != Model::TMP116 && model != Model::TMP116N;
    FakeTmp11xDevice device{model};
    FakeTmp11xBus bus;
    bus.attach(0x48, device);
    const Config config = configFor(bus, model);
    for (unsigned raw = 0; raw <= 0xFF; ++raw) {
      const auto ptr = static_cast<std::uint8_t>(raw);
      std::uint16_t out = 0x5AC3;
      auto before = bus.trace.size();
      const auto read = readWord(config, ptr, out, 50);
      if (ptr <= 0x08 || ptr == 0x0F) {
        CHECK(read.attempted && read.status.ok() && bus.trace.size() == before + 1);
        CHECK(bus.trace.back().tx == std::vector<std::uint8_t>{ptr});
      } else {
        checkRefused(read, bus, before);
        CHECK(read.status.is(Err::INVALID_PARAM) && out == 0x5AC3);
      }
      for (const auto access : {stable, limit, offset, oneShot}) {
        before = bus.trace.size();
        const std::uint16_t value = access.purpose == WritePurpose::ONE_SHOT ? 0x0C20 : 0x0420;
        const auto write = writeWord(config, ptr, value, access, 50);
        const bool allowed = (ptr == 0x01 && access.purpose == WritePurpose::STABLE_CONFIG)
            || (ptr == 0x01 && access.purpose == WritePurpose::ONE_SHOT)
            || ((ptr == 0x02 || ptr == 0x03) && access.purpose == WritePurpose::LIMIT)
            || (hasOffset && ptr == 0x07 && access.purpose == WritePurpose::MANAGED_OFFSET);
        if (allowed) {
          CHECK(write.attempted && write.status.ok() && bus.trace.size() == before + 1);
          CHECK((bus.trace.back().tx ==
                 std::vector<std::uint8_t>{ptr, static_cast<std::uint8_t>(value >> 8), 0x20}));
        } else {
          checkRefused(write, bus, before);
        }
      }
    }
    // Each disallowed bit is tested alone so a permissive reserved/status bit
    // cannot hide behind a correctly rejected command in a composite word.
    constexpr std::uint16_t invalidConfig[] = {0x8000, 0x4000, 0x2000, 0x1000, 0x0800,
                                               0x0C00, 0x0002, 0x0001};
    for (const auto value : invalidConfig) {
      const auto before = bus.trace.size();
      checkRefused(writeWord(config, 1, value, stable, 50), bus, before);
    }
    CHECK(writeWord(config, 1, 0x07FC, stable, 50).status.ok());
    const auto beforeShot = bus.trace.size();
    for (const auto rejected : {0x0020, 0x0420, 0x0820, 0x8C20, 0x4C20, 0x2C20,
                                0x1C20, 0x0C22, 0x0C21}) {
      checkRefused(writeWord(config, 1, rejected, oneShot, 50), bus, beforeShot);
    }
    checkRefused(writeWord(config, 1, 0x0FFC, {WritePurpose::ONE_SHOT, false, true}, 50), bus, beforeShot);
    checkRefused(writeWord(config, 1, 0x0FFC, {WritePurpose::ONE_SHOT, true, false}, 50), bus, beforeShot);
    CHECK(writeWord(config, 1, 0x0FFC, oneShot, 50).status.ok());
    for (const auto access : {stable, limit, offset}) {
      std::uint8_t ptr = 2;
      if (access.purpose == WritePurpose::STABLE_CONFIG) ptr = 1;
      if (access.purpose == WritePurpose::MANAGED_OFFSET) ptr = 7;
      for (auto unmet : {WriteAccess{access.purpose, true, false},
                         WriteAccess{access.purpose, false, true}}) {
        const auto before = bus.trace.size();
        checkRefused(writeWord(config, ptr, 0, unmet, 50), bus, before);
      }
    }
  }
}

void addressedResetAdmission() {
  constexpr struct {
    std::uint16_t word;
    std::uint8_t msb, lsb;
  } commands[] = {{0x0222, 0x02, 0x22}, {0x0622, 0x06, 0x22},
                  {0x0002, 0x00, 0x02}, {0x07FE, 0x07, 0xFE}};
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    FakeTmp11xDevice chip{model};
    FakeTmp11xBus bus;
    const auto config = configFor(bus, model, 0x4B);
    CHECK(bus.attach(0x4B, chip));
    for (const auto command : commands) {
      const auto before = bus.trace.size();
      const auto result = writeWord(config, 1, command.word, addressedReset, 3);
      if (model == Model::TMP116 || model == Model::TMP116N) {
        CHECK(result.status.is(Err::UNSUPPORTED));
        checkRefused(result, bus, before);
      } else {
        CHECK(result.status.ok() && result.attempted && bus.trace.size() == before + 1);
        const auto& frame = bus.trace.back();
        CHECK(!frame.combinedRead && frame.address == 0x4B && frame.timeoutMs == 3);
        CHECK(frame.txLen == 3 && frame.rxLen == 0);
        CHECK((frame.tx == std::vector<std::uint8_t>{0x01, command.msb, command.lsb}));
      }
    }
    const auto before = bus.trace.size();
    for (unsigned pointer = 0; pointer <= 0xFF; ++pointer) {
      if (pointer == 1) continue;
      checkRefused(writeWord(config, static_cast<std::uint8_t>(pointer), 0x0622,
                             addressedReset, 3), bus, before);
    }
    for (const auto otherPurpose : {stable, limit, offset, oneShot}) {
      checkRefused(writeWord(config, 1, 0x0622, otherPurpose, 3), bus, before);
    }
    if (model == Model::TMP116 || model == Model::TMP116N) continue;
    // Missing command, MOD=10/11, each status bit, and reserved bit 0.
    constexpr std::uint16_t invalid[] = {0x0620, 0x0A22, 0x0E22, 0x8622,
                                         0x4622, 0x2622, 0x1622, 0x0623};
    for (const auto word : invalid) {
      const auto result = writeWord(config, 1, word, addressedReset, 3);
      CHECK(result.status.is(Err::INVALID_PARAM));
      checkRefused(result, bus, before);
    }
    for (const auto missingProof : {WriteAccess{WritePurpose::ADDRESSED_RESET, false, true},
                                    WriteAccess{WritePurpose::ADDRESSED_RESET, true, false}}) {
      const auto result = writeWord(config, 1, 0x0622, missingProof, 3);
      CHECK(result.status.is(Err::INVALID_CONFIG));
      checkRefused(result, bus, before);
    }
  }
}

void bindingAndBudget() {
  FakeTmp11xDevice device{Model::TMP117};
  FakeTmp11xBus bus;
  for (unsigned address = 0x48; address <= 0x4B; ++address) {
    bus.attach(static_cast<std::uint8_t>(address), device);
  }
  const Config config = configFor(bus);
  std::uint16_t out = 0x1234;
  for (unsigned address = 0; address <= 0xFF; ++address) {
    Config trial = config;
    trial.i2cAddress = static_cast<std::uint8_t>(address);
    const auto before = bus.trace.size();
    const auto read = readWord(trial, 0x0F, out, 3);
    const auto write = writeWord(trial, 0x02, 0, limit, 51);
    if (address >= 0x48 && address <= 0x4B) {
      CHECK(read.attempted && read.status.ok() && write.attempted && write.status.ok());
      CHECK(bus.trace.size() == before + 2);
      CHECK(bus.trace[before].address == address && bus.trace[before].timeoutMs == 3);
      CHECK(bus.trace.back().address == address && bus.trace.back().timeoutMs == 50);
    } else {
      checkRefused(read, bus, before);
      checkRefused(write, bus, before);
      CHECK(read.status.is(Err::INVALID_CONFIG) && write.status.is(Err::INVALID_CONFIG));
    }
  }
  const auto budgetStart = bus.trace.size();
  CHECK(readWord(config, 0x0F, out, 100).status.ok());
  CHECK(writeWord(config, 2, 0, limit, 3).status.ok());
  CHECK(bus.trace.size() == budgetStart + 2);
  CHECK(bus.trace[budgetStart].timeoutMs == 50 && bus.trace.back().timeoutMs == 3);
  const auto before = bus.trace.size();
  auto rejectBinding = [&](const Config& trial) {
    out = 0x1234;
    const auto read = readWord(trial, 0, out, 50);
    const auto write = writeWord(trial, 2, 0, limit, 50);
    checkRefused(read, bus, before);
    checkRefused(write, bus, before);
    CHECK(read.status.is(Err::INVALID_CONFIG) && write.status.is(Err::INVALID_CONFIG));
    CHECK(out == 0x1234);
  };
  Config trial = config;
  trial.i2cWrite = nullptr;
  rejectBinding(trial);
  trial = config;
  trial.i2cWriteRead = nullptr;
  rejectBinding(trial);
  for (auto invalid : {Model::UNSELECTED, static_cast<Model>(255)}) {
    trial = config;
    trial.model = invalid;
    rejectBinding(trial);
  }
  for (const auto cap : {0u, 0x80000000u}) {
    trial = config;
    trial.i2cTimeoutMs = cap;
    rejectBinding(trial);
  }
  const auto expiredRead = readWord(config, 0x0F, out, 0);
  const auto expiredWrite = writeWord(config, 2, 0, limit, 0);
  checkRefused(expiredRead, bus, before);
  checkRefused(expiredWrite, bus, before);
  CHECK(expiredRead.status.is(Err::OPERATION_TIMEOUT) && expiredWrite.status.is(Err::OPERATION_TIMEOUT));
  CHECK(out == 0x1234);

  trial = config;
  trial.i2cUser = nullptr;
  trial.i2cWrite = [](std::uint8_t, const std::uint8_t*, std::size_t, std::uint32_t, void* user) {
    CHECK(user == nullptr);
    return Status::Ok();
  };
  trial.i2cWriteRead = [](std::uint8_t, const std::uint8_t*, std::size_t, std::uint8_t* rx,
                           std::size_t, std::uint32_t, void* user) {
    CHECK(user == nullptr);
    rx[0] = 0xCB;
    rx[1] = 0x31;
    return Status::Ok();
  };
  CHECK(readWord(trial, 0x0F, out, 50).status.ok() && out == 0xCB31);
  CHECK(writeWord(trial, 2, 0, limit, 50).status.ok());
}

void invalidFrames() {
  FakeTmp11xDevice device{Model::TMP117};
  FakeTmp11xBus bus;
  bus.attach(0x48, device);
  const Config config = configFor(bus);
  const std::uint8_t tx[] = {0x02, 0x12, 0xA5, 0xBB};
  std::uint8_t rx[] = {0x5A, 0xC3, 0xFF};
  for (std::size_t length = 0; length <= 4; ++length) {
    if (length != 1) checkRefused(readFrame(config, tx, length, rx, 2, 50), bus, 0);
    if (length != 2) checkRefused(readFrame(config, tx, 1, rx, length, 50), bus, 0);
    if (length != 3) checkRefused(writeFrame(config, tx, length, limit, 50), bus, 0);
  }
  checkRefused(readFrame(config, nullptr, 1, rx, 2, 50), bus, 0);
  checkRefused(readFrame(config, tx, 1, nullptr, 2, 50), bus, 0);
  checkRefused(writeFrame(config, nullptr, 3, limit, 50), bus, 0);
  const std::uint8_t badPointer[] = {0x12, 0x12, 0xA5};
  checkRefused(readFrame(config, badPointer, 1, rx, 2, 50), bus, 0);
  checkRefused(writeFrame(config, badPointer, 3, limit, 50), bus, 0);
  const std::uint8_t unlock[] = {0x04, 0x80, 0x00};
  checkRefused(writeFrame(config, unlock, 3, limit, 50), bus, 0);
  CHECK(rx[0] == 0x5A && rx[1] == 0xC3 && rx[2] == 0xFF);
}

void framesAndSharedAdapter() {
  FakeTmp11xDevice first{Model::TMP117};
  FakeTmp11xDevice other{Model::TMP116};
  first.words[0x0F] = 0xA117;
  other.words[0x0F] = 0x1116;
  other.words[0x05] = 0x1B2A;
  other.words[0x06] = 0x5AD3;
  FakeTmp11xBus bus;
  bus.attach(0x48, first);
  bus.attach(0x4B, other);
  const Config a = configFor(bus);
  const Config b = configFor(bus, Model::TMP116, 0x4B);
  std::uint16_t value = 0;
  auto read = [&](const Config& target, std::uint8_t ptr, std::uint16_t expected) {
    const auto before = bus.trace.size();
    const auto result = readWord(target, ptr, value, 3);
    CHECK(result.status.ok() && result.attempted && bus.trace.size() == before + 1);
    CHECK(value == expected);
    const auto& call = bus.trace.back();
    CHECK(call.combinedRead && call.result.ok() && call.address == target.i2cAddress);
    CHECK(call.timeoutMs == 3 && call.txLen == 1 && call.rxLen == 2);
    CHECK(call.tx == std::vector<std::uint8_t>{ptr});
    CHECK(call.rx.size() == 2 && call.rx[0] == (expected >> 8) && call.rx[1] == (expected & 0xFF));
  };
  read(a, 0x0F, 0xA117);
  read(b, 0x0F, 0x1116);
  const auto write = writeWord(a, 0x02, 0x12A5, limit, 100);
  CHECK(write.status.ok() && write.attempted && bus.trace.size() == 3);
  const auto& call = bus.trace.back();
  CHECK(!call.combinedRead && call.result.ok() && call.address == 0x48);
  CHECK(call.timeoutMs == 50 && call.txLen == 3 && call.rxLen == 0);
  CHECK((call.tx == std::vector<std::uint8_t>{0x02, 0x12, 0xA5}) && call.rx.empty());
  CHECK(first.words[0x02] == 0x12A5 && other.words[0x02] != 0x12A5);
  read(b, 0x05, 0x1B2A);
  read(b, 0x06, 0x5AD3);
  read(a, 0x0F, 0xA117);
}

void terminalStatusesAndReadPublication() {
  FakeTmp11xDevice device{Model::TMP117};
  device.words[0x0F] = 0xA117;
  FakeTmp11xBus bus;
  bus.attach(0x48, device);
  const Config config = configFor(bus);
  constexpr struct {
    Err adapter;
    Err expected;
  } failures[] = {
      {Err::I2C_ERROR, Err::I2C_ERROR}, {Err::I2C_NACK, Err::I2C_NACK},
      {Err::I2C_NACK_ADDR, Err::I2C_NACK_ADDR}, {Err::I2C_NACK_DATA, Err::I2C_NACK_DATA},
      {Err::I2C_SHORT_TRANSFER, Err::I2C_SHORT_TRANSFER}, {Err::I2C_TIMEOUT, Err::I2C_TIMEOUT},
      {Err::I2C_BUS, Err::I2C_BUS}, {Err::IN_PROGRESS, Err::I2C_ERROR},
      {Err::NOT_INITIALIZED, Err::I2C_ERROR}, {static_cast<Err>(0xFF), Err::I2C_ERROR},
  };
  for (const auto failure : failures) {
    TransferOutcome outcome{};
    outcome.status = Status::Error(failure.adapter, "adapter diagnostic", -703);
    auto before = bus.trace.size();
    bus.script.push_back(outcome);
    const auto write = writeWord(config, 2, 0xABCD, limit, 50);
    CHECK(write.attempted && bus.trace.size() == before + 1);
    CHECK(write.status.is(failure.expected) && write.status.detail == -703);
    CHECK(bus.trace.back().result.is(failure.adapter));
    // A short-transfer adapter reports an actual partial read and overwrites scratch.
    outcome.failedReadBytes = (failure.adapter == Err::I2C_SHORT_TRANSFER) ? 1 : 0;
    bus.script.push_back(outcome);
    before = bus.trace.size();
    std::uint16_t output = 0xBEEF;
    const auto read = readWord(config, 0x0F, output, 50);
    CHECK(read.attempted && bus.trace.size() == before + 1);
    CHECK(read.status.is(failure.expected) && read.status.detail == -703 && output == 0xBEEF);
    CHECK(bus.trace.back().result.is(failure.adapter) && bus.trace.back().result.detail == -703);
    if (outcome.failedReadBytes) CHECK(bus.trace.back().rx == std::vector<std::uint8_t>{0xA1});
  }
  std::uint8_t rx[] = {0x0D, 0x3B};
  const std::uint8_t pointer = 0x0F;
  TransferOutcome shortRead{};
  shortRead.status = Status::Error(Err::I2C_SHORT_TRANSFER, "one byte", 1);
  shortRead.failedReadBytes = 1;
  bus.script.push_back(shortRead);
  const auto result = readFrame(config, &pointer, 1, rx, 2, 50);
  CHECK(result.attempted && result.status.is(Err::I2C_SHORT_TRANSFER));
  CHECK(rx[0] == 0x0D && rx[1] == 0x3B);
}

void comparatorModeChange() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    FakeTmp11xDevice device{model};
    FakeTmp11xBus bus;
    CHECK(bus.attach(0x48, device));
    const Config config = configFor(bus, model);
    for (const bool thermHigh : {false, true}) {
      device.words[1] = 0x6000;  // Alert-mode LOW and DATA_READY.
      TransferOutcome transition{};
      transition.comparatorFlagsOnModeChange = thermHigh ? 0xC000 : 0x4000;
      bus.script.push_back(transition);
      CHECK(writeWord(config, 1, 0x0010, stable, 50).status.ok());
      std::uint16_t word = 0;
      CHECK(readWord(config, 1, word, 50).status.ok());
      CHECK(word == (thermHigh ? 0xA010 : 0x2010));  // LOW is zero on the first read.
      CHECK(readWord(config, 1, word, 50).status.ok());
      CHECK(word == (thermHigh ? 0x8010 : 0x0010));  // Only DATA_READY was consumed.

      bus.script.push_back(transition);
      CHECK(writeWord(config, 1, 0, stable, 50).status.ok());
      CHECK(readWord(config, 1, word, 50).status.ok());
      CHECK(word == transition.comparatorFlagsOnModeChange);
      CHECK(readWord(config, 1, word, 50).status.ok() && word == 0);
    }
  }
}

void ambiguousEffects() {
  FakeTmp11xDevice device{Model::TMP117};
  FakeTmp11xBus bus;
  bus.attach(0x48, device);
  const Config config = configFor(bus);
  TransferOutcome failure{};
  failure.status = Status::Error(Err::I2C_TIMEOUT, "completion unknown", -31);
  const std::optional<std::uint16_t> finalWords[] = {std::nullopt, 0x1234, 0x12CD};
  for (const auto afterFailure : finalWords) {
    device.words[2] = 0xABCD;
    failure.failedWriteWord = afterFailure;
    bus.script.push_back(failure);
    const auto before = bus.trace.size();
    const auto write = writeWord(config, 2, 0x1234, limit, 50);
    CHECK(write.attempted && write.status.is(Err::I2C_TIMEOUT) && write.status.detail == -31);
    CHECK(bus.trace.size() == before + 1);
    CHECK(device.words[2] == afterFailure.value_or(0xABCD));
  }
  failure.failedWriteWord.reset();
  failure.failedReadBytes = 1;
  for (const bool consumed : {false, true}) {
    failure.consumeOnFailure = consumed;
    constexpr std::uint8_t destructive[] = {0, 1};
    for (const auto ptr : destructive) {
      device.words[1] = 0xE000;  // Alert-mode HIGH, LOW and DATA_READY.
      bus.script.push_back(failure);
      std::uint16_t out = 0xCDBA;
      const auto before = bus.trace.size();
      const auto read = readWord(config, ptr, out, 50);
      CHECK(read.attempted && read.status.is(Err::I2C_TIMEOUT) && out == 0xCDBA);
      CHECK(bus.trace.size() == before + 1);
      const std::uint16_t consumedFlags = ptr == 0 ? 0xC000 : 0;
      CHECK(device.words[1] == (consumed ? consumedFlags : 0xE000));
    }
  }
}

void ambiguousOneShotCommands() {
  FakeTmp11xDevice chip{Model::TMP117};
  FakeTmp11xBus bus;
  CHECK(bus.attach(0x48, chip));
  const auto config = configFor(bus);
  chip.words[0] = 0x8000;
  TransferOutcome failure{};
  failure.status = Status::Error(Err::I2C_TIMEOUT, "Unknown command effect");
  for (bool applied : {false, true}) {
    CHECK(writeWord(config, 1, 0x0620, stable, 50).status.ok());
    failure.failedWriteWord = applied ? std::optional<std::uint16_t>{0x0E20} : std::nullopt;
    bus.script.push_back(failure);
    CHECK(writeWord(config, 1, 0x0E20, oneShot, 50).status.is(Err::I2C_TIMEOUT));
    CHECK(chip.addOneShotConversion(8) == applied);
    CHECK(chip.words[0] == 0x8000 && (chip.words[1] & 0x2000) == 0);

    // Fail SD during the first partial block of a new shot.
    CHECK(writeWord(config, 1, 0x0620, stable, 50).status.ok());
    CHECK(writeWord(config, 1, 0x0E20, oneShot, 50).status.ok());
    CHECK(chip.addOneShotConversion(8));
    failure.failedWriteWord = applied ? std::optional<std::uint16_t>{0x0620} : std::nullopt;
    bus.script.push_back(failure);
    CHECK(writeWord(config, 1, 0x0620, stable, 50).status.is(Err::I2C_TIMEOUT));
    CHECK(chip.addOneShotConversion(8) == !applied);
    CHECK(chip.words[0] == 0x8000 && (chip.words[1] & 0x2000) == 0);
  }
}

}  // namespace

int main() {
  testCommandTable();
  registerAccessByModel();
  protectedWritesDenyEveryPayload();
  addressedResetAdmission();
  publicRoutesPreserveEeprom();
  bindingAndBudget();
  invalidFrames();
  framesAndSharedAdapter();
  terminalStatusesAndReadPublication();
  comparatorModeChange();
  ambiguousEffects();
  ambiguousOneShotCommands();
  return EXIT_SUCCESS;
}
