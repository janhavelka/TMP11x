#pragma once

#include "RegisterTransport.h"

namespace TMP11x::detail {

// Shared by exclusive, timed register operations. Each clock sample is a
// floor reading in the binding's epoch, including the sample at callback return.
struct OperationBudget {
  static constexpr auto pending = Status::Error(Err::IN_PROGRESS, "Operation pending");
  static constexpr auto timeout = Status::Error(Err::OPERATION_TIMEOUT, "Operation deadline reached");
  static constexpr auto workLimit = Status::Error(Err::WORK_LIMIT, "Operation work allowance exhausted");

  std::uint32_t previousMs = 0;
  std::uint32_t remainingMs = 0;

  void start(const Config& binding, std::uint32_t timeoutMs) {
    _resolutionMs = binding.clockResolutionMs;
    remainingMs = (timeoutMs / _resolutionMs) * _resolutionMs;
    previousMs = binding.nowMs(binding.timeUser);
  }

  Status finishTransfer(const TransferResult& transfer) noexcept {
    if (transfer.completedMs) advance(*transfer.completedMs);
    return remainingMs == 0 ? timeout : transfer.status;
  }

  Status waitForUs(std::uint32_t durationUs, std::uint32_t elapsedMs = 0) noexcept {
    const std::uint64_t tickUs = static_cast<std::uint64_t>(_resolutionMs) * 1000;
    // One extra tick covers the unknown phase of the originating callback.
    const auto delay = (1 + (durationUs + tickUs - 1) / tickUs) * _resolutionMs;
    const auto outstanding = elapsedMs < delay ? delay - elapsedMs : 0;
    if (outstanding >= remainingMs) return timeout;
    _waitMs = static_cast<std::uint32_t>(outstanding);
    return pending;
  }

  // OK grants one dispatch. Waiting checks consume no dispatch allowance.
  Status beforeDispatch(std::uint32_t now) noexcept {
    const bool sameTick = now == previousMs;
    advance(now);
    if (remainingMs == 0) return timeout;
    if (_waitMs != 0) {
      if (sameTick && ++_sameTickChecks > 1000000u) return workLimit;
      return pending;
    }
    _sameTickChecks = 0;
    ++_dispatches;
    return Status::Ok();
  }

  Status afterDispatch(Status status) const noexcept {
    return status.inProgress() && _dispatches == 256 ? workLimit : status;
  }

 private:
  void advance(std::uint32_t now) noexcept {
    const std::uint32_t elapsed = now - previousMs;
    previousMs = now;
    remainingMs = elapsed < remainingMs ? remainingMs - elapsed : 0;
    _waitMs = elapsed < _waitMs ? _waitMs - elapsed : 0;
    if (elapsed != 0) _sameTickChecks = 0;
  }

  std::uint32_t _resolutionMs = 1;
  std::uint32_t _waitMs = 0;
  std::uint32_t _sameTickChecks = 0;
  unsigned _dispatches = 0;
};

}  // namespace TMP11x::detail
