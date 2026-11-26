# TMP11x public API contract

This is the design for the C++17 class `TMP11x::TMP11x`, included through
`<TMP11x/TMP11x.h>`. A bus owner can bind a sensor and inspect its registers
without changing its configuration. An application taking measurements
explicitly initializes a profile and chooses how to acquire each sample.
Feature stages introduce the declarations with their implementations.
[Implementation owners and coverage](architecture.md).

## Available lifecycle and identity operations

Construct one `TMP11x::TMP11x` at a stable address per sensor. The owner
serializes every call, including local inspection. `bind(const Config&)`
validates the binding fields, every settings selector and the THERM limit
ordering before copying the target. An invalid candidate returns `INVALID_CONFIG` and preserves the
previous binding and observations. Every accepted bind, even to the same
route, starts a new observation history with unknown profile trust.

Use `isBound()` to check whether a binding exists and `identityInfo()` to
inspect the bound model, address and last complete ID locally. A new binding
has `rawValid == false`. `readIdentity(out)` and `probe()` each perform one
combined read using `i2cTimeoutMs` and cache a complete ID even on
`IDENTITY_MISMATCH`. Before binding, both return `NOT_BOUND` locally.
A transfer failure preserves the cached evidence and any caller output.
These operations need no clock.

`desiredSettings()` and `configTrust()` return the local target and current
trust. After an accepted bind, `isInitialized()` is false and trust is
`UNKNOWN`. A compatible ID or a failed transfer preserves initialization,
startup evidence and profile trust. A complete incompatible ID clears that
evidence and retains the desired settings. `begin(config)` requires a clock
and an operation budget of at least one tick before replacing the binding.
`begin()` and `recover()` establish startup timing from their own compatible
ID read when proof is missing. They verify CONFIG and both limits and observe
unmanaged OFFSET on capable models in shutdown. `end()` clears observations while retaining
the binding and desired settings. `unbind()` also drops the binding.
Both operations succeed on an empty instance and invoke zero callbacks.
Stage 28 adds the active-job and pending-result guards described in the
operation table.

## Binding and sensor settings

`Config` is a value copied on acceptance. Its binding fields identify the
device, the borrowed resources and the transport policy for the lifetime of
that binding. Its `settings` member is a `SensorSettings` value. The driver
keeps its own desired copy. Only explicit `bind(config)` or `begin(config)`
can install a different binding.

| Binding field | Meaning and validation |
| --- | --- |
| `model`, `i2cAddress` | Select one of `Model::{TMP116,TMP116N,TMP117,TMP117M,TMP117N,TMP119}` from the BOM. Supply the unshifted seven-bit address `0x48–0x4B` of the known target. |
| `i2cWrite`, `i2cWriteRead`, `i2cUser` | Both synchronous callbacks are required. One opaque context serves both. A stateless function may use `nullptr`. |
| `i2cTimeoutMs`, `operationTimeoutMs` | Positive bounded millisecond intervals, each at most `INT32_MAX`. The former caps one callback. The latter bounds synchronous apply and reset, including waits. Explicit blocking-read and job budgets cover their entire call or job. |
| `nowMs`, `timeUser`, `clockResolutionMs` | Optional `uint32_t (*)(void*)` reporting monotonic elapsed milliseconds rounded down to its tick in one epoch. Its value at callback completion timestamps observations. Supply the clock's tick in whole milliseconds (default 1, valid 1–`INT32_MAX`) when installing the hook. Changing the hook, epoch or resolution requires rebind. |
| `readAlertPin`, `gpioUser` | Optional `Status (*)(bool& high, void*)` reading the physical GPIO level. The context identifies the pin. A missing hook makes this observation unavailable. No pin setup or interrupt handling occurs. |
| `offlineThreshold` | Positive consecutive tracked failure count at which health becomes OFFLINE. |

Callback signatures in namespace `TMP11x` use `Status` from `Status.h`.

```cpp
using I2cWriteFn = Status (*)(uint8_t address, const uint8_t* tx, size_t txLen,
                              uint32_t timeoutMs, void* user);
using I2cWriteReadFn = Status (*)(uint8_t address, const uint8_t* tx, size_t txLen,
                                  uint8_t* rx, size_t rxLen, uint32_t timeoutMs,
                                  void* user);
```

All non-owning contexts may be null if their functions support it. The owner
keeps resources alive through return from the last callback and serializes
access to the instance, including getters. Each callback must return a
terminal result, complete within its budget including lock acquisition, and
release all borrowed buffers before returning. Callback re-entry and ISR use
are unsupported. A successful word read completes one pointer byte and both
MSB-first data bytes in a single repeated-START transfer. A successful write
completes pointer, MSB and LSB. The adapter checks lengths and reports a short
transfer as an error even if the controller returned success. A status-only
core cannot infer missing bytes from a buffer's contents.

| `SensorSettings` field | Replay contract |
| --- | --- |
| `mode`, `cycle`, `averaging` | `Mode::{CONTINUOUS,SHUTDOWN}`, `Cycle` codes 0–7 in TI table order, `Averaging` for 1/8/32/64 conversions. Encode continuous as 00 and accept alias 10. One-shot and addressed reset each have a separate operation. |
| `alertMode`, `polarity`, `pinFunction` | `AlertMode::{ALERT,THERM}`, `Polarity::{ACTIVE_LOW,ACTIVE_HIGH}`, `PinFunction::{THRESHOLD,DATA_READY}`. All eight combinations are supported. Observe the raw pin independently. |
| `limits` | `Limits{lowCounts, highCounts}` in exact signed 16-bit counts. Both words belong to the target. Validate therm mode with `lowCounts <= highCounts`, including when entering therm. |
| `managedOffsetCounts` (stage 23) | `std::optional<int16_t>` in the same units. Disengaged by default. Engaged zero is a request to apply, verify and later replay zero. Reject any engaged value on TMP116/N before I2C. Releasing ownership preserves the live value. |

For `Config{}` the desired target is continuous, cycle code 4, eight-sample
averaging, alert mode, active-low threshold output, low −32768 and high
24576 counts. These are software defaults. Only a verified readback
describes the device. Callback pointers start null. Defaults of 50 ms per
callback, 1000 ms per synchronous apply/reset and five consecutive failures
are transport policy. `Model::UNSELECTED` and address zero require the owner
to select a model and address before binding. `isValidModel` and
`isValidAddress` check those choices. `getCapabilities(model, out)` fills
`hasOffset`, `hasAddressedReset` and `eepromWordCount` without I/O. An invalid
model returns `INVALID_PARAM` and preserves `out`. Use `Cycle::CODE_0` through
`CODE_7` and `Averaging::SAMPLES_1`, `SAMPLES_8`, `SAMPLES_32` or `SAMPLES_64`
for the stable selectors. CONV codes 0–3 have AVG-dependent periods.
[Continuous timing table](device-behavior.md#conversion-modes-and-intervals).

`setMode`, `setCycle` and `setAveraging` each copy the desired profile and
replace one selector. They require initialized, verified settings. Invalid
selectors on a verified instance return `INVALID_CONFIG` before a callback.
Known drift returns `CONFIG_DIRTY`. Drift first discovered during preflight
returns `CONFIG_MISMATCH` and retains the previous target. If preflight
reads matching CONFIG in a verified continuous generation with an established
baseline and BUSY clear, a timely READY observation stays eligible even if
a later limit read fails before the first write. A late CONFIG read still
records its flags and grants no new readiness. At the first
attempted write, the candidate becomes the recovery target and the previous
readiness baseline expires. Even a request for the current selector goes
through guarded shutdown, field verification and restoration of the desired
mode. The timely final CONFIG read starts a new verified generation, consumes
any READY seen there and retains the returned alerts. A cached sample keeps
its original generation.

`getConversionTiming(model, cycle, averaging, out)` in `<TMP11x/Timing.h>`
is independent of the binding. It returns durations in microseconds, the
number of conversions per result and the nominal continuous update rate in
hertz. The active estimate also describes a single one-shot block. Shutdown
has no periodic result stream. Invalid model or selector values return
`INVALID_PARAM` and preserve the output.

Stage 11 supplies binding and CONFIG/limits. Stages 18–21 can verify all
then-owned fields and publish samples with unknown offset provenance. Stage
23 adds the optional member, setter, readback, observation of unmanaged
calibration on supporting devices, and recovery through the same engine.
TMP116/N apply never accesses `0x07`. Validate every available enum, field
and capability before accepting a target. The signed codec uses
`counts / 128.0` °C. For Celsius inputs, reject nonfinite or outside
[−256, +255.9921875] before conversion, round to nearest count with ties
away from zero, and check the rounded range before narrowing. For decoding
use representable signed arithmetic. These bounds describe register values.
The exact-part operating and accuracy limits are given in the [board guide](board-integration.md).

`<TMP11x/Temperature.h>` exposes these bus-silent functions in
`TMP11x::temperature`. Pass the register word to `decodeCounts(raw)`
and its signed result to `toCelsius(counts)`. For a TEMP or limit word
in the opposite direction, `encodeCelsius(celsius, raw)` returns a
`Status`. On `INVALID_PARAM`, the caller keeps the original word.

### CONFIG word conversion

`<TMP11x/Configuration.h>` provides four bus-silent functions in `TMP11x`.
`validateSettings(model, settings)` checks the exact model, all six selectors
and the signed limit pair. In THERM the low limit must be at most the high
limit. In ALERT the limits are independent. `encodeConfiguration(model,
settings, word)` validates the same profile and preserves `word` on
`INVALID_CONFIG`. It builds a canonical volatile write value from the desired
fields, with continuous encoded as MOD=00 and shutdown as MOD=01.

`decodeConfiguration(word)` returns a `ConfigurationObservation`. Inspect
`rawMode()` for the original two MOD bits and `mode` for their interpretation.
`readClearMask` is `0xE000` in ALERT and `0x2000` in THERM. Intersect it with
`raw` to identify asserted flags acknowledged by that read. The original four
status bits remain available, including therm HIGH as a level. The decoder
has no clock. The read owner may attach its completion tick and validity.
An EEPROM lock check needs a separate EEPROM_UL observation at `0x04`.

`compareExpectedRegister(model, settings, pointer, word)` reports
`CONFIG_MISMATCH` for drift in CONFIG or either complete limit word. It
accepts MOD=10 for continuous. For shutdown, observed MOD=11 requires the
explicit fifth argument `OwnedTrigger::CONFIRMED`. Supply it only for a
physically successful trigger with retained ownership. Successful explicit
shutdown or invalidation ends that exception. A match compares one register
at the instant of observation. Profile trust and fresh-conversion evidence
require their own operations.

Power-on and general-call reset load MOD0 and bits 9:2 from EEPROM. MOD1
has no stored value. EEPROM reload after an addressed Soft_Reset remains
undocumented on the models that support it. Read live CONFIG to observe
the device's settings. The word `0x0220` describes the factory field values.
[Register ledger](registers/tmp11x.json), [reset policy](eeprom-and-reset-policy.md#addressed-reset-and-recovery).

## Status and retained evidence

`Status{Err code, int32_t detail, const char* msg}` has static-lifetime
message text, `ok()`, `is(Err)`, `inProgress()`, `Status::Ok()`,
`Status::Error(...)` and `errorName(Err)`. `OK` denotes successful completion
of the requested action. `IN_PROGRESS` belongs to the operation engine.
Normalize a nonterminal or incoherent adapter status to `I2C_ERROR` and
retain its diagnostic detail. Use `I2C_NACK_ADDR` or `I2C_NACK_DATA` only
when the adapter knows the phase. Use `I2C_NACK`, `I2C_BUS`,
`I2C_SHORT_TRANSFER` and `I2C_TIMEOUT` for other identified adapter causes.
`errorName` returns `"UNKNOWN"` for an unrecognized code. The API distinguishes
`INVALID_PARAM`, `INVALID_CONFIG`, `NOT_BOUND`, `NOT_INITIALIZED`,
`UNSUPPORTED`, `CLOCK_REQUIRED`, `BUSY`, `IDENTITY_MISMATCH`,
`EEPROM_UNSAFE`, `CONFIG_DIRTY`, `CONFIG_MISMATCH`,
`MEASUREMENT_NOT_READY`, `OPERATION_TIMEOUT`, `WORK_LIMIT`, `CANCELLED`,
`RESULT_NOT_AVAILABLE` and `TOKEN_MISMATCH`. Driver-side refusals add
zero physical outcomes.

| Public value | Evidence carried |
| --- | --- |
| `IdentityInfo` | Selected BOM model and bound address, complete raw ID when available, recognized group, revision and its validity, compatibility with the selection. The group describes the complete observed word even on a mismatch with the selection. TMP116/N requires exactly `0x1116` and has no decoded revision. TMP117/M/N and TMP119 require low 12 bits `0x117` and expose the high nibble, including zero or an unfamiliar revision. Compatible family, exact ordering code and physical authentication are distinct claims. |
| `HealthSnapshot` | `DriverState`, initialized flag, saturating total successes/failures and consecutive failures, last physical fault/status, observation time and validity. Start at `UNINIT` on accepted bind or teardown. Each tracked terminal success sets `READY` and clears the streak. Each tracked failure increments it and sets `DEGRADED` below the threshold or `OFFLINE` at/above it. Preserve the last fault across success. |
| `SettingsSnapshot` | Desired settings, observed raw/decoded fields with per-field validity and times, last verified generation/time, current trust (`UNKNOWN`, `DIRTY` or `VERIFIED`), first unresolved dirty reason and affected fields. Partial observations stay partial. A detected stable mismatch leaves the target intact. |
| `ConfigurationObservation` | One complete CONFIG word with decoded stable fields, observed MOD, BUSY, DATA_READY and mode-specific HIGH/LOW, observation time/validity and read effects. Preserve the raw MOD even when accepting a continuous alias or an owned one-shot. |
| `Sample` | `uint16_t raw`, `int16_t counts`, `double celsius`, `uint32_t observedAtMs`, `bool timestampValid`, `ReadKind acquisitionKind`, `bool freshConversion`, `uint32_t profileGeneration`, `ConfigTrust configTrust` and `OffsetProvenance offset`. The latter distinguishes unknown, unavailable on this model, observed unmanaged counts and verified managed counts, with validity for the recorded counts. Unknown is the only populated offset state before stage 23. |
| `AlertStatus` | Pending alert-mode HIGH/LOW events, separately observed therm HIGH level and validity, source of CONFIG read, generation/mode, observation time/validity and possible history loss. A mixed/unknown marker bounds the record when events span profiles, modes or uncertain observations. |
| `RegisterObservation`, `RegisterSnapshot`, `EepromSnapshot` | Selected model, identity evidence, ordered pointers and complete raw words, time validity, read effects and uncertainty per observation. Preserve completed prefix and first unsuccessful pointer in a failed composite. Report only available fields. EEPROM snapshot records lock/busy and one-based indexed shadows. |
| `OperationResult` | Token, kind/read kind, terminal status, last physical status/validity, callback count, read effects and possible writes. If the last callback completed a word read, `lastRead` holds its `RegisterObservation` (pointer, raw word and completion timestamp/validity), even at expiry. A write, a failed/short read or zero callbacks leaves this field empty. Only a timely successful READ supplies a `Sample`. Result copies preserve the instance's alert events. |
| `OperationSnapshot` (stage 28) | Slot state (`FREE`, `ACTIVE`, `RESULT_PENDING`). An occupied slot supplies its token and operation kind, plus the read kind for READ. Only a pending result supplies a terminal status. A free snapshot carries no previous operation. |

`isOnline()` is a bus-silent `initialized && (state == READY || state ==
DEGRADED)`. Tracked callbacks drive health even before initialization. A
successful managed ID transfer followed by a family mismatch counts one
success. The mismatch ends the operation. Public probe and diagnostics are
health-neutral on both success and failure. OFFLINE leaves admission open.
Invalidating device trust preserves transport history. This differs from the
initialization-gated transitions in [TMP1x2](library-comparison.md).

`totalSuccess` and `totalFailures` saturate at the 32-bit unsigned maximum.
`consecutiveFailures` saturates at 255. `lastError` retains the normalized
fault code, signed detail and static message across subsequent successes.
`lastSuccessMs` and `lastErrorMs` record the optional wrapping millisecond
clock at the respective callback returns. Each has its own validity flag
(`lastSuccessMsValid` or `lastErrorMsValid`) so a tick of zero is usable.

All output references stay unchanged on failure, including a rejected start
token, failed sample, scalar read and unmatched result token. There are two
evidence-bearing exceptions. An explicit ID read (`readIdentity` or
`readRegister(0x0F, …)`) returns the complete incompatible word alongside
`IDENTITY_MISMATCH`. An admitted diagnostic snapshot
returns a labelled partial snapshot on transfer failure or mismatch, including
an incompatible ID when received. Precondition rejection preserves its
output. Private observations, physical health, potential write effects and
flag bookkeeping survive a composite failure. A `takeResult` of a
matching terminal token succeeds at retrieval even when the job's own
status in the returned record is failure.

A complete incompatible ID on any path also invalidates live device state.
Clear initialization, startup proof, profile trust, live alert level and
ready/trigger evidence. Retain the mismatch, desired settings, transport
history and historical samples/events. While the selected target is known
incompatible, refuse I2C to every other pointer. An explicit ID check,
including the first step of begin/recover or a dump, can establish compatibility
again. Only verified begin/recover restores managed operation. A failed
transfer supplies no complete ID for a comparison.

## Operation gates

The following are member signatures in abbreviated C++ notation. Actions
return `Status` unless a value/predicate is indicated. “Free” means no active
job and no unconsumed terminal result. Local rejection uses zero sensor
callbacks. “Tracked” means one health outcome per physical callback,
including callbacks inside an otherwise failed operation. Reads of several
words always use individual checked transfers.

| Entry point | Required state | Work on acceptance |
| --- | --- | --- |
| `bind(const Config&)`, `end()`, `unbind()` | Free. Validate the complete new binding/profile before changing the instance. | Zero callbacks. Accepted bind installs target directly, discards old observations and starts uninitialized with unknown trust. End clears observations, health, startup and initialization, retaining binding and desired settings. Unbind also drops the binding. Teardown may be called on an empty instance. |
| `begin(const Config&)`, `begin()`, `recover()` | Free and clock. The first overload checks the candidate's required clock and validates its binding/profile before replacing the instance. The other two need an existing binding. Dirty, invalidated and previously failed instances may recover. | Tracked identity, startup, operation guard and full replay. Accepted replacement persists even if subsequent initialization fails. Teardown sends no shutdown command. |
| `invalidateDeviceState()` | Bound and free. | Zero callbacks. Clear startup origin/proof, initialized, live trust, current ready baseline, outstanding shot provenance and live alert level. Retain binding, target, health and historical samples/events under their original provenance. |
| `readIdentity(IdentityInfo&)`, `probe()` | Bound, free, known authorized target. | One neutral ID read at `0x0F`. Probe returns compatibility status. No initialization/guard or clock required. |
| `readRegister(uint8_t pointer, RegisterObservation&)`, `readEepromWord(uint8_t index, uint16_t&)` | Bound, free, mapped pointer or valid one-based shadow index. | One neutral word read. A lone observation can have unknown identity/startup/profile/freshness. Label pointer `0x07` by selected model. EEPROM words accept every bit pattern. Unsupported requests reject locally. |
| `readRegisterSnapshot(RegisterSnapshot&)`, `readEepromSnapshot(EepromSnapshot&)` | Bound and free. | Neutral bounded dump. Read ID first, stop on error/mismatch. Full dump then reads permitted pointers `0x00` through `0x08` in ascending order, for ten callbacks including ID. EEPROM snapshot then reads `0x04` and exactly four or three mapped shadows, for six or five callbacks. No data writes. |
| `readConfiguration(ConfigurationObservation&)`, `readLimits(Limits&)`, `readTemperatureOffset(int16_t&)`, `verifyConfiguration()` | Initialized, bound, free. Offset requires capability. | Tracked reads. CONFIG alone consumes flags. Read both limits, or one offset, as requested. Verify reads CONFIG and both limits, adding supported offset from stage 23. Compare owned fields and record actual unmanaged calibration. Partial reads update observed validity. Verification reports drift and can confirm an already clean profile. Repair of dirty or invalidated state requires full verified replay. |
| `configure(const Config&)`, `setMode(Mode)`, `setCycle(Cycle)`, `setAveraging(Averaging)`, `setLimits(Limits)`, `setAlert(AlertMode, Polarity, PinFunction)`, `setTemperatureOffset(double celsius)` | Initialized, verified, free, stable power, clock, valid candidate and capability. Configure compares every binding field, including callback/context identities and transport policy. `setMode(SHUTDOWN)` is the explicit shutdown operation. | Tracked guarded apply, including for limit-only or pin-only changes. The offset setter validates/quantizes the Celsius request and opts into management. Use configure with disengaged offset to release it. On known drift use recover. |
| `readLatest(Sample&)`, `readTemperature(double& celsius)` | Initialized and free, including dirty. Either desired mode. | One tracked TEMP. Both forms cache the complete sample with freshness false and an optional completion timestamp. The scalar form copies Celsius on success. Stage 18 supplies verified initialization. |
| `tryReadFresh(Sample&)` | Initialized, verified clean, retained startup proof, desired continuous and free. | Untimed, at most three tracked callbacks (guard, CONFIG if needed, TEMP if eligible). One attempt. A wait condition returns immediately. |
| `readFreshBlocking(Sample&, uint32_t timeoutMs)`, `readOneShotBlocking(Sample&, uint32_t timeoutMs)` | Same trust/ownership as try-fresh, clock and positive bounded deadline. Continuous for the former, shutdown for the latter. | Tracked, paced continuous observation or one guarded trigger followed by polling. Each step uses at most one callback. |
| `reset()` | Initialized, free, supported model, verified stable CONFIG, valid startup proof, clock and guard. | One addressed command plus bounded observations described below. No replay on return. Later explicit recover can reapply the retained target. |
| `getSettingsSnapshot()`, `healthSnapshot()`, `isOnline()`, `getLastSample(Sample&)`, `getAlertStatus()`, `takeAlertEvents(AlertStatus&)` | Memory-only, available even while a job/result owns the slot. Missing sample yields not-ready. | Zero sensor callbacks. Only take drains the captured alert events/loss indication. It leaves therm observation and hardware alone. A sample-age query needs a valid timestamp and the same clock epoch. |
| `readAlertPinLevel(bool& high)` | Bound with GPIO hook. Owner-serialized. | One optional GPIO observation, zero I2C. Exposes the physical level, independent of pending events and fresh evidence. |
| `OperationSnapshot getOperationSnapshot() const` | Any slot state, including unbound. Owner-serialized. | Return the stored slot metadata by value. Inspection reads no clock, performs no I2C and preserves the job and its result. |
| `startInitialize`, `startConfigure`, `startRecover`, `startRead` | Free, bound, clock and the corresponding synchronous gates. INITIALIZE reuses binding. Configure validates binding equality and a copied candidate. | Admission has zero I2C. Only a subsequent poll executes the shared engine. |
| `poll(nowMs, maxTransfers)`, `cancel()`, `takeResult(token, out)` | Active job for work/cancel, matching terminal token for take. | Poll spends at most the caller's callback budget. Cancel and take use zero. Terminal ownership persists until matching take, even on timeout or cancellation. |

`getSettingsSnapshot()`, `healthSnapshot()` and `getAlertStatus()` return
value copies. `getLastSampleAgeMs(uint32_t& age)` returns a status and uses
the authoritative clock only when the cached time is valid in that epoch.
A clockless binding supports identity and bounded diagnostics. The
latest-value read engine itself requires no clock for decoding or
publication, while public managed initialization requires one.

`readLimits(pair)` reads TLOW then THIGH. It publishes the signed pair on a
complete read, including a pair that reveals drift. A transfer failure keeps
the caller's pair intact. Each complete word updates the settings snapshot.

`AlertStatus::firstEvent` identifies the CONFIG read that first contributed a
pending HIGH or LOW. It carries the source operation, observed MOD, generation
and optional completion tick. `mixedOrUnknown` qualifies that attribution when
later events differ in source, mode or generation, or a capture lacked verified
settings. `lastConfigSource`, `lastConfigAtMs` and `readClearMask` describe the
latest complete CONFIG read, including a late success. Its clear mask is
`0xE000` for alert and `0x2000` for therm. An engaged `thermHigh` gives the
level at that read. An attempted CONFIG write, ambiguous CONFIG read or loss
of live device state invalidates the level. A failed CONFIG read, reported
outside access or an incompatible ID after startup can leave a gap in event
history (`possibleLoss`). Events captured before that gap retain their origin.

`takeAlertEvents(out)` succeeds even with an empty record. It copies the
status and clears the pending bits, first-event origin, mixed marker and loss
indication. The latest read metadata and therm level remain available.
`readAlertPinLevel(high)` returns `NOT_BOUND` before binding and `UNSUPPORTED`
when the binding omits the hook. A failed GPIO sample preserves the caller's
level and transport health. A hook returning `IN_PROGRESS` violates its
synchronous contract and yields `INVALID_CONFIG`.

Every complete live read of CONFIG, either limit or a supported offset
updates the corresponding observation, including a diagnostic word in a
partly failed dump. Compare owned fields against their verified values or,
during apply/readback, the values expected at that step. A mismatch latches
dirty, preserves the target and invalidates current ready evidence. A live
observation API returns its observed data on a complete read.
`verifyConfiguration()` reports `CONFIG_MISMATCH` on drift. Apply
verification or acquisition stops on drift. A later matching diagnostic
cannot clear dirty.

`getSettingsSnapshot()` uses optional observations for CONFIG, TLOW, THIGH
and supported OFFSET. An attempted write discards the previous observation for its register.
A failed read supplies no replacement. During an explicit verification,
`verificationReads` records each complete register using the `SettingsField`
bits. If TLOW differs and THIGH then times out, the call returns the transport
timeout, with TLOW drift and the completed CONFIG/TLOW prefix in the snapshot.
Replay preflight preserves this mask. The first replay write attempt clears it.
A matching scan of an already dirty profile returns `CONFIG_DIRTY`. A clean
scan updates `verifiedAtMs` while keeping the generation established by
replay. Explicit invalidation preserves both as historical evidence.

During begin/recover, initial reads establish the starting values for
replay. Verification compares values at the step where they must already
have taken effect. Unmanaged offset records calibration provenance without
joining the target.

On an offset-capable model, compare successive complete unmanaged OFFSET
observations while relying on the same uninterrupted device state. A change
from `0x0000` to `0x0040` without an intervening owned offset change breaks
the exclusive-access assumption. Keep the observed word. Invalidate live
device state, including startup proof, initialization and all saved readiness,
as for outside interference of unknown scope. An observation query returns
the word. A timely change detected in managed verification or replay gives
`CONFIG_MISMATCH`. Historical samples retain their original calibration.
Once the owner restores stable power and exclusive access, explicit recovery
observes the calibration and verifies the owned profile. The first observation
after bind, invalidation or reset establishes a new comparison baseline.
In all cases an unmanaged offset stays outside the replay target.

The device instance has a stable identity and cannot be copied, moved or
assigned. Tokens and retained result values can be copied. Jobs use
`OperationKind::{INITIALIZE,CONFIGURE,RECOVER,READ}` and an explicit
`ReadKind::{LATEST,CONTINUOUS_FRESH,ONE_SHOT}`. Start signatures take
`(uint32_t nowMs, uint32_t timeoutMs, OperationToken& token)`, prepended
by `const Config&` for configure or `ReadKind` for read. `poll(uint32_t
nowMs, uint8_t maxTransfers)` returns pending while work remains. Start and
poll retain the peer's `nowMs` argument for source familiarity. The hook
alone governs deadlines, timestamps and due times. During active or retained
ownership, unrelated I2C, a new start, bind, end, unbind and invalidation
return BUSY without changing state. After power loss stop using the old
proof, cancel if active, take the terminal result, invalidate, restore
stable power and explicitly recover. The owner can inspect the terminal
status repeatedly. One matching take transfers the result and frees the slot.

Every timed path samples the clock before and after the callback, clips
its timeout to the remaining whole-operation budget, and expires at equality.
Use unsigned elapsed differences with finite positive relative durations
at most `INT32_MAX`. The owner guarantees continuity and less than one
wrap between the unwrapped floor readings used to measure a deadline
interval or a sample age. Round the whole-operation budget down to
complete ticks at admission. Reject a budget shorter than one tick before
work or token commitment. This makes expiry observable on the timebase
used by the post-callback check. A zero-transfer poll can expire the job
and never calls the bus.
Blocking wrappers bound CPU steps as well as time, including a stuck clock.
Cooperative waits return pending without a sleep/yield or a held bus lock.
For a minimum wait after callback completion, the hook supplies only a
floor reading `t`. With clock tick `q = clockResolutionMs` and interval
`d`, allow the next transfer after elapsed time `q + ceil(d/q) × q` from
`t`. The extra tick covers the unknown completion phase. For example, on
a 10 ms clock a FIRST_ACK reported at 50 ms can occur just before 60 ms.
The first eligible tick for the 3 ms startup wait is 70 ms. Compute the
rounded interval in a wider type and compare with the remaining deadline
before scheduling. A due time at or beyond expiry permits zero transfers.
Stage 9 specifies the owner-facing scheduling, work limits and token
lifetime in detail. [Cooperative owner operations](owner-operations.md).

## Startup, apply and reset

1. Validate locally. Begin/recover and configuration replay read compatible ID
   before dependent access. If startup proof is missing in begin/recover,
   timestamp that operation's first successful compatible read at completion
   (`FIRST_ACK`). Under the owner's continuous stable-power/no-external-reset
   guarantee, wait at least 3 ms from that origin, rounded conservatively for
   clock resolution. A read can succeed during load. TMP116's BUSY descriptions
   conflict, and the approximate 1.5 ms POR time lacks a specified maximum.
   Never use a failed operation's pending origin, a binding time or a caller
   seed. Retain established proof while the guarantee holds.
2. Obtain a new `0x04` word for each mutation or fresh acquisition. EUN bit
   15 set latches `EEPROM_UNSAFE` immediately, including on diagnostics or
   with BUSY also set. A diagnostic returns the observed word and records
   the unsafe condition. A guarded operation stops with `EEPROM_UNSAFE`.
   Ordinary recovery, shutdown, addressed reset and a
   later diagnostic zero cannot clear it. The owner must power-cycle or
   arrange external relock, invalidate and re-establish startup and ownership.
   With EUN clear and BUSY bit 14 set, only a timed path may pace successful
   `0x04` rereads within remaining work/deadline. An untimed try returns
   `MEASUREMENT_NOT_READY` after that one callback.
   No retry follows a failed transfer. A later observed BUSY (including
   CONFIG bit 12) cancels the guard without itself implying drift. If
   untimed try-fresh reads CONFIG with BUSY set, retain the returned flags
   and issue no second guard or TEMP read. Return `MEASUREMENT_NOT_READY`
   for BUSY with matching fields. Stable-field drift still latches dirty
   and returns `CONFIG_MISMATCH`.
   A timed path may renew the guard before dependent work. A known unlocked
   target still requires external correction across teardown or rebind.
   Interference, reset, rebind or ambiguity also ends the guard. Never
   carry it to the next operation.
3. Read current stable fields/limits as needed for safe ordering. Establish
   explicit shutdown. A settings candidate becomes the retained desired
   target immediately before the **first mutating callback attempt**,
   including this SD write. A pre-write refusal/cancellation preserves the
   previous desired settings. Mark the profile dirty and discard current
   conversion evidence when mutation begins. Accepted bind installs its
   target earlier. Use canonical CONFIG words (stable bits 11:2, continuous
   00 or shutdown 01, zero status, command and reserved bits).
4. In guarded SD apply and read back the target stable fields and both limits.
   Read and preserve supported unmanaged offset, or apply and read back an
   explicitly managed target. Choose limit order using the known current
   pair. For [0,10] → [15,20] write high first. When lowering high below old
   low, write low first. For an unknown or inverted initial pair, require
   full apply/recovery, establish a valid verified pair before entering
   therm, and keep intermediate observations untrusted. On a transition to
   therm, verify the requested ordered pair before setting T/nA. When
   leaving therm for an inverted alert-mode pair, first write and verify
   alert mode in guarded SD while the old limits still hold. Only a timely
   matching readback under a valid guard permits a limit write that would
   invert the pair.
   Separate writes can transiently change comparator/pin behavior. Stop
   after failure and retain the candidate and partial observations for
   explicit recovery.
5. Consume old readiness with a CONFIG read in the expected SD profile.
   Write the desired mode. Read CONFIG again, check **every** stable field,
   feed alert bookkeeping and consume its readiness as the new generation
   baseline. Only timely verification of the entire owned target commits a
   new verified generation, clears dirty and admits initialized acquisition.
   Failure at the last read leaves dirty even when every write ACKed.
   If that CONFIG reports BUSY, keep its read effects, renew the guard at
   `0x04` within the remaining budget and repeat final verification before
   committing the generation.
   Reconfiguration of an already initialized target can therefore leave
   latest reads available with untrusted provenance. No rollback is implicit.

Device reset uses the last verified stable settings with canonical MOD=00/01,
zero status/reserved bits and bit 1 set on TMP117/M/N or TMP119 only. Discard
startup/guard, initialization, live profile/alert trust and shot/ready
evidence immediately before the single command attempt. Preserve desired
settings and historical events/samples. A failed attempt leaves uncertain
effects. After physical success wait at least 2 ms from completion, rounded
up, before any post-reset observation. Establish a new postwait compatible
ID/FIRST_ACK, observe startup wait, safe `0x04` and full profile readback.
Report mismatch/unsafe/transport failure explicitly. Keep initialization
clear until explicit begin/recover. No observed reset bit, factory image,
EEPROM reload, EUN state or bus availability is assumed. [Safety policy
and per-model evidence](eeprom-and-reset-policy.md).

## Acquisition and read effects

- **Latest.** A timely complete TEMP read publishes the word, including
  possible startup sentinel `0x8000` or an old value after a settings
  change. In a verified continuous profile that lost its readiness baseline,
  a complete TEMP read establishes a new boundary at its completion. A later
  DATA_READY indication can then count toward freshness. Record the generation
  and trust *at observation*. Latest alone does not attribute the stored word
  to a conversion under those settings.
  `0x8000` decodes to −256 °C and can be a confirmed saturated result.
- **Continuous fresh.** Obtain this operation's safe `0x04`. If the current
  generation lacks a baseline, the first destructive baseline read consumes
  old readiness without publishing it. Otherwise use one unconsumed ready
  observation of the same verified generation, or compare CONFIG and require
  DATA_READY set. Then read TEMP once. A not-ready try preserves caller
  output and cache. Blocking and cooperative fresh requests pace CONFIG
  checks at the nominal CONV × AVG table period within the whole deadline.
  Time, a changed temperature or a GPIO transition supplies no fresh proof.
- **Owned one-shot.** Require desired shutdown, initialization and verified
  clean settings locally. Read `0x04`, then CONFIG and compare all owned
  stable CONFIG fields **before any CONFIG write**. Mismatch or unowned
  MOD=11 latches dirty with zero CONFIG writes. Write guarded SD to abort
  residual work, read and compare CONFIG in SD to consume stale readiness,
  then write MOD=11 **once**. Keep desired shutdown. From physical trigger
  completion, schedule the first readiness poll at AVG count × the applicable
  single-conversion maximum, rounded up for the authoritative clock. Later
  polls follow the bounded pacing policy. Require new DATA_READY for this
  trigger, then read TEMP. MOD is never proof of completion or quiescence.
- **Trigger ownership.** A terminally successful trigger permits MOD=01 or
  11 as compatible with desired shutdown during the shot and after
  completion, timeout or cancellation, until a successful explicit SD write
  or invalidation/rebind. All other fields must still match. A late trigger
  retains physical ownership and returns timeout. An ambiguous SD/trigger
  invalidates trust and cannot establish verified shutdown or a new owned
  trigger. Cancel/timeout performs no restore or retrigger. A later explicit
  shot must pass its own preflight and SD/baseline boundary.
- **Every CONFIG/TEMP reader.** Compare observed CONFIG against the expected
  stable fields for the current step when known. On drift, retain its flags,
  mark dirty and stop dependent acquisition even if ready was set. CONFIG
  consumes DATA_READY and acknowledges alert-mode HIGH/LOW. OR captured
  alert events into the bounded historical record before any later step.
  A later zero cannot erase them. Therm HIGH is a live hysteresis level
  and LOW is zero. TEMP consumes DATA_READY only. Diagnostic CONFIG may
  retain eligible same-generation readiness. TEMP, ambiguous destructive
  reads, reset or outside access consume or invalidate that evidence.
  A failed CONFIG read can lose readiness and alert-mode events. A failed
  TEMP read can lose readiness alone. Both preserve previously captured alerts.
- **Publication.** A late successful read updates physical health (when
  tracked), observed words and read effects, even in a failed composite.
  It cannot publish a sample, replace the last good cache, mark initialized
  or clear dirty. Late writes retain their physical effects. Cached samples
  are immutable acquisition records. After reconfiguration, invalidation or
  reset, expose them as historical. Sample-age validity requires the same
  clock epoch and a known observation time, including a valid zero. Record
  offset observations independently of conversion attribution. Only a new
  confirmed conversion under the verified generation may be attributed to
  its changed offset. Never add the chip's offset to TEMP again.

For example, a cached fresh generation-7 sample at raw `0x0C80` remains
25 °C with its generation-7 offset and observation time after a verified
offset change creates generation 8. A latest read of the same bits under
generation 8 reports `freshConversion=false`. A later generation-8 ready
observation plus a timely TEMP read can publish a fresh 25 °C again.

A ready bit can coalesce conversions. An MSB-first combined read is required,
but TEMP byte latching during a simultaneous continuous update is unspecified.
Under exclusive ownership a completed one-shot in shutdown has a stable TEMP.
Missed events return not-ready or timeout. The polling intervals, timing
conditions and unresolved TMP117 DATA_READY observation are recorded in
[conversion behavior and vectors](device-behavior.md).
