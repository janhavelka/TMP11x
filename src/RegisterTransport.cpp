#include "RegisterTransport.h"

#include <algorithm>

#include "TMP11x/CommandTable.h"

namespace TMP11x::detail {

Status validateTransportBinding(const Config& binding) noexcept {
  if (!isValidModel(binding.model) || !isValidAddress(binding.i2cAddress) ||
      binding.i2cWrite == nullptr || binding.i2cWriteRead == nullptr ||
      !isValidInterval(binding.i2cTimeoutMs)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid register transport binding");
  }
  return Status::Ok();
}

namespace {

Status admitTransfer(const Config& binding, std::uint32_t remainingMs) {
  const auto status = validateTransportBinding(binding);
  if (!status.ok()) return status;
  if (remainingMs == 0) return Status::Error(Err::OPERATION_TIMEOUT, "Transfer budget expired");
  return Status::Ok();
}

TransferResult callbackResult(Status status) {
  switch (status.code) {
    case Err::OK:
    case Err::I2C_ERROR:
    case Err::I2C_NACK:
    case Err::I2C_NACK_ADDR:
    case Err::I2C_NACK_DATA:
    case Err::I2C_SHORT_TRANSFER:
    case Err::I2C_TIMEOUT:
    case Err::I2C_BUS: return {status, true};
    default: return {Status::Error(Err::I2C_ERROR, "Invalid I2C callback status", status.detail), true};
  }
}

std::uint16_t decodeWord(const std::uint8_t* bytes) {
  return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8) | bytes[1]);
}

Status checkWrite(command::Access policy, std::uint16_t value, WriteAccess access) {
  switch (policy) {
    case command::Access::INVALID:
      return Status::Error(Err::INVALID_PARAM, "Unmapped register pointer");
    case command::Access::OBSERVE:
    case command::Access::PROTECTED_WORD:
      return Status::Error(Err::UNSUPPORTED, "Register writes are prohibited");
    case command::Access::STABLE_CONFIG:
      if (access.purpose != WritePurpose::STABLE_CONFIG || (value & ~command::stableConfigMask) != 0) {
        return Status::Error(Err::INVALID_PARAM, "Expected canonical stable CONFIG fields");
      }
      break;
    case command::Access::GUARDED_LIMIT:
      if (access.purpose != WritePurpose::LIMIT) {
        return Status::Error(Err::INVALID_PARAM, "Expected limit write");
      }
      break;
    case command::Access::GUARDED_OFFSET:
      if (access.purpose != WritePurpose::MANAGED_OFFSET) {
        return Status::Error(Err::INVALID_PARAM, "Expected managed offset write");
      }
      break;
  }
  if (!access.startupReady || !access.lockedIdle) {
    return Status::Error(Err::INVALID_CONFIG, "Missing startup or locked-idle write guard");
  }
  return Status::Ok();
}

}  // namespace

TransferResult readFrame(const Config& binding, const std::uint8_t* tx, std::size_t txLen,
                         std::uint8_t* rx, std::size_t rxLen, std::uint32_t remainingMs) {
  const auto admission = admitTransfer(binding, remainingMs);
  if (!admission.ok()) return {admission, false};
  if (tx == nullptr || txLen != 1 || rx == nullptr || rxLen != 2) {
    return {Status::Error(Err::INVALID_PARAM, "Expected one pointer and two receive bytes"), false};
  }
  if (command::accessFor(binding.model, tx[0]) == command::Access::INVALID) {
    return {Status::Error(Err::INVALID_PARAM, "Unmapped register pointer"), false};
  }

  std::uint8_t scratch[2]{};
  const auto grant = std::min(binding.i2cTimeoutMs, remainingMs);
  const auto result = callbackResult(binding.i2cWriteRead(binding.i2cAddress, tx, txLen, scratch,
                                                         sizeof(scratch), grant, binding.i2cUser));
  if (result.status.ok()) {
    rx[0] = scratch[0];
    rx[1] = scratch[1];
  }
  return result;
}

TransferResult writeFrame(const Config& binding, const std::uint8_t* tx, std::size_t txLen,
                          WriteAccess access, std::uint32_t remainingMs) {
  const auto admission = admitTransfer(binding, remainingMs);
  if (!admission.ok()) return {admission, false};
  if (tx == nullptr || txLen != 3) {
    return {Status::Error(Err::INVALID_PARAM, "Expected a pointer and two data bytes"), false};
  }
  const auto permission = checkWrite(command::accessFor(binding.model, tx[0]), decodeWord(tx + 1), access);
  if (!permission.ok()) return {permission, false};

  const auto grant = std::min(binding.i2cTimeoutMs, remainingMs);
  return callbackResult(binding.i2cWrite(binding.i2cAddress, tx, txLen, grant, binding.i2cUser));
}

TransferResult readWord(const Config& binding, std::uint8_t pointer, std::uint16_t& out,
                        std::uint32_t remainingMs) {
  std::uint8_t bytes[2]{};
  const auto result = readFrame(binding, &pointer, 1, bytes, sizeof(bytes), remainingMs);
  if (result.status.ok()) out = decodeWord(bytes);
  return result;
}

TransferResult writeWord(const Config& binding, std::uint8_t pointer, std::uint16_t value,
                         WriteAccess access, std::uint32_t remainingMs) {
  const std::uint8_t bytes[] = {pointer, static_cast<std::uint8_t>(value >> 8),
                                static_cast<std::uint8_t>(value & 0xFF)};
  return writeFrame(binding, bytes, sizeof(bytes), access, remainingMs);
}

}  // namespace TMP11x::detail
