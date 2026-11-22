#include "Identity.h"

#include "ModelDescriptor.h"

namespace TMP11x::detail {

IdentityResult readIdentity(RegisterTransport transport, IdentityInfo& out, std::uint32_t remainingMs) {
  IdentityInfo observed{};
  observed.model = transport.binding.model;
  observed.i2cAddress = transport.binding.i2cAddress;
  std::uint16_t raw = 0;
  const auto transfer = readWord(transport, command::deviceId, raw, remainingMs);
  if (!transfer.status.ok()) return {transfer.status, transfer};

  observed.raw = raw;
  observed.rawValid = true;
  for (const auto& row : modelTable) {
    const auto& descriptor = row.descriptor;
    if ((raw & descriptor.idMask) != descriptor.idValue) continue;

    observed.group = descriptor.identityGroup;
    observed.revisionValid = descriptor.revisionMask != 0;
    observed.revision = static_cast<std::uint8_t>(
        (raw & descriptor.revisionMask) >> descriptor.revisionShift);
    // readWord validated the selected model before admitting the transfer.
    observed.compatible = observed.group == findModelDescriptor(observed.model)->identityGroup;
    break;
  }

  out = observed;
  if (observed.compatible) return {transfer.status, transfer};
  return {Status::Error(Err::IDENTITY_MISMATCH, "Incompatible device ID", raw), transfer};
}

}  // namespace TMP11x::detail
