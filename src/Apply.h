#pragma once

#include "RegisterTransport.h"

namespace TMP11x::detail {

// One exclusive, timed replay. The candidate is staged separately from the
// binding until the first write. start() admits without bus traffic. Each
// step() performs at most one transfer or inspects an outstanding wait.
struct Apply {
  explicit Apply(TMP11x& device) noexcept : Apply(device, device._config.settings) {
    _replayingTarget = true;
  }
  Apply(TMP11x& device, const SensorSettings& candidate) noexcept
      : _device(device), _candidate(candidate), _shutdown(candidate) {}
  Apply(const Apply&) = delete;
  Apply& operator=(const Apply&) = delete;

  static Status validateTiming(const Config& binding) noexcept;
  Status start();
  Status step();

  const auto& alerts() const noexcept { return _device._alerts; }
  std::optional<std::uint32_t> baselineMs() const noexcept { return _device._live.baselineMs; }

 private:
  enum class Phase {
    IDENTITY, GUARD, CURRENT_CONFIG, CURRENT_LOW, CURRENT_HIGH,
    WRITE_SHUTDOWN, VERIFY_SHUTDOWN, WRITE_FIRST_LIMIT, VERIFY_FIRST_LIMIT,
    WRITE_SECOND_LIMIT, VERIFY_SECOND_LIMIT, ENABLE_THERM, BASELINE, WRITE_MODE, FINAL_CONFIG
  };

  void advanceClock(std::uint32_t now) noexcept;
  Status waitFor(std::uint32_t minimumMs) noexcept;
  Status dispatch();
  Status readIdentityStep();
  Status readGuard();
  Status readConfig();
  Status readLimit(std::uint8_t pointer);
  Status writeConfig(const SensorSettings& settings, Phase next);
  Status writeLimit(std::uint8_t pointer, Phase next);
  Status finishTransfer(const TransferResult& result) noexcept;

  TMP11x& _device;
  SensorSettings _candidate;
  SensorSettings _shutdown;
  Phase _phase = Phase::IDENTITY;
  Phase _afterGuard = Phase::CURRENT_CONFIG;
  Status _status = Status::Error(Err::INVALID_PARAM, "Apply has not started");
  std::uint32_t _previousMs = 0;
  std::uint32_t _remainingMs = 0;
  std::uint32_t _waitMs = 0;
  std::uint32_t _sameTickChecks = 0;
  unsigned _dispatches = 0;
  bool _started = false;
  bool _replayingTarget = false;
  bool _lockedIdle = false;
  std::uint8_t _firstLimit = 0;
  std::uint8_t _secondLimit = 0;
};

}  // namespace TMP11x::detail
