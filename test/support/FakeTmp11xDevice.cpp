#include "FakeTmp11xDevice.h"

#include <algorithm>

namespace TMP11x::test {
namespace {

// These literals are independent of the production register and capability tables.
bool isTmp117Like(Model model) {
  return model == Model::TMP117 || model == Model::TMP117M ||
         model == Model::TMP117N || model == Model::TMP119;
}

bool targetAddress(std::uint8_t address) {
  return address >= 0x48 && address <= 0x4B;
}

std::int32_t signedCounts(std::uint16_t word) {
  return word < 0x8000u ? word : static_cast<std::int32_t>(word) - 65536;
}

void consumeFlags(FakeTmp11xDevice& device, std::uint8_t pointer) {
  auto& config = device.words[0x01];
  if (pointer == 0x00 || pointer == 0x01) {
    config &= 0xDFFF;  // TEMP and CONFIG both acknowledge DATA_READY.
  }
  if (pointer == 0x01 && (config & 0x0010) == 0) {
    config &= 0x3FFF;  // Alert-mode HIGH/LOW latches acknowledge on CONFIG.
  }
}

Status readWord(FakeTmp11xDevice& device, std::uint8_t pointer, std::uint8_t* rx,
                TransferTrace& frame, const TransferOutcome& outcome) {
  const auto word = device.words[pointer];
  const std::uint8_t bytes[] = {static_cast<std::uint8_t>(word >> 8),
                                static_cast<std::uint8_t>(word)};
  const auto delivered = outcome.status.ok() ? sizeof(bytes) :
                         std::min(outcome.failedReadBytes, sizeof(bytes));
  std::copy_n(bytes, delivered, rx);
  frame.rx.assign(bytes, bytes + delivered);
  if (outcome.status.ok() || outcome.consumeOnFailure) {
    consumeFlags(device, pointer);
  }
  return outcome.status;
}

Status writeWord(FakeTmp11xDevice& device, const std::uint8_t* tx,
                 const TransferOutcome& outcome) {
  const auto pointer = tx[0];
  const auto value = static_cast<std::uint16_t>((tx[1] << 8) | tx[2]);
  // Model the unchanged-register outcome for an addressed reset. Its effects
  // on companion CONFIG bits, lock state and EEPROM shadows are unspecified.
  if (pointer == 0x01 && isTmp117Like(device.model) && (value & 0xF803) == 0x0002) {
    return outcome.status;
  }
  // Ordinary CONFIG writes select canonical settings or the MOD=11 command.
  // Rejection here does not prescribe a silicon NACK for a policy violation.
  const bool stable = (value & 0xF803) == 0;
  const bool oneShot = (value & 0xFC03) == 0x0C00;
  const bool allowed = (pointer == 0x01 && (stable || oneShot)) ||
                       pointer == 0x02 || pointer == 0x03 ||
                       (pointer == 0x07 && isTmp117Like(device.model));
  if (!allowed) {
    return Status::Error(Err::I2C_ERROR, "Unexpected fixture write");
  }
  if (outcome.status.ok()) {
    if (outcome.ignoreSuccessfulWrite) return outcome.status;
    if (pointer == 0x01) {
      const auto previous = device.words[pointer];
      auto status = previous & 0xF000;
      if (((previous ^ value) & 0x0010) != 0) {
        const auto comparatorMask = (value & 0x0010) != 0 ? 0x8000 : 0xC000;
        status = (status & 0x3000) | (outcome.comparatorFlagsOnModeChange & comparatorMask);
      }
      device.applyConfigWord(static_cast<std::uint16_t>(status | value));
      if (oneShot && device.reportShutdownForOneShot) device.words[pointer] &= 0xF7FF;
    } else {
      device.words[pointer] = value;
    }
  } else if (outcome.failedWriteWord) {
    if (pointer == 0x01) {
      device.applyConfigWord(*outcome.failedWriteWord);
    } else {
      device.words[pointer] = *outcome.failedWriteWord;
    }
  }
  return outcome.status;
}

Status exchange(FakeTmp11xDevice* device, TransferTrace& frame, const std::uint8_t* tx,
                std::uint8_t* rx, const TransferOutcome& outcome) {
  if (device == nullptr) {
    return Status::Error(Err::I2C_NACK_ADDR, "No device at fixture address");
  }
  if (tx == nullptr || frame.txLen != (frame.combinedRead ? 1u : 3u) ||
      (frame.combinedRead && (rx == nullptr || frame.rxLen != 2))) {
    return Status::Error(Err::I2C_ERROR, "Unexpected fixture frame");
  }
  if (tx[0] > 0x08 && tx[0] != 0x0F) {
    return Status::Error(Err::I2C_ERROR, "Unexpected fixture pointer");
  }
  return frame.combinedRead ? readWord(*device, tx[0], rx, frame, outcome) :
                              writeWord(*device, tx, outcome);
}

}  // namespace

void FakeTmp11xDevice::applyConfigWord(std::uint16_t word) {
  const auto mode = word & 0x0C00u;
  // Synthetic OS results require an explicit SD boundary before the trigger.
  _oneShotActive = mode == 0x0C00u && _explicitShutdown;
  _explicitShutdown = mode == 0x0400u;
  if (mode == 0x0400u) {
    _sum = 0;
    _count = 0;
    _blockUncertain = false;
  } else if (_count != 0) {
    _blockUncertain = true;
  }
  words[0x01] = word;
}

bool FakeTmp11xDevice::addContinuousConversion(std::int16_t counts) {
  if ((words[0x01] & 0x0400u) != 0) return false;
  return addConversion(counts);
}

bool FakeTmp11xDevice::addOneShotConversion(std::int16_t counts) {
  if (!_oneShotActive) return false;
  return addConversion(counts);
}

bool FakeTmp11xDevice::addConversion(std::int16_t counts) {
  auto& config = words[0x01];
  const bool therm = (config & 0x0010u) != 0;
  const auto low = signedCounts(words[0x03]);
  const auto high = signedCounts(words[0x02]);
  if (_blockUncertain || (therm && low > high)) return false;
  constexpr unsigned lengths[] = {1, 8, 32, 64};
  const auto length = lengths[(config >> 5) & 3u];
  const auto sum = _sum + counts;
  if (_count + 1 < length) {
    _sum = sum;
    ++_count;
    return true;
  }
  const auto divisor = static_cast<std::int32_t>(length);
  if (sum % divisor != 0) return false;  // Fractional rounding is unspecified.
  const auto mean = sum / divisor;
  const auto offset = isTmp117Like(model) ? signedCounts(words[0x07]) : 0;
  const auto reported = std::clamp<std::int32_t>(mean + offset, -32768, 32767);
  words[0x00] = static_cast<std::uint16_t>(reported);
  config |= 0x2000u;
  if (therm) config &= 0xBFFFu;  // LOW always reads zero in THERM.
  // With nonzero calibration, the scenario supplies HIGH/LOW independently.
  // TI specifies the offset in TEMP but leaves comparator ordering open.
  if (offset == 0) {
    // Block-end comparison is inferred for TMP116/N and explicit for TMP117/M/N and TMP119.
    if (mean > high) config |= 0x8000u;
    if (therm) {
      if (mean < low) config &= 0x7FFFu;
    } else if (mean < low) {
      config |= 0x4000u;
    }
  }
  _sum = 0;
  _count = 0;
  _oneShotActive = false;
  return true;
}

bool FakeTmp11xBus::attach(std::uint8_t address, FakeTmp11xDevice& device) {
  if (!targetAddress(address) ||
      (device.model != Model::TMP116 && device.model != Model::TMP116N && !isTmp117Like(device.model))) {
    return false;
  }
  _devices[address - 0x48] = &device;
  return true;
}

Status FakeTmp11xBus::write(std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                            std::uint32_t timeoutMs, void* user) {
  return static_cast<FakeTmp11xBus*>(user)->transfer(false, address, tx, txLen,
                                                     nullptr, 0, timeoutMs);
}

Status FakeTmp11xBus::writeRead(std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                                std::uint8_t* rx, std::size_t rxLen, std::uint32_t timeoutMs,
                                void* user) {
  return static_cast<FakeTmp11xBus*>(user)->transfer(true, address, tx, txLen,
                                                     rx, rxLen, timeoutMs);
}

Status FakeTmp11xBus::transfer(bool combinedRead, std::uint8_t address, const std::uint8_t* tx,
                               std::size_t txLen, std::uint8_t* rx, std::size_t rxLen,
                               std::uint32_t timeoutMs) {
  trace.push_back({combinedRead, address, txLen, rxLen, timeoutMs, {}, {}, {}});
  auto& frame = trace.back();
  if (tx != nullptr) {
    frame.tx.assign(tx, tx + std::min(txLen, std::size_t{3}));
  }
  TransferOutcome outcome;
  if (!script.empty()) {
    outcome = script.front();
    script.pop_front();
  }
  auto* device = targetAddress(address) ? _devices[address - 0x48] : nullptr;
  frame.result = exchange(device, frame, tx, rx, outcome);
  return frame.result;
}

}  // namespace TMP11x::test
