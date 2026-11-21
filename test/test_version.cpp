#include "TMP11x/Version.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static_assert(noexcept(TMP11x::version()));

int main() {
  const std::string expected = std::to_string(TMP11x::VERSION_MAJOR) + "." +
                               std::to_string(TMP11x::VERSION_MINOR) + "." +
                               std::to_string(TMP11x::VERSION_PATCH);
  if (TMP11x::VERSION != expected) {
    std::fputs("Version text disagrees with the numeric components\n", stderr);
    return EXIT_FAILURE;
  }

  const char* linked = TMP11x::version();
  if (linked == nullptr || linked != expected) {
    std::fputs("Linked library version disagrees with the header\n", stderr);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
