#include "TMP11x/TMP11x.h"

#include "DeviceEvidence.h"
#include "Identity.h"
#include "ModelDescriptor.h"
#include "TMP11x/Temperature.h"

namespace TMP11x {

const char* RegisterObservation::name() const noexcept {
  const auto* model = detail::findModelDescriptor(identity.model);
  if (!model) return "UNKNOWN";
  switch (pointer) {
    case command::temperature: return "TEMP";
    case command::config: return "CONFIG";
    case command::highLimit: return "THIGH";
    case command::lowLimit: return "TLOW";
    case command::eepromUnlock: return "EEPROM_UL";
    case command::deviceId: return "DEVICE_ID";
  }
  if (pointer == command::offset && model->capabilities.hasOffset) return "OFFSET";
  constexpr const char* shadows[] = {"EEPROM1", "EEPROM2", "EEPROM3", "EEPROM4"};
  for (std::uint8_t i = 0; i < model->capabilities.eepromWordCount; ++i) {
    if (pointer == model->eepromPointers[i]) return shadows[i];
  }
  return "UNKNOWN";
}

std::optional<std::int16_t> RegisterObservation::signedCounts() const noexcept {
  const auto access = command::accessFor(identity.model, pointer);
  if ((access == command::Access::OBSERVE && pointer == command::temperature) ||
      access == command::Access::GUARDED_LIMIT || access == command::Access::GUARDED_OFFSET) {
    return temperature::decodeCounts(raw);
  }
  return std::nullopt;
}

bool EepromLockObservation::unlocked() const noexcept {
  return (raw & command::unlock) != 0;
}

bool EepromLockObservation::busy() const noexcept {
  return (raw & command::unlockBusy) != 0;
}

Status TMP11x::readDiagnosticWord(std::uint8_t pointer, RegisterObservation& out, bool timestamp) {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  const auto access = command::accessFor(_config.model, pointer);
  if (access == command::Access::INVALID) return Status::Error(Err::INVALID_PARAM, "Unmapped register pointer");
  if (pointer != command::deviceId && _identity.rawValid && !_identity.compatible) {
    return Status::Error(Err::IDENTITY_MISMATCH, "Incompatible device ID", _identity.raw);
  }

  std::uint16_t raw{};
  auto transfer = detail::readWord(_config, pointer, raw, _config.i2cTimeoutMs);
  if (timestamp && transfer.status.ok() && _config.nowMs) {
    transfer.completedMs = _config.nowMs(_config.timeUser);
  }

  detail::DeviceEvidence evidence{*this};
  std::uint16_t clearMask = 0;
  if (pointer == command::temperature) {
    evidence.observeTemperature(transfer);
    clearMask = command::dataReady;
  } else if (access == command::Access::STABLE_CONFIG || access == command::Access::GUARDED_LIMIT ||
             access == command::Access::GUARDED_OFFSET) {
    // Before a replay there is no live profile against which to compare these words.
    const auto* expected = _live.trust == ConfigTrust::UNKNOWN ? nullptr : &_config.settings;
    evidence.observeRegister(pointer, raw, transfer, expected, ConfigReadSource::DIAGNOSTIC);
    if (pointer == command::config && transfer.status.ok()) clearMask = _live.config->readClearMask;
  } else if (transfer.attempted && !transfer.status.ok()) {
    _live.guardInterruption = transfer.status;
  }
  if (!transfer.status.ok()) return transfer.status;

  auto status = transfer.status;
  if (pointer == command::deviceId) {
    const auto family = detail::decodeIdentity(_config, raw, _identity);
    if (!family.ok()) {
      clearLiveState();
      status = family;
    }
  } else if (pointer == command::eepromUnlock) {
    // Retain EUN and BUSY for guarded operations while returning the complete diagnostic word.
    evidence.observeGuard(raw, transfer.completedMs);
  }
  out = {_identity, pointer, raw, transfer.completedMs, clearMask,
         _live.trust, _live.startupReady, _live.profileGeneration};
  return status;
}

Status TMP11x::readRegister(std::uint8_t pointer, RegisterObservation& out) {
  return readDiagnosticWord(pointer, out, true);
}

Status TMP11x::readIdentity(IdentityInfo& out) {
  RegisterObservation word;
  const auto status = readDiagnosticWord(command::deviceId, word, false);
  if (word.identity.rawValid) out = word.identity;
  return status;
}

Status TMP11x::probe() {
  IdentityInfo id;
  return readIdentity(id);
}

Status TMP11x::readRegisterSnapshot(RegisterSnapshot& out) {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  out = {};
  out.identity.model = _config.model;
  out.identity.i2cAddress = _config.i2cAddress;
  constexpr std::uint8_t pointers[] = {
      command::deviceId, command::temperature, command::config, command::highLimit, command::lowLimit,
      command::eepromUnlock, command::eeprom1, command::eeprom2, command::offset, command::lastEeprom};
  for (const auto pointer : pointers) {
    auto& word = out.words[out.wordCount];
    const auto status = readRegister(pointer, word);
    if (pointer == command::deviceId && word.identity.rawValid) out.identity = word.identity;
    if (!status.ok()) {
      out.failedPointer = pointer;
      if (pointer == command::deviceId && word.identity.rawValid) ++out.wordCount;
      if (pointer == command::config) {
        // A short CONFIG transfer supplies no reliable comparator mode.
        out.possibleReadClearMask = command::dataReady | command::highFlag | command::lowFlag;
      }
      if (pointer == command::temperature) out.possibleReadClearMask = command::dataReady;
      return status;
    }
    ++out.wordCount;
  }
  return Status::Ok();
}

Status TMP11x::readEepromWord(std::uint8_t index, std::uint16_t& out) {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  const auto& map = *detail::findModelDescriptor(_config.model);
  if (index == 0 || index > map.capabilities.eepromWordCount) {
    return Status::Error(Err::INVALID_PARAM, "EEPROM index outside the selected model's range", index);
  }
  RegisterObservation word;
  const auto status = readDiagnosticWord(map.eepromPointers[index - 1], word, false);
  if (status.ok()) out = word.raw;
  return status;
}

Status TMP11x::readEepromSnapshot(EepromSnapshot& out) {
  if (!isBound()) return Status::Error(Err::NOT_BOUND, "No sensor binding");
  out = {};
  out.identity.model = _config.model;
  out.identity.i2cAddress = _config.i2cAddress;
  RegisterObservation word;
  const auto idStatus = readRegister(command::deviceId, word);
  if (word.identity.rawValid) {
    out.identity = word.identity;
    out.identityAtMs = word.observedAtMs;
  }
  if (!idStatus.ok()) {
    out.failedPointer = command::deviceId;
    return idStatus;
  }

  const auto lockRead = readRegister(command::eepromUnlock, word);
  if (!lockRead.ok()) {
    out.failedPointer = command::eepromUnlock;
    return lockRead;
  }
  out.lock = EepromLockObservation{word.raw, word.observedAtMs};

  const auto& map = *detail::findModelDescriptor(_config.model);
  while (out.wordCount < map.capabilities.eepromWordCount) {
    const auto pointer = map.eepromPointers[out.wordCount];
    const auto status = readRegister(pointer, word);
    if (!status.ok()) {
      out.failedPointer = pointer;
      return status;
    }
    out.words[out.wordCount++] = {pointer, word.raw, word.observedAtMs};
  }
  return Status::Ok();
}

}  // namespace TMP11x
