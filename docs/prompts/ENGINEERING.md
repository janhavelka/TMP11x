# TMP11x engineering and state contracts

## Transport and ownership

The application owns I2C pins, clock rate, adapter init/recovery, mutex and
multiplexer route. The driver borrows `i2cWrite` and `i2cWriteRead` callbacks
with opaque user state. A pointer read is one atomic write+repeated-START+read
callback returning exactly two MSB-first bytes. No retries, shared-bus reset,
background tasks or pin changes. The core passes the clipped remaining timeout;
the adapter must enforce it across controller work and lock delay.
Never retain temporary callback buffers; never invoke callbacks from ISR.
With a Status-only callback, success promises the complete transfer; adapters
must detect short transfers and map them to errors. The core cannot detect a
short transfer falsely reported as success. An opaque user context may be null
when the callback does not need it; required callback pointers may not be null.

Selected devices specify 1–400 kHz I2C, no sensor SCL drive, and a 20–40 ms
hardware serial timeout (35 ms typical prose) under documented line-held/no-clock
conditions. This is separate from callback/operation deadlines and does not
limit total multi-transfer job duration. The application configures the bus;
the core cannot infer electrical compliance. Always write an explicit valid
pointer and read one word; no assumed auto-increment, pointer sharing or CRC.

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
verified safe volatile configuration. `end` is bus-silent local close retaining
the binding and desired settings; it clears initialization/startup evidence.
`unbind` also removes the binding. Neither establishes a power-on timestamp;
the next begin must establish fresh startup proof. Physical shutdown is an
explicit hardware operation. `OFFLINE` is a passive health label, not an
automatic retry gate. Raw reads/probe are intentionally
separate from tracked health; document their exact side effects.

`DriverState` describes tracked transport health independently of initialization.
UNINIT means no tracked outcome since bind/teardown; tracked successes/failures
drive health even before initialization. Invalidation clears device trust, not
transport history. `isOnline()` is bus-silent and means initialized plus
READY/DEGRADED; it is neither a live probe nor proof of freshness/config trust.
Stage 8 freezes this separately from the transport-health state;
this deliberately differs from TMP1x2's initialization-gated state changes.
Stage 18 extends bus-silent bind validation to the complete accepted profile.
Rejected binding preserves the old instance; accepted rebind discards old
device observations and starts uninitialized with unknown hardware trust.
Binding sets its new target directly; commit-on-first-write applies to later
settings/configure operations, not to replacing the binding.

Distinguish desired settings, observed settings, hardware trust, physical I2C
outcome and fresh measurement. A failed mutating transfer may have committed
fully or partly. Latch dirty state until verified reapplication after checking
EEPROM lock/busy. A successful transport callback does not imply config match.
The desired profile contains stable CONFIG fields, both limits and supported
opt-in managed offset. Offset management defaults off: TMP117/TMP119 offset is
observed but never replayed until explicitly owned; TMP116 has no offset access.
Explicit managed zero is a real setting, and requesting managed offset on
TMP116 rejects before I2C. Replay/readback must cover every owned field.
Separate register writes
are not atomic; describe intermediate comparator states and partial failures,
and do not promise glitch-free paired limits or rollback.

Dirty/uninitialized/invalidated profiles allow latest-value diagnostics with
untrusted provenance, but reject managed fresh acquisition before I2C.
Explicit initialization/recovery can re-establish trust subject to binding,
job ownership, deadline and safety gates. Before the one-shot's first CONFIG
write (including SD), compare all stable CONFIG fields, not only MOD;
a mismatch invalidates trust without CONFIG writes.
Changing limits in therm mode requires a non-inverted target pair. Limit writes
occur inside the shared guarded SD apply phase. Defensively keep intermediate
pairs ordered when the current pair is known; unknown/inverted current pairs
need full-profile verification before therm operation.
The response to TLOW > THIGH is undocumented, not a new hardware prohibition.

Stages 18/21 implement private resumable step engines shared with stages 29/30.
Each step uses at most one physical callback; waits have explicit due times.
Synchronous wrappers drive those states with a bounded clock loop. No second
apply/acquisition implementation or test-only production operation is needed.

## Conversion and freshness

TEMP value = signed16(raw)/128 °C; preserve exact negative values and signed
rounding. Raw `0x8000` after reset is an initial sentinel *until a completed
conversion*, not a global invalid numerical code. CONV selects the continuous
conversion cycle; AVG changes the active conversion duration, so their effect
on the actual cycle is coupled. One-shot duration depends on AVG, not CONV.
Retain minimum/typical/maximum timing with its conditions; label software
margins and polling choices as policy, not datasheet guarantees.
CONV/AVG cycle-table entries are nominal, with no specified continuous-cycle
maximum. TMP117's 17.5 ms maximum is explicitly conditioned on one-shot mode;
extrapolating it to continuous cadence is policy. Stage 6 defines finite
continuous timeout margins without advertising them as silicon guarantees.
For precision, by default schedule the first owned-one-shot readiness poll at
AVG count times the applicable single-conversion maximum, with rounding
up to the clock resolution. Earlier sparse polling is an explicit latency
tradeoff, especially on TMP119 (SNIS236 p.23 recommends avoiding active I2C).
Bounded continuous-fresh helpers/jobs default to one CONFIG-ready poll per
nominal CONV×AVG cycle, rounded up to clock resolution, measured from the
baseline or previous poll. A due time beyond the deadline expires without
another poll. This is pacing policy, not a continuous-cycle maximum or a way
to avoid every active conversion. Untimed try-fresh performs one bounded
attempt; its caller owns repeated-call pacing. These policies limit only this
driver's own polling, not other devices or masters on the shared bus.
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
The API policy requires desired shutdown before admission. A continuous desired
mode rejects locally; the caller explicitly selects shutdown. Each managed
one-shot first guards 0x04 and reads CONFIG to compare owned stable fields
before any CONFIG write. A mismatch latches dirty and refuses the trigger;
the SD write must not silently repair drift before checking it. An observed
MOD=11 is compatible only with retained known owned-shot provenance, never an
unknown outside trigger. The operation then uses an explicit shutdown write to abort residual work,
reads CONFIG to establish the old-readiness baseline, and triggers exactly once.
Desired mode stays shutdown. MOD readback during/after a one-shot is undocumented;
it proves neither completion nor abort. From the terminal physically successful
trigger through in-flight/completed/cancelled/timed-out states, MOD=01/11 is
compatible by owned-trigger provenance until successful explicit SD or
invalidation/rebind. A physical trigger success after the deadline retains
provenance, not timely success. An ambiguous trigger/SD write invalidates trust;
it cannot establish clean ownership or verified shutdown. Cancellation/timeout
is bus-silent and never restores or retriggers.
The next explicitly requested one-shot establishes its own shutdown boundary.

All managed profile changes, including limit-only and routing/polarity-only
changes while continuous, use guarded shutdown, apply/readback, a cleared
readiness baseline, then the desired continuous mode and a final CONFIG
readback comparing every stable field. Dirty remains set until that final
verification succeeds; its returned flags feed the evidence record and its
readiness consumption establishes the new acquisition baseline. This avoids guessing how
a mid-average write affects the current block. Bind/outside access invalidates
the baseline; a continuous try-fresh call never silently writes or establishes
profile trust. If no baseline exists under an otherwise trusted generation,
the first read only establishes it. Freshness requires that baseline and new
DATA_READY evidence. Discarding one completion is not a proven alternative.
If readiness is never observed, return a bounded not-ready/timeout outcome;
elapsed time or an unverified GPIO level is not fresh-conversion evidence.

Averaging is accumulated in nonoverlapping blocks, not a moving average.
The pinned datasheets do not explicitly define same-value CONFIG restart,
mid-average setting changes, shutdown retention of DATA_READY/alert-mode flags,
MOD readback during/after one-shot, retrigger behavior or TEMP latching across
a simultaneous two-byte read/update. Preserve these unknowns and distinguish
conservative invalidation/acquisition policy from silicon claims. Combined
transfers do not prove an undocumented word latch; a stable owned one-shot
result has different evidence from a continuously updating register.

Alert high/low flags and temperature window/therm behavior are documented
mode-specifically: CONFIG reads clear latched alert-mode HIGH/LOW flags;
therm-mode HIGH follows the high-limit/hysteresis conditions and LOW is zero.
TEMP reads clear DATA_READY, not comparator flags. ALERT pin state only
reflects application-owned GPIO observation and is not equivalent to a
latched driver event. Raw diagnostics must mark possible loss of status history.
Every successful CONFIG read, including diagnostics, readback and a late or
subsequently failed operation, contributes returned alert-mode HIGH/LOW to a
sticky owner-visible record. A later zero read cannot erase it. Stage 19 owns
bookkeeping and stage 22 exposes bus-silent inspect/consume; include source,
profile generation and uncertainty when a failed read may have consumed flags.
If sticky events span generations/modes, use a bounded mixed/unknown provenance
marker; do not attribute the aggregate to the most recently observed profile.
Therm-mode HIGH is a level snapshot, not an OR-latched software event. While
T/nA remains therm and no reset occurs, its documented set/clear condition
is a completed comparison; shutdown/abort alone is not a documented clear.
Routing and polarity affect the pin, not that flag's retention. Pin persistence
through shutdown with routing/polarity unchanged is an inference from these
rules, not an independent explicit retention specification.
TMP116 does not explicitly say comparisons wait for the full averaging block;
keep that inference separate from TMP117/TMP119's explicit wording.
An alert-mode flag remains latched when temperature returns in range; after
CONFIG acknowledgement the next out-of-range conversion can reassert it.
Active-high ALERT still uses an open-drain pin. Offset affects the reported
TEMP result with saturation, but the exact offset/comparator ordering requires
further evidence; do not certify it through a self-authored simulator.

## EEPROM safety and reset

REG 0x04 unlock/busy, 0x05–0x08 EEPROM/offset vary by model. EUN powers up
locked (0), but external writers may leave it set. EUN is bit 15 of register
0x04; busy bit 14 mirrors CONFIG bit 12. Establish the
startup/reset timing policy as well as locked/not-busy state before mutation.
TMP116 SBOS740A p.17 says busy is cleared during reset load, contradicting p.26
and p.29 tables: busy==0 alone must not prove completed startup. The roughly
1.5 ms POR interval is not a specified maximum. Record assumptions and require
bounded readback; preserve this source conflict rather than guessing hardware.
FIRST_ACK is the only startup-origin policy: when proof is absent, use completion
of the first successful compatible read in the current initialize/recover
operation. Store that timestamp internally using the authoritative clock hook;
it is not a Config field or an API/CLI input. An old bind/operation timestamp
cannot substitute. Startup reads are permitted (TMP116 p.17; TMP117 p.18;
TMP119 p.20), so this is a conservative power-present anchor, not completed POR.
The owner must ensure continuously stable power/no outside reset from that
read through every operation relying on the retained proof. Unknown power
stability refuses mutation. The wait remains policy: TMP117 tRESET=1.5 ms
(p.7) is typical, without a maximum.

Retain established startup proof until invalidated. Bus-silent
`invalidateDeviceState()` clears its internal origin, initialized/config/
freshness/owned-trigger knowledge while retaining desired settings, transport
history and historical sample/alert provenance. It requires idle/no retained
result; rejection preserves state. After power loss stop using old trust,
cancel/take any result, invalidate, restore stable power and explicitly recover.
Invalidation may occur before power restoration because it supplies no timestamp.
Accepted bind, end/unbind and any driver-issued or reported reset clear old
origin/proof. Stage 25 clears it before the reset attempt, honors the separate
2-ms reset wait and then establishes a new post-reset FIRST_ACK origin/proof
before later writes. Stage 19 owns public invalidation; before it exists,
stage 18 tests use fresh bind/private state seams, not a placeholder API.

Acquire an operation-local 0x04 guard at the start of each mutating operation
(apply/recover, limits, offset, shutdown, one-shot, reset) or freshness-producing
acquisition, before its first write/freshness observation under exclusive
ownership. EUN=1 refuses immediately. EUN=0/busy=1 permits paced successful
0x04 re-reads only, bounded by deadline and work cap; no retries after transfer
failure and no writes before observing EUN=0/busy=0. Use neither destructive
CONFIG busy nor a previous operation's lock check as a substitute. Reuse the safe word only
within that uninterrupted operation; invalidate on reset, rebind, outside
access, ambiguous transfer failure or observed busy. After reset, recheck
0x04 before further writes. Latest/diagnostic reads need not poll 0x04 and
never claim fresh acquisition. EUN=1 permits no writes, including shutdown.
An untimed/nonblocking acquisition returns busy/not-ready instead of entering
a timed busy-wait; only a deadline-bearing path may repeat guard observations.
The library cannot stop externally unlocked continuous conversions safely.
This release has no unlock/relock/maintenance path. Recovery from EUN=1 needs
an application power cycle or authorized external relock, then invalidation
and verified recovery; addressed reset is not a relock path.

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
The addressed reset word preserves verified stable settings with MOD=00/01,
zeros status/reserved bits and sets bit 1; this is conservative library policy,
not a documented encoding of all reset side effects. EEPROM reload, EUN effect,
other-bit application and bus availability during soft reset remain unknown.
Wait at least the documented 2 ms, then require safe 0x04 and full readback;
do not infer configuration, relock or completion solely from the elapsed time.

## Cooperative operations

Single operation slot per instance; admission is bus-silent. A `poll(nowMs,
maxTransfers)` turn invokes at most that many physical callbacks, and never
holds the bus during conversion waits. A zero budget performs deadlines/state
checks only. Poll returns pending during waits; it does not sleep or invoke a
yield loop. The clock hook is authoritative for deadlines and ages; retained
`nowMs` arguments do not override it. Stage 9 lists required-clock APIs and
rejects a missing required hook before I2C. All stored timestamps use its epoch.
Enforcing an absolute deadline across callbacks requires an authoritative
clock hook re-read after each callback; entry-only `nowMs` cannot detect a
late successful callback. Use bounded wrap-safe time differences and reject excessive
relative deadlines (`>INT32_MAX`).
Clock gaps of a full 32-bit wrap cannot be detected; continuity and maximum
service gap are owner obligations, not an implementable overflow detector.

An active job or unconsumed terminal result rejects every unrelated I2C method
with BUSY and zero callbacks. `poll`, cancel/result consumption and passive
getters remain available; explicit lifecycle transitions obey stage 28's rules.
Only TEMP/CONFIG read effects consume DATA_READY, but the all-I2C exclusion
keeps ownership uniform. Uncoordinated outside reads still invalidate evidence.

Result identity is retained until exactly-once `takeResult(token, out)`;
rejected admission does not overwrite the caller's token. Cancellation is
bus-silent but cannot undo a conversion or ambiguous write. If `poll` ends
after a late successful read, report whole-operation timeout; preserve the
physical transport result separately. All outputs on failure have a documented
unchanged-or-partial-value policy.
Late successful reads still update observed values and retained alert evidence;
they do not clear dirty, set initialized, publish a successful sample or replace
the last-good cache. Late writes retain possible hardware effects and dirty
state. Logical timeout does not rewrite the physical health outcome.
