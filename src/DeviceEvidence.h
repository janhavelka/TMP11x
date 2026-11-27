#pragma once

#include "RegisterTransport.h"

namespace TMP11x::detail {

// Record the physical outcome before the owner resolves its deadline or publishes output.
struct DeviceEvidence {
  TMP11x& device;

  // Record a complete EEPROM_UL word, including when its callback was late.
  Status observeGuard(std::uint16_t raw, std::optional<std::uint32_t> tick) noexcept;

  // Compare owned registers, including managed OFFSET, and check unmanaged calibration continuity.
  Status observeRegister(std::uint8_t pointer, std::uint16_t raw, const TransferResult& transfer,
                         const SensorSettings* expected, ConfigReadSource source, bool timely = true) noexcept;
  void observeTemperature(const TransferResult& transfer) noexcept;
  void observeWrite(std::uint8_t pointer, std::uint16_t raw, const TransferResult& transfer) noexcept;

 private:
  void observeBusy(std::optional<std::uint32_t> tick) noexcept;
  void loseReadiness() noexcept;
  void markDirty(DirtyReason reason, SettingsField field) noexcept;
  void captureConfig(std::uint16_t raw, std::optional<std::uint32_t> tick, ConfigReadSource source,
                     bool timely) noexcept;
};

}  // namespace TMP11x::detail
