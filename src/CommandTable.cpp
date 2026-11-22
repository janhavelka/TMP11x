#include "TMP11x/CommandTable.h"

#include "ModelDescriptor.h"

namespace TMP11x::command {

Access accessFor(Model model, std::uint8_t pointer) noexcept {
  const auto* descriptor = detail::findModelDescriptor(model);
  if (descriptor == nullptr) return Access::INVALID;

  switch (pointer) {
    case temperature:
    case deviceId: return Access::OBSERVE;
    case config: return Access::STABLE_CONFIG;
    case highLimit:
    case lowLimit: return Access::GUARDED_LIMIT;
    case eepromUnlock: return Access::PROTECTED_WORD;
  }
  if (pointer == offset && descriptor->capabilities.hasOffset) return Access::GUARDED_OFFSET;
  for (std::uint8_t i = 0; i < descriptor->capabilities.eepromWordCount; ++i) {
    if (descriptor->eepromPointers[i] == pointer) return Access::PROTECTED_WORD;
  }
  return Access::INVALID;
}

}  // namespace TMP11x::command
