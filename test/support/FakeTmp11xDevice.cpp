#include "FakeTmp11xDevice.h"

#include <algorithm>

namespace TMP11x::test {
namespace {

// These literals are independent of the production register and capability tables.
bool hasOffset(Model model) {
  return model == Model::TMP117 || model == Model::TMP117M ||
         model == Model::TMP117N || model == Model::TMP119;
}

bool targetAddress(std::uint8_t address) {
  return address >= 0x48 && address <= 0x4B;
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
  // Reject calls outside the fixture's volatile stable-profile scope. This
  // diagnostic does not prescribe a silicon NACK for a policy violation.
  const bool allowed = (pointer == 0x01 && (value & 0xF803) == 0) ||
                       pointer == 0x02 || pointer == 0x03 ||
                       (pointer == 0x07 && hasOffset(device.model));
  if (!allowed) {
    return Status::Error(Err::I2C_ERROR, "Unexpected fixture write");
  }
  if (outcome.status.ok()) {
    if (pointer == 0x01) {
      device.words[pointer] = static_cast<std::uint16_t>((device.words[pointer] & 0xF000) | value);
    } else {
      device.words[pointer] = value;
    }
  } else if (outcome.failedWriteWord) {
    device.words[pointer] = *outcome.failedWriteWord;
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

bool FakeTmp11xBus::attach(std::uint8_t address, FakeTmp11xDevice& device) {
  if (!targetAddress(address) ||
      (device.model != Model::TMP116 && device.model != Model::TMP116N && !hasOffset(device.model))) {
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
