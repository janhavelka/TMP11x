#include "Identity.h"
#include "support/FakeTmp11xDevice.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

using namespace TMP11x;
using namespace TMP11x::detail;
using namespace TMP11x::test;

#define CHECK(condition)                                                           \
  do {                                                                             \
    if (!(condition)) {                                                            \
      std::fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);        \
      std::exit(EXIT_FAILURE);                                                     \
    }                                                                              \
  } while (false)

Config configFor(FakeTmp11xBus& bus, Model selected, std::uint8_t address) {
  Config binding;
  binding.model = selected;
  binding.i2cAddress = address;
  binding.i2cUser = &bus;
  binding.i2cWrite = FakeTmp11xBus::write;
  binding.i2cWriteRead = FakeTmp11xBus::writeRead;
  binding.i2cTimeoutMs = 19;
  return binding;
}

constexpr IdentityInfo previous{Model::TMP117M, 0x4B, 0xC117, true,
                                IdentityGroup::TMP117_LIKE, 12, true, true};

void checkPreserved(const IdentityInfo& out) {
  CHECK(out.model == previous.model && out.i2cAddress == previous.i2cAddress);
  CHECK(out.raw == previous.raw && out.rawValid == previous.rawValid);
  CHECK(out.group == previous.group && out.compatible == previous.compatible);
  CHECK(out.revision == previous.revision && out.revisionValid == previous.revisionValid);
}

const TransferTrace& checkFrame(const FakeTmp11xBus& bus, std::size_t before,
                                std::uint8_t address, std::uint32_t grant) {
  CHECK(bus.trace.size() == before + 1);
  const auto& frame = bus.trace.back();
  CHECK(frame.combinedRead && frame.address == address && frame.timeoutMs == grant);
  CHECK(frame.txLen == 1 && frame.tx == std::vector<std::uint8_t>{0x0F});
  CHECK(frame.rxLen == 2);
  return frame;
}

// An absent revision is represented by -1 only in these expected test vectors.
void checkObservation(const FakeTmp11xBus& bus, const Config& binding, std::uint16_t raw,
                      IdentityGroup group, int revision, bool compatible) {
  const auto before = bus.trace.size();
  // Exercise first publication on a fresh bus and replacement of prior evidence thereafter.
  IdentityInfo out = before == 0 ? IdentityInfo{} : previous;
  const auto result = readIdentity(binding, out, 7);
  const auto& frame = checkFrame(bus, before, binding.i2cAddress, 7);
  CHECK(frame.result.ok() && result.transfer.attempted && result.transfer.status.ok());
  CHECK((frame.rx == std::vector<std::uint8_t>{static_cast<std::uint8_t>(raw >> 8),
                                               static_cast<std::uint8_t>(raw)}));
  CHECK(result.status.is(compatible ? Err::OK : Err::IDENTITY_MISMATCH));
  if (!compatible) CHECK(result.status.detail == raw);
  CHECK(out.model == binding.model && out.i2cAddress == binding.i2cAddress);
  CHECK(out.rawValid && out.raw == raw && out.group == group && out.compatible == compatible);
  CHECK(out.revisionValid == (revision >= 0));
  if (out.revisionValid) CHECK(out.revision == revision);
}

void selectedModelsAndIdBits() {
  for (const auto model : {Model::TMP116, Model::TMP116N, Model::TMP117,
                           Model::TMP117M, Model::TMP117N, Model::TMP119}) {
    FakeTmp11xDevice device{model};
    FakeTmp11xBus bus;
    CHECK(bus.attach(0x49, device));
    const auto binding = configFor(bus, model, 0x49);
    const bool selects116 = model == Model::TMP116 || model == Model::TMP116N;
    auto& id = device.words[0x0F];
    id = 0x1116;
    checkObservation(bus, binding, id, IdentityGroup::TMP116, -1, selects116);
    for (int revision = 0; revision < 16; ++revision) {
      id = static_cast<std::uint16_t>((revision << 12) | 0x0117);
      checkObservation(bus, binding, id, IdentityGroup::TMP117_LIKE, revision, !selects116);
    }
    // TMP114 (0x1114) and TMP118 (0x1180) expose their IDs at 0x0B.
    // Here an arbitrary responder returns those bit patterns at 0x0F.
    for (const auto word : {0x0116, 0x2116, 0x1114, 0x1180}) {
      id = static_cast<std::uint16_t>(word);
      checkObservation(bus, binding, id, IdentityGroup::UNKNOWN, -1, false);
    }
    // Challenge each significant DID bit individually. Flipping bit 0 of
    // 0x1116 yields 0x1117, already covered above as a recognized other group.
    const int firstBit = selects116 ? 1 : 0;
    const int comparedBits = selects116 ? 16 : 12;
    for (int bit = firstBit; bit < comparedBits; ++bit) {
      id = static_cast<std::uint16_t>((selects116 ? 0x1116 : 0xA117) ^ (1u << bit));
      checkObservation(bus, binding, id, IdentityGroup::UNKNOWN, -1, false);
    }
  }
}

void checkFailedRead(FakeTmp11xBus& bus, const Config& binding, Status error,
                     std::size_t delivered = 0) {
  TransferOutcome fault;
  fault.status = error;
  fault.failedReadBytes = delivered;
  bus.script.push_back(fault);
  const auto before = bus.trace.size();
  IdentityInfo out = previous;
  const auto result = readIdentity(binding, out, 63);
  const auto& frame = checkFrame(bus, before, binding.i2cAddress, 19);
  CHECK(frame.rx.size() == delivered && frame.result.is(error.code));
  CHECK(result.transfer.attempted && result.transfer.status.is(error.code));
  CHECK(result.status.is(error.code) && result.status.detail == error.detail);
  CHECK(result.transfer.status.detail == error.detail);
  checkPreserved(out);
}

void sharedBusNeighbors() {
  FakeTmp11xBus bus;
  // The stage 12 fixture supplies register storage and traces. Its model
  // configures write behavior only. These probes exercise just the ID frame.
  FakeTmp11xDevice tmp114{Model::TMP117}, tmp118{Model::TMP117};
  FakeTmp11xDevice unrelated{Model::TMP117}, target{Model::TMP117};
  // Non-indexed-read rules and maps imply zero here, with TMP114 CRC disabled.
  tmp114.words[0x0F] = 0;
  tmp118.words[0x0F] = 0;
  unrelated.words[0x0F] = 0x2117;
  target.words[0x0F] = 0x0117;
  CHECK(bus.attach(0x48, tmp114) && bus.attach(0x49, tmp118));
  CHECK(bus.attach(0x4A, unrelated) && bus.attach(0x4B, target));

  const auto tmp114Binding = configFor(bus, Model::TMP117, 0x48);
  checkObservation(bus, tmp114Binding, 0, IdentityGroup::UNKNOWN, -1, false);
  checkObservation(bus, configFor(bus, Model::TMP117, 0x49), 0, IdentityGroup::UNKNOWN, -1, false);
  checkObservation(bus, configFor(bus, Model::TMP117, 0x4A), 0x2117, IdentityGroup::TMP117_LIKE, 2, true);
  checkObservation(bus, configFor(bus, Model::TMP117, 0x4B), 0x0117, IdentityGroup::TMP117_LIKE, 0, true);

  // Alternative adverse TMP114 CRC responses, scripted independently of the
  // ordinary zero case. TI leaves the exact omitted-CRC outcome unspecified.
  checkFailedRead(bus, tmp114Binding, Status::Error(Err::I2C_NACK_DATA, "scripted CRC NACK", -14));
  tmp114.words[0x0F] = 0xFFFF;
  checkObservation(bus, tmp114Binding, 0xFFFF, IdentityGroup::UNKNOWN, -1, false);
}

void failedReadsAndLocalRefusals() {
  FakeTmp11xDevice device{Model::TMP116};
  device.words[0x0F] = 0x1116;
  FakeTmp11xBus bus;
  CHECK(bus.attach(0x48, device));
  const auto binding = configFor(bus, Model::TMP116, 0x48);
  checkFailedRead(bus, binding, Status::Error(Err::I2C_NACK_ADDR, "address NACK", 11));
  checkFailedRead(bus, binding, Status::Error(Err::I2C_TIMEOUT, "timeout", -22));
  checkFailedRead(bus, binding, Status::Error(Err::I2C_SHORT_TRANSFER, "empty", 0));
  checkFailedRead(bus, binding, Status::Error(Err::I2C_SHORT_TRANSFER, "one byte", 1), 1);

  auto checkRefusal = [&](const Config& trial, std::uint32_t remainingMs, Err error) {
    const auto before = bus.trace.size();
    IdentityInfo out = previous;
    const auto result = readIdentity(trial, out, remainingMs);
    CHECK(bus.trace.size() == before && !result.transfer.attempted);
    CHECK(result.status.is(error) && result.transfer.status.is(error));
    checkPreserved(out);
  };
  for (const auto invalid : {Model::UNSELECTED, static_cast<Model>(0xFF)}) {
    checkRefusal(configFor(bus, invalid, 0x48), 19, Err::INVALID_CONFIG);
  }
  for (const auto address : {0x47, 0x4C, 0x90}) {
    checkRefusal(configFor(bus, Model::TMP116, static_cast<std::uint8_t>(address)), 19, Err::INVALID_CONFIG);
  }
  checkRefusal(binding, 0, Err::OPERATION_TIMEOUT);
}

}  // namespace

int main() {
  selectedModelsAndIdBits();
  sharedBusNeighbors();
  failedReadsAndLocalRefusals();
  return EXIT_SUCCESS;
}
