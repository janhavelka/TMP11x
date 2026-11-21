# Cooperative owner operations

One application owner drives one TMP11x instance per physical sensor. A start
validates a request and returns its token. Only `poll` advances its register
steps. A terminal result occupies the same slot until the owner takes it.
The synchronous APIs use the same apply and acquisition steps. The public
types, callback signatures and device gates are defined in the
[API contract](api-contracts.md) and the [register ownership map](architecture.md).

## Admission

The common trailing arguments to each start are `uint32_t nowMs, uint32_t
timeoutMs, OperationToken& token`. Start and poll ignore the supplied
`nowMs` for **all** timing decisions and timestamps. On success start
returns `IN_PROGRESS`, copies the request into the slot and assigns the
token. Borrowed callback contexts must outlive the binding.

| Start, with common arguments omitted | Required local state | Work permitted in subsequent polls |
| --- | --- | --- |
| `startInitialize(…)` | Bound, including a previously failed initialization. | Check ID, establish missing startup proof, guard, apply and verify the bound target. |
| `startRecover(…)` | Bound, including dirty, offline or invalidated. | Check ID, renew missing proof and replay the entire owned target. |
| `startConfigure(const Config& candidate, …)` | Initialized, clean and verified. Identical binding, valid candidate settings and capabilities. | Check ID and guard, then apply and verify the copied candidate. |
| `startRead(ReadKind::LATEST, …)` | Initialized in either desired mode. Dirty settings are permitted. | One TEMP read with observed trust and `freshConversion=false`. |
| `startRead(ReadKind::CONTINUOUS_FRESH, …)` | Initialized, verified clean, startup proof, desired continuous. | Guard, obtain eligible current-generation readiness, read TEMP. |
| `startRead(ReadKind::ONE_SHOT, …)` | Initialized, verified clean, startup proof, desired shutdown. | Guard, check stable fields, establish the SD boundary and trigger once. |

First check slot ownership. ACTIVE and RESULT_PENDING return `BUSY` even
for a malformed competing request. On a free slot, require a binding
(`NOT_BOUND`), a connected operation kind (`UNSUPPORTED` for an unavailable
engine), valid arguments and a usable deadline. Unknown read selectors and
invalid explicit budgets return `INVALID_PARAM`. Missing required clock
returns `CLOCK_REQUIRED`. Validate initialization (`NOT_INITIALIZED`),
managed trust (`CONFIG_DIRTY` for a dirty target), desired mode
(`INVALID_CONFIG` for the wrong mode) and the selected model's capabilities
(`UNSUPPORTED`). A known EEPROM-unsafe target refuses guarded work with
`EEPROM_UNSAFE`. Power stability and exclusion of outside access are owner
preconditions for using retained proof.

Compare configure's model, seven-bit address, both I²C callbacks and context,
both timeout fields, clock function/context/resolution, GPIO function/context
and offline threshold **by field**. The owner keeps each context bound to
the same physical target, route policy and clock epoch. A changed binding or invalid profile
returns `INVALID_CONFIG`. A managed offset on TMP116/N, even zero, returns
`UNSUPPORTED`. Initialize and recover have no binding argument. Commit the
slot and token only after all checks pass. Every start, including rejection,
uses zero sensor callbacks. Rejection leaves the output token, serial counter,
binding, desired profile and slot intact.

## Slot and result lifetime

| Method | FREE | ACTIVE | RESULT_PENDING |
| --- | --- | --- | --- |
| `poll(uint32_t nowMs, uint8_t maxTransfers)` | `RESULT_NOT_AVAILABLE` | `IN_PROGRESS` while waiting or more work remains. On completion, retain and return the terminal job status. | Return the saved job status, zero transfers. |
| `cancel()` | `RESULT_NOT_AVAILABLE` | Retain and return `CANCELLED`, zero transfers. | `BUSY`, preserve the result. |
| `takeResult(token, out)` | `RESULT_NOT_AVAILABLE` | `RESULT_NOT_AVAILABLE` | Wrong token gives `TOKEN_MISMATCH`. On a match, copy the result, free the slot and return `OK`. |

Only the owner calls these methods, between callbacks. Cancel acts on an
ACTIVE slot as found, even if the owner stopped polling. It requires no clock
read or bus action. If a poll already observed timeout, the timeout record
owns the slot. A failed take leaves `out` untouched. A matching take copies
the token, kind/read kind, terminal job status, last physical status/validity,
callback count, possible writes and read effects, and any timely successful
sample. A cancelled job with no callbacks has count zero and no physical
status. Taking a failed job still returns `OK`. The result's read effects
come from this job's callbacks. Taking alert events from the instance while
the job is active or pending preserves that job's evidence. Taking the
result also leaves the instance's events available to their own consumer.

An `OperationToken` identifies the issuing live instance and a nonzero
32-bit admission serial. Compare both for a retained result. Two sensors
can each issue serial 1. A scheduler keeps the issuing instance and token
together. The device class has a stable address and disallows copy and
move construction and assignment. This keeps one binding, slot and serial
history per issuer. Tokens and results can be copied as values. The
serial starts at 1, advances only on acceptance and survives end, unbind
and rebind. After issuing `UINT32_MAX`, the next otherwise valid start
returns `WORK_LIMIT` (token space exhausted). Never issue zero or recycle a
serial. Destroying the instance ends the token's lifetime. A serialized
serial alone cannot identify a sensor across instances or owner sessions.

While ACTIVE or RESULT_PENDING, all unrelated I²C calls, including ID,
probe, TEMP, CONFIG, EEPROM, dumps and reset, return `BUSY` before I/O.
The same gate covers every start, bind, begin, end, unbind and
`invalidateDeviceState()`. Local snapshots and explicit alert consumption
remain available. GPIO observation uses its separately bound callback.
Serialize these calls too. After loss of power or outside access, stop
using live evidence, cancel if active, take the result, invalidate and
recover explicitly under restored power and exclusive ownership.

## Clock and work by API

| API | Clock requirement | Maximum sensor callbacks or scheduling rule |
| --- | --- | --- |
| Bind, end, unbind, invalidate, capability/codec/timing queries, settings/health/online/last-sample and alert getters, take alert events, cancel, take result | None | Zero. GPIO level needs its own hook and uses zero I²C. |
| `getLastSampleAgeMs` | Same authoritative epoch and valid TEMP completion timestamp | Zero. Missing sample or unknown age returns `MEASUREMENT_NOT_READY`; missing hook with a cached sample returns `CLOCK_REQUIRED`. Preserve the output on failure. |
| Probe, identity, read one mapped register or one EEPROM shadow | Optional for observation time | One neutral combined read. |
| Register snapshot, EEPROM snapshot | Optional | ID first. At most 10 reads for a full dump, 6 or 5 for lock plus four or three shadows including ID. Stop at first failure. |
| Managed readConfiguration, readLimits, readTemperatureOffset, verifyConfiguration | Optional | Respect initialization and model gates. At most 1, 2, 1 and 3–4 tracked reads respectively. |
| `readLatest`, `tryReadFresh` | Optional | One tracked TEMP for latest. Untimed continuous try uses at most three tracked callbacks and never waits. |
| Both begin overloads, configure/setters, recover, supported reset | Required | `operationTimeoutMs` bounds the entire synchronous operation. Check a replacement binding's clock before installation. |
| `readFreshBlocking`, `readOneShotBlocking`, all four starts and active poll, including a LATEST job | Required | Explicit whole-operation timeout, including time between owner turns. |

With no hook, clock-optional observations carry invalid timestamps. Managed
initialization itself needs a clock. Tick zero is valid with a valid stamp.
Age is the unsigned elapsed time since the host observed TEMP. The owner
must establish continuous time and an unambiguous difference between
the acquisition and current floor readings as defined below. A clock
restart or change of epoch requires rebind.
An owner needing older ages tracks them in a wider timebase. A cache read
preserves the acquisition time, generation, offset provenance and freshness
of the original sample.

### Deadline arithmetic

The hook reports monotonic milliseconds rounded **down** to a tick of
`q = clockResolutionMs` before truncation to 32 bits. A job starts with
the hook reading at admission. For `1 <= timeoutMs <= INT32_MAX` and
`1 <= q <= INT32_MAX`, set the budget to `(timeoutMs / q) * q` using
integer division. Reject zero, excessive or sub-tick budgets before
commitment. Every elapsed interval, including time parked by the owner,
uses unsigned subtraction. One implementation keeps the previous hook
reading and subtracts each unsigned delta from the remaining budget,
saturating at zero. This works across wrap even when a long final service
gap takes the job more than one wrap from admission. Check expiry at equality.

To measure elapsed time by subtraction, the owner must know that the two
**unwrapped floor readings** differ by less than `2^32` ms. A sufficient
bound on the actual interval is `2^32 - q` ms, including callback time.
Rounding the earlier reading down can add almost one tick to the measured
interval. For example, with q=2, physical times 1 and 4294967296 ms both
report zero. Their physical gap is less than a wrap, yet a 10 ms job has
long expired. The two low words cannot detect this case. If service
continuity is lost, cancel the active job and take the result before
resuming timed work. A direct sample-age calculation needs the bound from
acquisition to query, even when intervening polls met it individually.

For example, the following C++17 checks exercise the callback and wait
arithmetic at wrap and at the largest supported tick. The wait helper
accepts whole milliseconds already rounded up from the specified interval.

```cpp
#include <cstdint>

constexpr std::uint32_t remaining(std::uint32_t budget, std::uint32_t previous,
                                  std::uint32_t current) {
    const std::uint32_t elapsed = current - previous;
    return elapsed < budget ? budget - elapsed : 0;
}

constexpr std::uint64_t minimumWait(std::uint32_t ms, std::uint32_t tick) {
    const std::uint64_t q = tick;  // validated, positive resolution
    return q + ((ms + q - 1) / q) * q;
}

static_assert(remaining(12, 0xfffffff8u, 1) == 3);
static_assert(remaining(3, 1, 4) == 0);
static_assert(remaining(3, 1, 5) == 0);
static_assert(remaining(3, 1, 1) == 3);
static_assert(remaining(10, 0xfffffffeu, 0) == 8);
static_assert(minimumWait(3, 10) == 20);
static_assert(minimumWait(1, 2147483647) == 4294967294ULL);
```

Each timed callback, including the last, follows this order.

1. Sample the hook and update remaining time. If expired, finish with
   `OPERATION_TIMEOUT` and zero further transfers. Check the phase's due
   time and work allowances before starting a callback.
2. Pass `min(i2cTimeoutMs, remainingMs)` to exactly one callback. Commit
   any first-write intent immediately before invoking it.
3. Resample on return. Account for the terminal physical status, complete
   observed words, possible mutation and destructive reads even on expiry.
   If expired, finish with `OPERATION_TIMEOUT`, including when the physical
   status is an error. The adapter can return after the logical deadline
   despite clipping, for example when the pre-call tick has unknown phase.
   A timely physical error ends the operation with that error. Sample
   time again before a deferred terminal success/publication.
   Expiry suppresses publication and further I²C.

For a minimum wait of `d` ms after a callback with floor reading `t`, the
earliest eligible elapsed interval from `t` is `q + ceil(d/q) * q`. The
extra tick covers the unknown sub-tick completion phase. Some stage 6
vectors give an exactly tick-aligned completion as a fixture input. A
public hook reading alone supplies no such alignment. Round fractional
conversion durations up without first truncating to milliseconds. Calculate
in a wider type and compare the interval with the remaining time at that
origin before narrowing. If the next required callback is due at or beyond
expiry, finish with `OPERATION_TIMEOUT` without issuing it. Store a bounded
relative wait and decrease it by elapsed time. For a continuous baseline
predating admission in the same uninterrupted epoch, the unsigned
difference is a lower bound on elapsed floor time even across multiple
wraps. Credit at most that difference toward the minimum wait. An aliased
short difference can delay the poll. A missing valid timestamp earns zero
credit. Allow one due poll and pace again from its actual completion.

### Work limits

These numbers are software policy for the shared step engines. One phase
dispatch performs bounded local work and invokes zero or one sensor
callback. Recording its physical outcome and deciding terminal success or
failure are part of the same dispatch.

| Limit | When the allowance runs out |
| --- | --- |
| Per `poll`, at most 32 phase dispatches and at most `maxTransfers` sensor callbacks. | End the turn with `IN_PROGRESS` if more work remains. At budget zero check state and deadline only. A parked wait returns immediately. |
| Per timed operation, at most 256 phase dispatches across all turns, including each guard reread and each readiness poll. | The last permitted dispatch can finish normally. If a further dispatch is required, finish with `WORK_LIMIT` before issuing it. A new turn cannot renew this allowance. |
| During a wait, at most 1,000,000 unsuccessful positive-budget checks of the same hook tick without phase progress. Applies to blocking loops and cooperative service. | A further such check returns terminal `WORK_LIMIT`. A changed tick or actual phase progress resets this counter. A zero-budget turn never spins or charges a wait check. |

Deadline expiry takes precedence when a work check also finds no time left.
If dispatch 32 of a turn is dispatch 256 of the job, finish on that turn.
Return the step's terminal status, or `WORK_LIMIT` if it needs another step.
Waiting probes spend no dispatch or transfer allowance. Blocking wrappers
drive the same bounded phases without sleeping or calling a yield hook.
Even with a frozen clock, a blocking call terminates. With an advancing
clock the fixed deadline bounds the number of distinct ticks as well.
Repeated equal readings are normal. `WORK_LIMIT` reports resource exhaustion;
it supplies no diagnosis of a broken clock. An owner withholding all transfer
budget on a frozen clock can inspect or cancel its parked job.

## Register steps, observations and publication

**Initialize, configure and recover.** Start from the bound target or copied
candidate. Begin/recover with missing startup proof use the completion of the
current operation's first successful compatible ID read as FIRST_ACK and
wait the 3 ms policy interval.
Discard an unfinished origin when the operation ends. On every mutation or
fresh acquisition, obtain a current-operation `0x04` guard. EUN set stops
guarded work immediately, including with BUSY set. Keep the unsafe latch
until the owner's corrective action, invalidation and renewed proof. With
EUN clear and BUSY set, timed work waits at least 1 ms from that read's
completion (floor-clock rule) before rereading `0x04`. A CONFIG BUSY
observation also cancels the guard. Retain its flags, check drift, then
pace a guard renewal before dependent work. A transfer failure ends the
attempt. Guard and FIRST_ACK have different lifetimes. [Safety and startup policy](eeprom-and-reset-policy.md#startup-and-operation-guard).

Immediately before configure's first mutating callback, including guarded
SD, retain the candidate as desired and mark the profile dirty. Before
that point cancellation preserves the previous target and all actual read
effects. Discard prior conversion evidence when mutation begins. Apply and
verify stable CONFIG, both ordered limits and supported managed offset in
SD. Observe supported unmanaged offset. TMP116/N uses
`0x07` for EEPROM3. Consume the SD baseline, write the desired mode and
compare all final stable fields. If the final CONFIG shows BUSY, renew the
guard and repeat verification within the remaining budget. Only timely
verification of the entire target establishes a verified generation and
initialization. That final matching CONFIG establishes the generation's
baseline. Retain its alerts and consume its DATA_READY without crediting
it to a later fresh read. A partial or ambiguous write retains the target,
affected fields and dirty evidence. Recovery requires an explicit new operation.
An already initialized instance can retain latest access with dirty trust.
A failed first initialization cannot grant it. [Apply order and identity exceptions](api-contracts.md#startup-apply-and-reset).

**Continuous fresh.** An untimed try stops after one BUSY guard read, or
after safe guard plus CONFIG with BUSY, with `MEASUREMENT_NOT_READY` and
no TEMP. Stable drift in CONFIG returns `CONFIG_MISMATCH` and latches
dirty even with BUSY or DATA_READY. A missing baseline in an otherwise
verified generation consumes the first observation without publication.
Require one unconsumed eligible ready observation for that generation,
then one timely TEMP. An ambiguous CONFIG or TEMP read discards any saved
ready evidence. The next fresh attempt needs a new eligible observation.
The timed variant paces readiness checks from the baseline or preceding
poll at the nominal CONV × AVG table period. A
latest read requires no guard and has no fresh-conversion claim.

**Owned one-shot.** Under the guard compare every stable CONFIG field
before writing anything. Only an earlier physically successful owned
trigger permits MOD=11 here. Write guarded SD, read matching SD CONFIG
to consume stale readiness, then issue exactly one MOD=11 write. Desired
mode remains SD. From physical trigger completion wait the AVG count
times the model's single-conversion maximum before polling CONFIG. After
a ready-clear poll wait at least 5 ms from its completion. Compare every
stable field at the SD baseline and at every readiness poll, applying the
owned MOD exception only while it is valid. On drift, record the returned
flags, latch dirty and stop before the next transfer. A timely mismatch
returns `CONFIG_MISMATCH`. Require new DATA_READY for this trigger, then
read TEMP. Apply the floor-clock rule to each wait.
Use the [conversion windows and literal vectors](device-behavior.md#polling-and-deadlines)
when sizing a budget. A due tick, GPIO level, MOD field or cached
temperature alone supplies no completion evidence.

A successful trigger, even if late, permits observed MOD=01 or 11 with
matching remaining stable fields, through completion, cancellation and
timeout. Retain that provenance until a successful explicit SD write or
invalidation of device knowledge. A failed ambiguous SD/trigger leaves
the effect uncertain and trust dirty. Neither a failed write nor a later
MOD value establishes a new trigger. A later shot requires its own guard,
SD and baseline. Cancel performs zero rollback or retrigger writes.

**Every observation.** All job transfers are tracked. Count one physical
outcome per invoked callback independently of operation status. Normalize
nonterminal adapter status to `I2C_ERROR` with its detail. Preserve a
known NACK phase, short transfer, bus error or transport timeout. Local
refusal, cancellation, work exhaustion and deadline expiry add no physical
failure. Complete CONFIG reads, including late ones, retain raw fields
and consumed DATA_READY. OR alert-mode HIGH/LOW into the event record with
source, mode, generation, timestamp validity and uncertainty. Aggregate
mixed origins explicitly. Therm HIGH is a live level. Every completed TEMP
read, including a diagnostic or late read, consumes outstanding readiness.
An ambiguous CONFIG read can also lose alert events. A failed TEMP
can lose readiness alone. Preserve historical events on either failure.
Use timely, eligible evidence once. Late observations and partial results
cannot clear dirty, initialize, publish a successful sample or replace the
last-good cache. [Read effects and output exceptions](api-contracts.md#status-and-retained-evidence).

## Application bus ownership

Charge a combined pointer/repeated-START/two-byte word read as one sensor
callback, and a pointer/MSB/LSB data write as one. The board adapter owns
one critical section per callback. Acquire the bus, select the target route
through STOP, keep the lock through the sensor transaction and release it
on every exit. Re-establish the route on each callback. Route failure
suppresses downstream sensor traffic. The turn still spent its callback.
Count mux transactions separately in board instrumentation and bus budgets.
Keep the same physical sensor under exclusive ownership across dependent
reads, writes and waits. Release the bus to other sensors between callbacks.

The adapter enforces the supplied timeout across lock, mux and controller
work, checks complete lengths and returns one terminal status. If its SDK
cannot bound the transfer within the remaining time, refuse before starting
the transaction. No retained buffers, re-entry, hidden retries or bus lock
held over a conversion wait. The board sets electrical timing. Between START
and STOP, if the controller holds SCL low or the sensor holds SDA low
without clocks, TI gives a **20–40 ms** serial-interface timeout (35 ms
typical in prose). This concerns the sensor
interface. The application bounds its callbacks independently, and a job
can span many short transactions. [Exact-part timing rows and prose](device-behavior.md#polling-and-deadlines).

## Acceptance traces

Each `R` or `W` below is one terminal sensor callback. Counts exclude mux
traffic. Except where stated, the fixture supplies a free, appropriately
initialized, verified sensor and a timely clock.

| Case | Stimulus | Expected result |
| --- | --- | --- |
| Admission and teardown | Change each configure binding field in turn with a saved token in the output. Separately start a valid job and try bind/end/unbind/invalidate, ID/EEPROM reads and another start before and after cancel. | Mismatch gives `INVALID_CONFIG`, preserves the saved issuer and serial, and uses zero callbacks. Valid start gives a new token and zero callbacks. All competing calls give `BUSY` until take. |
| Retained identity | Sensors A and B each issue serial 1. Cancel A, poll A again, cancel again, try B's token on A, then take A's token twice. After end/rebind, cancel A's next accepted job and try the old A token before taking the new one. | The first sequence returns `CANCELLED`, `CANCELLED`, `BUSY`, `TOKEN_MISMATCH`, `OK` with cancelled record, then `RESULT_NOT_AVAILABLE`. The stale token also gives `TOKEN_MISMATCH` with output and pending result intact. Zero callbacks throughout. The last allocatable serial works once. Even after end/rebind the next valid start gives `WORK_LIMIT` with its output intact. |
| Wrap and equality | At hook `0xFFFFFFF8`, q=1, start LATEST with 12 ms, passing caller time 100000. At hook 1, poll with caller time 0, callback cap 50. Return OK at hook 4. Repeat with `poll(any,0)` at 4 before any transfer. | First run grants 3 ms, counts one success, records TEMP and its read effect, and retains timeout without sample/cache replacement. Second run times out with zero callbacks and no valid last physical status. |
| Coarse clock | q=10, FIRST_ACK at floor tick 50. Try the 3 ms wait at 60 and 70. In a separate run admission at 40 with requested 39 ms gives expiry 70. Exercise timeout 0, 9, 10, `INT32_MAX` and one above it. | First eligible tick is 70. If expiry is also 70 the guard never runs. Only 10 and `INT32_MAX` admit, with budgets 10 and 2147483640. |
| Two routes, one turn each | A is a TMP117 in SD, desired stable word `0x0620`, THIGH 20 °C. Start ONE_SHOT. Poll A with budgets 1,1,1,1,1. Between A's turns let B on another route perform its own LATEST. | A emits `R04=0000, R01=2620, W01=0620, R01=0620, W01=0E20`, one per turn. Preflight consumed old ready. The SD baseline reads clear. B emits one TEMP read. Each callback reselects its target. A wait turn spends zero. A later budget 255 can issue `R01=AE20, R00=0C80` on A for a fresh 25 °C and captured HIGH. Seven A callbacks, one trigger. |
| Drift after the trigger | In the two-route trace, A's first ready poll instead returns `R01=AE40` on time. Consume A's alert events before taking its result. | The owned MOD=11 matches, but AVG changed. Six A callbacks, one trigger, zero TEMP reads. Return `CONFIG_MISMATCH`, keep the target dirty and cache intact. Both the alert consumer and the retained job record observe HIGH. The job also records consumed DATA_READY. |
| Missing or unsafe evidence | Preflight against desired `0620` returns `0640` or unowned `0E20`. Separately inject guard `C000`, guard `4000`, or safe guard then matching CONFIG with BUSY/ready. | Preflight drift gives two reads, dirty and zero writes. EUN gives one read and `EEPROM_UNSAFE`. Timed BUSY can only recheck the guard after the paced interval. Untimed fresh with matching desired CC stops on BUSY in one or two reads, zero writes and zero TEMP. |
| Cancellation around a write | Stage a new limits pair. Stop on either side of the first SD write, or make that callback return a data NACK after a possible partial write. | Before invocation cancel preserves the old target. After invocation the candidate remains desired and dirty, including on NACK. Count the attempted write once, record its status and possible effect. No compensating transfer. |
| Late verification or trigger | Let the matching final CONFIG of initial replay return HIGH at expiry. Separately let an OS trigger succeed at expiry, or cancel the shot on a later wait turn. | Late read gives one tracked success for that callback and retained HIGH. Initialization stays false, target dirty, no new verified generation. Late trigger gives timeout with physical OK. Both shot exits preserve owned-trigger provenance, with zero extra writes. An ambiguous trigger error gives dirty uncertainty instead. |
| Continuous baseline and destructive reads | In CC, final verified apply CONFIG returns ready. Try fresh with no later ready. On another attempt CONFIG returns HIGH and a new ready, then TEMP fails. Take any pending result and try again with a safe guard and ready clear. Separately fail CONFIG after capturing HIGH. | Both ready-clear tries stop after guard and CONFIG with `MEASUREMENT_NOT_READY`, zero TEMP reads and the cache intact. The failed TEMP discards the saved ready and records possible loss. The failed CONFIG can also lose alert history. Captured HIGH survives later zero flags and both failures until explicit event consumption. |
| Work and transport | Park at a future due tick. Poll with budget 255 on a frozen clock. Separately feed only successful BUSY words with advancing time and a long deadline, or make the first guard fail. | Each parked turn returns promptly. The 1,000,001st unchanged unsuccessful wait check gives `WORK_LIMIT`. No timed operation executes a 257th phase dispatch. A timely guard transport failure ends after one callback. |

These traces are design inputs for the shared engines in stages 18 and 21,
the slot in 28, their integration in 29–30 and fault injection in 31.
[Producer and consumer boundaries](prompts/HANDOFFS.md).
