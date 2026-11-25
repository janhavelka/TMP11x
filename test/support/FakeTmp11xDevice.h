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

  // Feed one synthetic ADC result under continuous ALERT-mode settings.
  // Test inputs must give exactly representable block means.
  bool addContinuousConversion(std::int16_t counts);

  // OS ignores CONV and stops after one complete AVG block. TI leaves MOD
  // readback unspecified; select 01 or 11 independently of physical completion.
  bool reportShutdownForOneShot = false;
  bool addOneShotConversion(std::int16_t counts);

  // The bus supplies the complete post-write word, including scripted failure effects.
  // SD discards a partial block. After another mid-block write, synthetic
  // conversions are unavailable until SD establishes a known accumulator.
  void applyConfigWord(std::uint16_t word);

 private:
  bool addConversion(std::int16_t counts);
  std::int32_t _sum = 0;
  unsigned _count = 0;
  bool _blockUncertain = false;
  bool _explicitShutdown = false;
  bool _oneShotActive = false;
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
