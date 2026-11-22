#pragma once

#include "TMP11x/CommandTable.h"

namespace TMP11x::detail {

// Shared policy for models with the same register map and ID interpretation.
struct ModelDescriptor {
  Capabilities capabilities;
  IdentityGroup identityGroup;
  std::uint16_t idMask;
  std::uint16_t idValue;
  std::uint16_t revisionMask;  // Zero when the ID has no documented revision field.
  std::uint8_t revisionShift;
  std::uint8_t eepromPointers[4];  // Only the first capabilities.eepromWordCount entries are mapped.
};

struct ModelRow {
  Model models[4];  // Unused entries are Model::UNSELECTED (zero).
  ModelDescriptor descriptor;
};

// SBOS740A pp. 24, 26, 31; SNOSD82D pp. 25, 28, 32; SNIS236 pp. 32, 35, 39.
inline constexpr ModelRow modelTable[] = {
    {{Model::TMP116, Model::TMP116N},
     {{false, false, 4}, IdentityGroup::TMP116, command::wholeWord, 0x1116, 0, 0,
      {command::eeprom1, command::eeprom2, command::eeprom3Tmp116, command::lastEeprom}}},
    {{Model::TMP117, Model::TMP117M, Model::TMP117N, Model::TMP119},
     {{true, true, 3}, IdentityGroup::TMP117_LIKE, command::baseIdMask, 0x0117, command::revisionMask, 12,
      {command::eeprom1, command::eeprom2, command::lastEeprom}}},
};

// Returns a static descriptor for a valid selection, or nullptr.
const ModelDescriptor* findModelDescriptor(Model model) noexcept;

}  // namespace TMP11x::detail
