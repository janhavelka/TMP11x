#include "TMP11x/CommandTable.h"

#include "ModelDescriptor.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace {

using namespace TMP11x;
namespace cmd = TMP11x::command;

// Independent literals from the three stage 5 register profiles.
static_assert(cmd::temperature == 0x00 && cmd::config == 0x01);
static_assert(cmd::highLimit == 0x02 && cmd::lowLimit == 0x03);
static_assert(cmd::eepromUnlock == 0x04 && cmd::eeprom1 == 0x05 && cmd::eeprom2 == 0x06);
static_assert(cmd::eeprom3Tmp116 == 0x07 && cmd::offset == 0x07 && cmd::lastEeprom == 0x08);
static_assert(cmd::deviceId == 0x0F && cmd::wholeWord == 0xFFFF);
static_assert(cmd::baseIdMask == 0x0FFF && cmd::revisionMask == 0xF000);
static_assert(cmd::highFlag == 0x8000 && cmd::lowFlag == 0x4000);
static_assert(cmd::dataReady == 0x2000 && cmd::configBusy == 0x1000);
static_assert(cmd::modeMask == 0x0C00 && cmd::cycleMask == 0x0380 && cmd::averagingMask == 0x0060);
static_assert(cmd::therm == 0x0010 && cmd::polarity == 0x0008 && cmd::dataReadyPin == 0x0004);
static_assert(cmd::stableConfigMask == 0x07FC && cmd::softReset == 0x0002);
static_assert(cmd::configReserved(false) == 0x0003 && cmd::configReserved(true) == 0x0001);
static_assert(cmd::unlock == 0x8000 && cmd::unlockBusy == 0x4000 && cmd::unlockReserved == 0x3FFF);

void require(bool matched, Model model, unsigned ptr) {
  if (!matched) {
    std::fprintf(stderr, "Command metadata mismatch (model=%u, pointer=0x%02X)\n",
                 static_cast<unsigned>(model), ptr);
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace

void testCommandTable() {
  using A = cmd::Access;
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    const bool tmp116 = model == Model::TMP116 || model == Model::TMP116N;
    const A expected[] = {A::OBSERVE, A::STABLE_CONFIG, A::GUARDED_LIMIT, A::GUARDED_LIMIT,
                          A::PROTECTED_WORD, A::PROTECTED_WORD, A::PROTECTED_WORD,
                          tmp116 ? A::PROTECTED_WORD : A::GUARDED_OFFSET, A::PROTECTED_WORD};
    for (unsigned ptr = 0; ptr <= 0xFF; ++ptr) {
      auto access = A::INVALID;
      if (ptr <= 8) access = expected[ptr];
      if (ptr == 0x0F) access = A::OBSERVE;
      require(cmd::accessFor(model, static_cast<std::uint8_t>(ptr)) == access, model, ptr);
    }
    const auto* descriptor = detail::findModelDescriptor(model);
    require(descriptor != nullptr, model, 0x0F);
    const auto& caps = descriptor->capabilities;
    require(caps.hasOffset == !tmp116 && caps.hasAddressedReset == !tmp116, model, 0x07);
    require(caps.eepromWordCount == (tmp116 ? 4 : 3), model, 0x08);
    const std::uint8_t words116[] = {5, 6, 7, 8};
    const std::uint8_t words117[] = {5, 6, 8};
    for (unsigned i = 0; i < caps.eepromWordCount; ++i) {
      const auto ptr = tmp116 ? words116[i] : words117[i];
      require(descriptor->eepromPointers[i] == ptr, model, ptr);
    }
    require(descriptor->idMask == (tmp116 ? 0xFFFF : 0x0FFF), model, 0x0F);
    require(descriptor->revisionMask == (tmp116 ? 0 : 0xF000), model, 0x0F);
  }
  for (auto invalid : {Model::UNSELECTED, static_cast<Model>(0xFF)}) {
    require(cmd::accessFor(invalid, 0x0F) == A::INVALID, invalid, 0x0F);
  }
}
