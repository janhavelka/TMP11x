#pragma once

#include "RegisterTransport.h"

namespace TMP11x::detail {

// The timed owner has already admitted and rounded the budget in the binding's
// clock epoch. remainingMs is the budget left at previousMs.
struct ReadBudget {
  std::uint32_t previousMs;
  std::uint32_t remainingMs;
};

// One TEMP (0x00) observation. A complete raw word acknowledges DATA_READY,
// including at expiry. An attempted transfer without a word may have consumed
// readiness. With no attempt there was no read effect or physical outcome.
struct LatestReadResult {
  Status status;
  TransferResult transfer;
  std::optional<std::uint16_t> raw;
};

// Borrow the binding and cache for one owner-serialized read after admission.
struct LatestRead {
  explicit LatestRead(TMP11x& device) noexcept;

  ConfigTrust trust;
  std::uint32_t generation;
  std::optional<Sample>& cache;

  LatestReadResult read(Sample& out, std::optional<ReadBudget> budget = std::nullopt) const;

 private:
  RegisterTransport _transport;
};

}  // namespace TMP11x::detail
