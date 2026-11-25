#pragma once

#include "OperationBudget.h"
#include "TMP11x/Timing.h"

namespace TMP11x::detail {

// One owner-serialized acquisition. Admission is bus-silent. Each step
// parks, performs local work or invokes at most one terminal bus callback.
// Dropping a pending operation performs no I/O and leaves physical evidence intact.
struct Acquisition {
  Acquisition(TMP11x& device, ReadKind kind) noexcept : _device(device), _kind(kind) {}
  Acquisition(const Acquisition&) = delete;
  Acquisition& operator=(const Acquisition&) = delete;

  Status start(std::optional<std::uint32_t> timeoutMs);
  Status step(Sample& out);
  Status run(Sample& out, std::optional<std::uint32_t> timeoutMs = std::nullopt);

 private:
  enum class Phase { GUARD, PREFLIGHT, SHUTDOWN, BASELINE, TRIGGER, POLL, TEMP };

  Status admit(std::optional<std::uint32_t> timeoutMs);
  Status dispatch(Sample& out);
  Status readGuard();
  Status readConfig();
  Status writeConfig();
  Status preparePoll();
  Status renewGuard(Phase resume);
  Status finish(const TransferResult& result) noexcept;

  TMP11x& _device;
  ReadKind _kind;
  Phase _phase = Phase::GUARD;
  Phase _afterGuard = Phase::POLL;
  Status _status = Status::Error(Err::INVALID_PARAM, "Acquisition has not started");
  OperationBudget _budget;
  ConversionTiming _timing{};
  std::optional<std::uint32_t> _lastPollMs;
  std::uint32_t _generation = 0;
  std::uint16_t _shutdownWord = 0;
  bool _started = false;
  bool _timed = false;
  bool _lockedIdle = false;
};

}  // namespace TMP11x::detail
