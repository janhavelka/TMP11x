#pragma once

#include "OperationBudget.h"

namespace TMP11x::detail {

// One synchronous addressed reset. The command and subsequent observations
// share the deadline established during preflight.
struct Reset {
  explicit Reset(TMP11x& device) noexcept : _device(device) {}
  Reset(const Reset&) = delete;
  Reset& operator=(const Reset&) = delete;

  Status run();

 private:
  Status awaitDispatch(std::uint32_t delayUs = 0);
  Status readGuard(std::uint32_t delayUs = 0);
  Status issueCommand();
  Status readProfile();
  TMP11x& _device;
  OperationBudget _budget;
};

}  // namespace TMP11x::detail
