#include "TMP11x/Status.h"

#include <cstdint>
#include <type_traits>

using namespace TMP11x;

static_assert(std::is_same_v<std::underlying_type_t<Err>, std::uint8_t>);
static_assert(std::is_same_v<decltype(Status::detail), std::int32_t>);
static_assert(std::is_same_v<decltype(Status::msg), const char*>);
static_assert(std::is_trivially_copyable_v<Status>);
static_assert(Status::Ok().ok());
static_assert(Status::Error(Err::IN_PROGRESS, "waiting").inProgress());
static_assert(!Status::Error(Err::I2C_NACK, "NACK").ok());
static_assert(DriverState::UNINIT != DriverState::READY);
static_assert(DriverState::READY != DriverState::DEGRADED);
static_assert(DriverState::DEGRADED != DriverState::OFFLINE);
