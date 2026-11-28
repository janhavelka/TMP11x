#include "Identity.h"

#include "ModelDescriptor.h"

namespace TMP11x::detail {

Status decodeIdentity(const Config& binding, std::uint16_t raw, IdentityInfo& out) noexcept {
  IdentityInfo observed{};
  observed.model = binding.model;
  observed.i2cAddress = binding.i2cAddress;
  observed.raw = raw;
  observed.rawValid = true;
  for (const auto& row : modelTable) {
    const auto& descriptor = row.descriptor;
    if ((raw & descriptor.idMask) != descriptor.idValue) continue;

    observed.group = descriptor.identityGroup;
    observed.revisionValid = descriptor.revisionMask != 0;
    observed.revision = static_cast<std::uint8_t>(
        (raw & descriptor.revisionMask) >> descriptor.revisionShift);
    const auto* selected = findModelDescriptor(observed.model);
    observed.compatible = selected && observed.group == selected->identityGroup;
    break;
  }

  out = observed;
  if (observed.compatible) return Status::Ok();
  return Status::Error(Err::IDENTITY_MISMATCH, "Incompatible device ID", raw);
}

IdentityResult readIdentity(RegisterTransport transport, IdentityInfo& out, std::uint32_t remainingMs) {
  std::uint16_t raw{};
  const auto transfer = readWord(transport, command::deviceId, raw, remainingMs);
  if (!transfer.status.ok()) return {transfer.status, transfer};
  const auto family = decodeIdentity(transport.binding, raw, out);
  return {family.ok() ? transfer.status : family, transfer};
}

}  // namespace TMP11x::detail
