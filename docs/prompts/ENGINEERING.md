# TMP11x engineering and state contracts

## Transport and ownership

The application owns I2C pins, clock rate, adapter init/recovery, mutex and
multiplexer route. The driver borrows `i2cWrite` and `i2cWriteRead` callbacks
with opaque user state. A pointer read is one atomic write+repeated-START+read
callback returning exactly two MSB-first bytes. No retries, shared-bus reset,
background tasks or pin changes. Timeout covers controller and lock delay.
Never retain temporary callback buffers; never invoke callbacks from ISR.

One application task serializes all stateful driver API access; for multiple
instances on a mux, establish target route before every physical callback.
The library does not save a mux channel or global I2C owner. The application
may poll different devices between a conversion trigger and completion.

## Lifecycle, health, trust

`bind` validates/caches only, no bus traffic. `begin` performs identity and
verified safe volatile configuration. `end` is bus-silent local close, while
physical shutdown is an explicit hardware operation. `OFFLINE` is a passive
health label, not an automatic retry gate. Raw reads/probe are intentionally
separate from tracked health; document their exact side effects.

Distinguish desired settings, observed settings, hardware trust, physical I2C
outcome and fresh measurement. A failed mutating transfer may have committed
fully or partly. Latch dirty state until verified reapplication after checking
EEPROM lock/busy. A successful transport callback does not imply config match.

## Conversion and freshness

TEMP value = signed16(raw)/128 °C; preserve exact negative values and signed
rounding. Raw `0x8000` after reset is an initial sentinel *until a completed
conversion*, not a global invalid numerical code. Average count and standby
cycle are independent settings. Establish earliest conversion readiness from
source-backed worst-case timing and/or the DATA_READY hardware evidence.

DATA_READY is cleared on CONFIG or TEMP read; configuration diagnostics can
consume freshness indication. A one-shot completion and a latest-value read
are different public contracts. Never claim a new continuous sample solely
because two temperatures differ or because a wall-clock period elapsed.

Alert high/low flags and temperature window/therm behavior are documented
mode-specifically; CONFIG reads acknowledge flags. ALERT pin state only
reflects application-owned GPIO observation and is not equivalent to a
latched driver event. Raw diagnostics must mark possible loss of status history.

## EEPROM safety and reset

REG 0x04 unlock/busy, 0x05–0x08 EEPROM/offset vary by model. EEPROM is factory
locked but external writers may leave EUN set. Verify safe locked/not-busy
state before a mutating write. If lock state cannot be established, refuse
mutation with an explicit error and never try to unlock/relock without an
owned maintenance transaction. Protect NIST/factory traceability bytes.

First-release driver supports readonly EEPROM, ID and optional per-device
software reset **only if exact documentation proves it is safe**. Never use
I2C general-call reset in initialization, recovery, CLI or HIL. Reset may
invalidate stored configuration and freshness; reset is not a substitute for
owner-permission or bus recovery. No EEPROM program API is implemented.

## Cooperative operations

Single operation slot per instance; admission is bus-silent. A `poll(nowMs,
maxTransfers)` turn invokes at most that many physical callbacks, and never
holds the bus during conversion waits. A zero budget performs deadlines/state
checks only. Caller supplies monotonic 32-bit milliseconds or an authoritative
clock hook. Use bounded wrap-safe time differences and reject excessive
relative deadlines (`>INT32_MAX`).

Result identity is retained until exactly-once `takeResult(token, out)`;
rejected admission does not overwrite the caller's token. Cancellation is
bus-silent but cannot undo a conversion or ambiguous write. If `poll` ends
after a late successful read, report whole-operation timeout; preserve the
physical transport result separately. All outputs on failure have a documented
unchanged-or-partial-value policy.
