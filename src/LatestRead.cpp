#include "LatestRead.h"

#include "DeviceEvidence.h"
#include "TMP11x/CommandTable.h"
#include "TMP11x/Temperature.h"

namespace TMP11x::detail {

LatestRead::LatestRead(TMP11x& device) noexcept
    : trust(device._live.trust), generation(device._live.profileGeneration),
      cache(device._lastSample), _device(device) {}

LatestReadResult LatestRead::read(Sample& out, std::optional<ReadBudget> budget, ReadKind kind) const {
  const auto transport = RegisterTransport::tracked(_device);
  const auto& binding = transport.binding;
  auto remaining = binding.i2cTimeoutMs;
  std::uint32_t before = 0;
  if (budget) {
    if (binding.nowMs == nullptr) {
      const auto status = Status::Error(Err::CLOCK_REQUIRED, "Timed read requires a clock");
      return {status, {status, false}, std::nullopt};
    }
    before = binding.nowMs(binding.timeUser);
    const std::uint32_t elapsed = before - budget->previousMs;
    remaining = elapsed < budget->remainingMs ? budget->remainingMs - elapsed : 0;
  }

  std::uint16_t raw = 0;
  const auto transfer = readWord(transport, command::temperature, raw, remaining);
  DeviceEvidence{_device}.observeTemperature(transfer);
  LatestReadResult result{transfer.status, transfer, std::nullopt};
  if (transfer.status.ok()) result.raw = raw;
  if (budget && transfer.attempted) {
    const std::uint32_t callbackElapsed = *transfer.completedMs - before;
    if (callbackElapsed >= remaining) {
      result.status = Status::Error(Err::OPERATION_TIMEOUT, "TEMP read reached the operation deadline");
    }
  }
  if (!result.status.ok()) return result;

  Sample sample{};
  sample.acquisitionKind = kind;
  sample.freshConversion = kind != ReadKind::LATEST;
  sample.raw = raw;
  sample.counts = temperature::decodeCounts(raw);
  sample.celsius = temperature::toCelsius(sample.counts);
  sample.timestampValid = transfer.completedMs.has_value();
  sample.observedAtMs = transfer.completedMs.value_or(0);
  sample.configTrust = trust;
  sample.profileGeneration = generation;
  if (command::accessFor(binding.model, command::offset) != command::Access::GUARDED_OFFSET) {
    sample.offset.state = OffsetState::UNAVAILABLE;
  } else if (sample.freshConversion && _device._live.offset) {
    const auto state = binding.settings.managedOffsetCounts ? OffsetState::VERIFIED_MANAGED
                                                           : OffsetState::OBSERVED_UNMANAGED;
    sample.offset = {state, _device._live.offset->counts, true};
  }
  cache = sample;
  out = sample;
  return result;
}

}  // namespace TMP11x::detail
