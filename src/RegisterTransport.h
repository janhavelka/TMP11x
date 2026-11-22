#pragma once

#include "TMP11x/Config.h"

namespace TMP11x::detail {

// A local refusal costs zero transfers. A callback, even an incoherent or failed
// one, costs one and may already have affected the device.
struct TransferResult {
  Status status;
  bool attempted;
};

enum class WritePurpose { STABLE_CONFIG, LIMIT, MANAGED_OFFSET };

// Supplied by the typed operation owner. Both proofs belong to the current
// target and uninterrupted operation. The serializer never acquires a guard.
struct WriteAccess {
  WritePurpose purpose;
  bool startupReady;
  bool lockedIdle;
};

// Borrow an owner-serialized binding for the call. Timed owners supply the
// current remaining budget; untimed observations can pass the per-callback cap.
TransferResult readWord(const Config& binding, std::uint8_t pointer, std::uint16_t& out,
                        std::uint32_t remainingMs);
TransferResult writeWord(const Config& binding, std::uint8_t pointer, std::uint16_t value,
                         WriteAccess access, std::uint32_t remainingMs);

// Checked frame boundary used by the word helpers. Only one complete word is
// legal. A failure leaves rx untouched, including an adapter-reported short.
TransferResult readFrame(const Config& binding, const std::uint8_t* tx, std::size_t txLen,
                         std::uint8_t* rx, std::size_t rxLen, std::uint32_t remainingMs);
TransferResult writeFrame(const Config& binding, const std::uint8_t* tx, std::size_t txLen,
                          WriteAccess access, std::uint32_t remainingMs);

}  // namespace TMP11x::detail
