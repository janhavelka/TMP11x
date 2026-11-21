#include "TMP11x/Version.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

int main() {
  const char* linked = TMP11x::version();
  if (linked == nullptr || std::string_view{linked} != TMP11x::VERSION) {
    return EXIT_FAILURE;
  }
  std::puts(linked);
  return EXIT_SUCCESS;
}
