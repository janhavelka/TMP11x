#pragma once

#include "OperationBudget.h"

namespace TMP11x::detail {

// One synchronous addressed reset. The command and subsequent observations
// share the deadline established during preflight.
struct Reset {
  explicit Reset(TMP11x& device) noexcept : _device(device) {}
  Reset(const Reset&) = delete;
  Reset& operator=(const Reset&) = delete;

  // Success here means only that the addressed write completed within budget.
  Status issueCommand();

 private:
  Status awaitDispatch();
  TMP11x& _device;
  OperationBudget _budget;
};

}  // namespace TMP11x::detail
