#include "TMP11x/Config.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

using namespace TMP11x;

static_assert(std::is_same_v<I2cWriteFn,
                            Status (*)(std::uint8_t, const std::uint8_t*, std::size_t,
                                       std::uint32_t, void*)>);
static_assert(std::is_same_v<I2cWriteReadFn,
                            Status (*)(std::uint8_t, const std::uint8_t*, std::size_t,
                                       std::uint8_t*, std::size_t, std::uint32_t, void*)>);
static_assert(std::is_same_v<NowMsFn, std::uint32_t (*)(void*)>);
static_assert(std::is_same_v<GpioReadFn, Status (*)(bool&, void*)>);
static_assert(std::is_same_v<decltype(Config::offlineThreshold), std::uint8_t>);
static_assert(std::is_same_v<decltype(Limits::lowCounts), std::int16_t>);
static_assert(std::is_same_v<decltype(Limits::highCounts), std::int16_t>);
static_assert(std::is_trivially_copyable_v<Config>);
static_assert(std::is_trivially_copyable_v<SensorSettings>);
static_assert(std::is_trivially_copyable_v<Capabilities>);
