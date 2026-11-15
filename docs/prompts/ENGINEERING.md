# TMP11x engineering and state contracts

## Transport and ownership

The application owns I2C pins, clock rate, adapter init/recovery, mutex and
multiplexer route. The driver borrows `i2cWrite` and `i2cWriteRead` callbacks
with opaque user state. A pointer read is one atomic write+repeated-START+read
callback returning exactly two MSB-first bytes. No retries, shared-bus reset,
background tasks or pin changes. Timeout covers controller and lock delay.
Never retain temporary callback buffers; never invoke callbacks from ISR.
With a Status-only callback, success promises the complete transfer; adapters
must detect short transfers and map them to errors. The core cannot detect a
short transfer falsely reported as success. An opaque user context may be null
when the callback does not need it; required callback pointers may not be null.

One application task serializes all stateful driver API access; for multiple
instances on a mux, establish target route before every physical callback.
The library does not save a mux channel or global I2C owner. The application
may poll different devices between a conversion trigger and completion.
The owner also excludes other readers/writers of the same sensor while an
operation depends on its flags, selected configuration or EEPROM lock state.
Per-transfer bus locking alone does not protect those multi-transfer assumptions.
If outside access/reset can occur, require explicit invalidation and verified
recovery; do not claim protection against an uncoordinated second bus master.

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
conversion*, not a global invalid numerical code. CONV selects the continuous
conversion cycle; AVG changes the active conversion duration, so their effect
on the actual cycle is coupled. One-shot duration depends on AVG, not CONV.
Use minimum/typical timing for polling schedules and applicable maximum timing
for bounds; label any software margin as policy, not a datasheet guarantee.
Elapsed time alone is not evidence that this instance completed a conversion.

DATA_READY is cleared on CONFIG or TEMP read; configuration diagnostics can
consume freshness indication. A one-shot completion and a latest-value read
are different public contracts. Never claim a new continuous sample solely
because two temperatures differ or because a wall-clock period elapsed.
Before an owned one-shot, establish shutdown and a known old-readiness baseline;
tie fresh evidence to the new trigger. A cached result keeps its acquisition
provenance but is not a newly acquired sample on every read. A timestamp taken
after TEMP transfer is an observation time, not the exact conversion instant.
DATA_READY is one bit, not a conversion counter: missed updates cannot be counted
from that bit or repeated temperatures.

Alert high/low flags and temperature window/therm behavior are documented
mode-specifically: CONFIG reads clear latched alert-mode HIGH/LOW flags;
therm-mode HIGH follows the high-limit/hysteresis conditions and LOW is zero.
TEMP reads clear DATA_READY, not comparator flags. ALERT pin state only
reflects application-owned GPIO observation and is not equivalent to a
latched driver event. Raw diagnostics must mark possible loss of status history.

## EEPROM safety and reset

REG 0x04 unlock/busy, 0x05–0x08 EEPROM/offset vary by model. EEPROM is factory
locked but external writers may leave EUN set. EUN is bit 15 of register 0x04;
its busy flag is bit 14, distinct from CONFIG's busy bit 12. Establish the
startup/reset timing policy as well as locked/not-busy state before mutation.
TMP116 SBOS740A p.17 says busy is cleared during reset load, contradicting p.26
and p.29 tables: busy==0 alone must not prove completed startup. The roughly
1.5 ms POR interval is not a specified maximum. Record assumptions and require
bounded readback; preserve this source conflict rather than guessing hardware.
If safe state cannot be established, refuse mutation. This release exposes no
unlock, relock or maintenance write path. Do not trigger conversions with EUN
set or claim that a latest register read is a valid new conversion in that state.

First-release driver supports readonly EEPROM, ID and optional per-device
software reset for TMP117/TMP119 (CONFIG bit 1; documented 2 ms reset interval).
TMP116's bits 1:0 are reserved: reject per-device reset without I2C. Ordinary
CONFIG writes force reset to zero and canonicalize legal MOD=10 continuous
encoding to MOD=00 when encoding settings; readback comparison accepts the
documented normalization. Never use
I2C general-call reset in initialization, recovery, CLI or HIL. Reset may
invalidate stored configuration and freshness; reset is not a substitute for
owner-permission or bus recovery. Guard reset itself against EUN/busy and avoid
interpreting its always-zero command bit as completion. No EEPROM program API
is implemented. EEPROM address reads observe volatile shadow registers, not a
separate physical NVM read; a previous writer may have changed those shadows.
Never label their contents immutable or prove factory traceability from them.

## Cooperative operations

Single operation slot per instance; admission is bus-silent. A `poll(nowMs,
maxTransfers)` turn invokes at most that many physical callbacks, and never
holds the bus during conversion waits. A zero budget performs deadlines/state
checks only. Poll returns pending during waits; it does not sleep or invoke a
yield loop. The admission/start clock and poll clock must share one epoch.
Enforcing an absolute deadline across callbacks requires an authoritative
clock hook re-read after each callback; entry-only `nowMs` cannot detect a
late successful callback. Use bounded wrap-safe time differences and reject excessive
relative deadlines (`>INT32_MAX`).

Result identity is retained until exactly-once `takeResult(token, out)`;
rejected admission does not overwrite the caller's token. Cancellation is
bus-silent but cannot undo a conversion or ambiguous write. If `poll` ends
after a late successful read, report whole-operation timeout; preserve the
physical transport result separately. All outputs on failure have a documented
unchanged-or-partial-value policy.
