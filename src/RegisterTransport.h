#pragma once

#include <limits>
#include <optional>

#include "TMP11x/TMP11x.h"

namespace TMP11x::detail {

// Positive intervals below the half-range of the wrapping millisecond clock.
constexpr bool isValidInterval(std::uint32_t ms) noexcept {
  return ms > 0 && ms <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
}

Status validateTransportBinding(const Config& binding) noexcept;

// Borrow the target and optional history for one owner-serialized operation.
// A bare binding selects the health-neutral transport used by diagnostics.
struct RegisterTransport {
  RegisterTransport(const Config& config, HealthSnapshot* history = nullptr) noexcept
      : binding(config), health(history) {}

  static RegisterTransport tracked(TMP11x& device) noexcept {
    return {device._config, &device._health};
  }

  const Config& binding;
  HealthSnapshot* health;
};

constexpr bool isOnline(bool initialized, DriverState state) noexcept {
  return initialized && (state == DriverState::READY || state == DriverState::DEGRADED);
}

// A local refusal costs zero transfers. A callback, even an incoherent or failed
// one, costs one and may already have affected the device.
struct TransferResult {
  Status status;
  bool attempted;
  std::optional<std::uint32_t> completedMs{};  // The same tick recorded by tracked health.
};

enum class WritePurpose { STABLE_CONFIG, LIMIT, MANAGED_OFFSET, ONE_SHOT };

// Supplied by the typed operation owner. Both proofs belong to the current
// target and uninterrupted operation. The serializer never acquires a guard.
struct WriteAccess {
  WritePurpose purpose;
  bool startupReady;
  bool lockedIdle;
};

// Borrow an owner-serialized binding for the call. Timed owners supply the
// current remaining budget; untimed observations can pass the per-callback cap.
TransferResult readWord(RegisterTransport transport, std::uint8_t pointer, std::uint16_t& out,
                        std::uint32_t remainingMs);
TransferResult writeWord(RegisterTransport transport, std::uint8_t pointer, std::uint16_t value,
                         WriteAccess access, std::uint32_t remainingMs);

// Checked frame boundary used by the word helpers. Only one complete word is
// legal. A failure leaves rx untouched, including an adapter-reported short.
TransferResult readFrame(RegisterTransport transport, const std::uint8_t* tx, std::size_t txLen,
                         std::uint8_t* rx, std::size_t rxLen, std::uint32_t remainingMs);
TransferResult writeFrame(RegisterTransport transport, const std::uint8_t* tx, std::size_t txLen,
                          WriteAccess access, std::uint32_t remainingMs);

}  // namespace TMP11x::detail
