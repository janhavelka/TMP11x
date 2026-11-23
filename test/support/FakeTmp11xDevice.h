#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

#include "TMP11x/Config.h"

namespace TMP11x::test {

// Tests seed complete register words, including ID and any programmed settings.
// The zero-initialized backing store carries no claim about factory contents.
struct FakeTmp11xDevice {
  explicit FakeTmp11xDevice(Model selected) : model(selected) {}

  Model model;
  std::array<std::uint16_t, 16> words{};
};

struct TransferOutcome {
  Status status{};                           // Raw adapter status, even an incoherent one.
  std::size_t failedReadBytes = 0;           // Prefix delivered to the caller's scratch.
  bool consumeOnFailure = false;             // Device may acknowledge flags before failure.
  std::optional<std::uint16_t> failedWriteWord{};  // Explicit post-error word, or preserve old.
  // Script HIGH/LOW (bits 15:14) on a successful change of comparator mode.
  // TI leaves the transition unspecified. Therm mode only exposes HIGH.
  std::uint16_t comparatorFlagsOnModeChange = 0;
  bool ignoreSuccessfulWrite = false;  // A reset-load write may ACK without changing storage.
};

struct TransferTrace {
  bool combinedRead;
  std::uint8_t address;
  std::size_t txLen;
  std::size_t rxLen;
  std::uint32_t timeoutMs;
  std::vector<std::uint8_t> tx;  // Copy of the supplied word frame.
  std::vector<std::uint8_t> rx;  // Only bytes actually delivered.
  Status result;
};

class FakeTmp11xBus {
 public:
  // The test owns each attached device for the duration of its use by this bus.
  bool attach(std::uint8_t address, FakeTmp11xDevice& device);

  static Status write(std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                      std::uint32_t timeoutMs, void* user);
  static Status writeRead(std::uint8_t address, const std::uint8_t* tx, std::size_t txLen,
                          std::uint8_t* rx, std::size_t rxLen, std::uint32_t timeoutMs,
                          void* user);

  std::deque<TransferOutcome> script;  // One outcome per invocation, then normal completion.
  std::vector<TransferTrace> trace;    // One owned entry per callback, including failures.

 private:
  Status transfer(bool combinedRead, std::uint8_t address, const std::uint8_t* tx,
                  std::size_t txLen, std::uint8_t* rx, std::size_t rxLen,
                  std::uint32_t timeoutMs);

  std::array<FakeTmp11xDevice*, 4> _devices{};
};

}  // namespace TMP11x::test
