#pragma once

#include "RegisterTransport.h"
#include "TMP11x/TMP11x.h"

namespace TMP11x::detail {

// A complete ID can fail the family check after a successful transfer.
// The caller decides whether this operation tracks physical transport health.
struct IdentityResult {
  Status status;
  TransferResult transfer;
};

// Read DEVICE_ID once at the bound address. A complete word replaces out even
// on IDENTITY_MISMATCH. Local refusal and transport failure preserve out.
IdentityResult readIdentity(const Config& binding, IdentityInfo& out, std::uint32_t remainingMs);

}  // namespace TMP11x::detail
