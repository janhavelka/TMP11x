#pragma once

#include "RegisterTransport.h"
#include "TMP11x/TMP11x.h"

namespace TMP11x::detail {

// A complete ID can fail the family check after a successful transfer.
// The transport view selects whether the read records physical health.
struct IdentityResult {
  Status status;
  TransferResult transfer;
};

// Interpret a complete DEVICE_ID word, retaining its raw bits even on family mismatch.
Status decodeIdentity(const Config& binding, std::uint16_t raw, IdentityInfo& out) noexcept;

// Read DEVICE_ID once at the bound address. A complete word replaces out even
// on IDENTITY_MISMATCH. Local refusal and transport failure preserve out.
IdentityResult readIdentity(RegisterTransport transport, IdentityInfo& out, std::uint32_t remainingMs);

}  // namespace TMP11x::detail
