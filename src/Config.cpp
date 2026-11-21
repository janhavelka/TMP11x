#include "ModelDescriptor.h"

namespace TMP11x {

const detail::ModelDescriptor* detail::findModelDescriptor(Model model) noexcept {
  if (model == Model::UNSELECTED) {
    return nullptr;
  }
  for (const auto& row : modelTable) {
    for (const auto supported : row.models) {
      if (supported == model) {
        return &row.descriptor;
      }
    }
  }
  return nullptr;
}

bool isValidModel(Model model) noexcept {
  return detail::findModelDescriptor(model) != nullptr;
}

Status getCapabilities(Model model, Capabilities& out) noexcept {
  const auto* descriptor = detail::findModelDescriptor(model);
  if (descriptor == nullptr) {
    return Status::Error(Err::INVALID_PARAM, "Unsupported model selection");
  }
  out = descriptor->capabilities;
  return Status::Ok();
}

}  // namespace TMP11x
