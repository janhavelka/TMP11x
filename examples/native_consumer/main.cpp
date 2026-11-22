#include "TMP11x/CommandTable.h"
#include "TMP11x/Version.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

int main() {
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
