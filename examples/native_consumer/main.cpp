#include "TMP11x/CommandTable.h"
#include "TMP11x/TMP11x.h"
#include "TMP11x/Version.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

int main() {
  const TMP11x::TMP11x sensor;
  const TMP11x::HealthSnapshot health = sensor.healthSnapshot();
  if (health.state != TMP11x::DriverState::UNINIT || health.initialized || sensor.isOnline() ||
      health.totalSuccess != 0 || health.totalFailures != 0 || !health.lastError.ok() ||
      health.lastSuccessMsValid || health.lastErrorMsValid) {
    return EXIT_FAILURE;
  }
  if (TMP11x::command::accessFor(TMP11x::Model::TMP117, TMP11x::command::offset) !=
      TMP11x::command::Access::GUARDED_OFFSET) {
    return EXIT_FAILURE;
  }
  const char* linked = TMP11x::version();
  if (linked == nullptr || std::string_view{linked} != TMP11x::VERSION) {
    return EXIT_FAILURE;
  }
  std::puts(linked);
  return EXIT_SUCCESS;
}
