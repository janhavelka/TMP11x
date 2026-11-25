# TMP11x conversion timing, freshness and alerts

The same temperature word can be a reset value, an old result or a newly
completed conversion. To publish a fresh sample, the owner needs a verified
profile, an acquisition baseline, evidence of a subsequent result update and
a timely, successful TEMP read. The six BOM selections fall under three TI
datasheets. Page references below are one-based PDF pages. The independent
literals and event sequences are inputs to future host tests.
[Behavior vectors](registers/behavior-vectors.json), [register ledger](registers/tmp11x.json).

## Conversion modes and intervals

| MOD written | Physical transition | Result boundary |
|---|---|---|
| 00 or 10 | Continuous conversion (CC). The 10 alias reads back 00. | One TEMP update per completed block. The device spends any remaining cycle time in standby. |
| 01 | Enter shutdown (SD) and abort conversion in progress. | A partial block supplies no result. An earlier TEMP word can still be read. |
| 11 | Execute one block, then return to physical shutdown. | One-shot (OS) has active conversion time only. AVG selects the block length. CONV has no effect. |

TMP116 specifies one-shot entry from shutdown. TMP117 and TMP119 describe
the trigger and return to shutdown. The managed sequence below explicitly
enters shutdown on all three. Only MOD0 can persist in EEPROM. The startup
mode can therefore be CC or SD. MOD readback during and after OS is unspecified.
[TMP116 pp.13–14, 26](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=13),
[TMP117 pp.14–15, 27–28](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=14),
[TMP119 pp.16–17, 34–35](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=16).

Each column below gives the **nominal continuous period in milliseconds**.
TI's CONV field descriptions refer to standby, but the mode diagrams and
cycle tables include active time in the displayed period. These are the
literal cells on all three parts. No maximum continuous period is specified.
[TMP116 Table 7](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=27),
[TMP117 Table 7-7](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28),
[TMP119 Table 8-6](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=35).

| CONV bits | AVG 00 · 1 | AVG 01 · 8 | AVG 10 · 32 | AVG 11 · 64 |
|---|---:|---:|---:|---:|
| 000 | 15.5 | 125 | 500 | 1000 |
| 001 | 125 | 125 | 500 | 1000 |
| 010 | 250 | 250 | 500 | 1000 |
| 011 | 500 | 500 | 500 | 1000 |
| 100 | 1000 | 1000 | 1000 | 1000 |
| 101 | 4000 | 4000 | 4000 | 4000 |
| 110 | 8000 | 8000 | 8000 | 8000 |
| 111 | 16000 | 16000 | 16000 | 16000 |

With a 1 s cycle and eight conversions the table budgets about 125 ms
active and 875 ms standby. When the block fills or exceeds the selected
cycle, standby is zero. TMP116 Table 7 marks these cells with footnote 1.
The 1 s cell at AVG 11 does not imply an available 8 ms quiet window.
The **active** durations below use the electrical rows independently of the
rounded cycle table. Each entry is min / typ / max in ms, calculated as
conversion count × single-conversion time under the cited conditions.

| Conversions | TMP116, TMP116N | TMP117, TMP117M, TMP117N | TMP119 |
|---:|---|---|---|
| 1 | 13.5 / 15.5 / 17 | 13 / 15.5 / 17.5 | 13 / 15.5 / 17.5 |
| 8 | 108 / 124 / 136 | 104 / 124 / 140 | 104 / 124 / 140 |
| 32 | 432 / 496 / 544 | 416 / 496 / 560 | 416 / 496 / 560 |
| 64 | 864 / 992 / 1088 | 832 / 992 / 1120 | 832 / 992 / 1120 |

TMP116 specifies one conversion over −55 to +125 °C and 1.9–5.5 V.
TMP117 specifies **one-shot mode**. Using its row for continuous active time
is an extrapolation. TMP119 specifies **no internal averaging**. On TMP117
and TMP119 the table conditions are −55 to +70 °C at 1.7–5.5 V, or −55 to
+150 °C at 1.8–5.5 V. Typical values on all three are at +25 °C and 3.3 V.
The multi-conversion rows assume successive conversions under those conditions
with no added block overhead. They exclude controller time. Increasing AVG
trades a longer active interval for more samples per result. The precision,
current and thermal conditions of the exact BOM part still apply.
[TMP116 p.5](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5),
[TMP117 p.6](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6),
[TMP119 p.7](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7).

`getConversionTiming` reports the table period, nominal standby, reciprocal
update rate and active estimates for a proposed configuration. With cycle
code 4, increasing AVG from 8 to 64 keeps the nominal rate at 1 Hz while
reducing the standby budget from 875 ms to zero. Averaging more conversions
can reduce random noise, but it integrates changes in temperature over a
longer interval and can raise average current as active time replaces
standby. A longer cycle at fixed AVG can lower average current at the cost
of less frequent updates. Accuracy and current specifications apply under
the selected grade's documented conditions.
[TMP116 averaging and cycles](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=12),
[TMP117 averaging and cycles](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=12),
[TMP119 averaging and cycles](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=14).

## Polling and deadlines

Use the owner's authoritative monotonic clock. For a wait measured from
callback completion, schedule the first tick at or after the latest possible
completion time plus the required interval. For a known tick-aligned completion,
round the interval up to whole ticks. If a floor clock reports tick `t` with
resolution `q` and the sub-tick phase is unknown, use `t + q` as the upper
bound for scheduling that wait. This applies to startup, addressed reset and paced
conversion polls. For example, a reset finishing at 50.9 ms reads as 50 ms on
a 1 ms floor clock. Tick 53 is safe for the 2 ms wait. Fix the whole-operation
deadline at admission.
It covers guard, preflight, trigger or baseline, wait, CONFIG polls and TEMP
read, including time inside each callback. A callback gets at most the
remaining budget. Re-read the clock on return. A physical success can
consume flags or launch a shot after the logical deadline. Record that
effect and return timeout without publishing a sample. A due time beyond
the deadline ends the wait with zero further reads. At zero remaining
budget, no new callback can start. A finite work cap also bounds a blocking
helper when its clock fails to advance. Keep the bus available between steps.
[Owner and clock contracts](prompts/ENGINEERING.md).

| Path | Default first eligibility | Later polls and budget |
|---|---|---|
| Owned OS | From terminal successful trigger callback, wait the count-scaled **maximum** active interval, rounded up. Expected duration uses the typical column. | If CONFIG returns ready clear, wait at least 5 ms rounded up from that poll's completion. Allow the upper active estimate plus 10 ms slack for the conversion window. A tighter remaining operation budget wins. |
| Timed CC fresh | From the current generation's baseline or latest readiness poll, allow one CONFIG poll per nominal table period, rounded up. An overdue poll may run once when admitted by the guard and deadline. | Pace again from the actual poll completion. Default conversion-window budget is `max(1.25 × nominal period, upper active estimate) + 10 ms`, rounded up. |
| Untimed CC try-fresh | Make one bounded attempt when the caller asks. | The caller paces subsequent calls. An unsafe or busy guard, missing baseline or absent ready evidence ends the attempt. |

The 25% margin, 10 ms slack and 5 ms OS poll interval are library policy.
For a whole-operation budget, allow separately for setup and transfers at
admission. A wait origin never renews that budget. For example, on TMP117
at AVG 01 an OS trigger completing exactly at 104 ms gives an expected
duration of 124 ms and first CONFIG eligibility at 244 ms on a 1 ms clock. A deadline
of 240 ms precludes that poll. In CC at CONV 010 / AVG 01 the nominal
period is 250 ms. The corresponding window is 323 ms on that clock
(`max(312.5, 140) + 10`, rounded up). A zero-ready poll can therefore
exhaust the window before a second poll is due.

Earlier OS polling is an explicit latency tradeoff. It can communicate
during an active conversion. TMP119 recommends avoiding I²C during active
conversion for best precision. Even default pacing governs only this
driver's traffic. Other bus users can communicate, an active-limited CC
mode may have no standby, and a due time cannot prove accuracy or completion.
[TMP119 p.23](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23).

The serial interface has its own timeout. Between START and STOP a held-low
SCL by the controller or held-low SDA by the sensor without clocks can reset
the interface. TI gives 35 ms typical in prose and 20–40 ms in the timing
tables (fast-mode columns on TMP117 and TMP119). These limits concern the
held-line condition. Conversion latency, total transaction length, callback
timeout and whole-operation deadline are separate quantities.
[TMP116 pp.6, 21](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=6),
[TMP117 pp.7, 22](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=7),
[TMP119 pp.9, 24](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=9).

## Startup and reset evidence

POR loads EEPROM-backed values into volatile registers. Reads can succeed
during load. TMP116 says writes are ignored in that interval. TMP117 and
TMP119 specify ignored CONFIG and limit writes. The effect of an OFFSET write
during load is unknown. The three electrical tables give 1.5 ms typical reset
time with no maximum. TMP116 p.17 says busy is **clear** during load, while
the CONFIG and EEPROM unlock field descriptions say busy is **set** during
power-up. A clear observation is compatible with an unfinished load.
[TMP116 pp.5, 17, 26, 29](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 pp.7, 18](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=7),
[TMP119 pp.8, 20](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20).

When proof is absent, begin/recover records **FIRST_ACK** internally at the
completion of its first successful compatible read. Use a **3 ms policy wait**
from that origin, rounded up, to allow twice the typical reset interval.
This sets a finite wait despite the missing silicon maximum. Bind time and
caller timestamps supply no origin. The owner must ensure stable power and
exclude outside reset/access from that read through every dependent operation.
If power history is unknown, refuse mutation and managed fresh acquisition.

After the wait, read 0x04 and require EUN=0 and BUSY=0 before managed writes
or freshness observation. Verify the owned settings by readback. Retain valid
startup proof across operations. Obtain a new 0x04 guard for each operation
and reuse it only under uninterrupted exclusive ownership within that operation.
EUN=1 refuses immediately, including a proposed shutdown. On BUSY=1 with
EUN=0, a timed operation can make paced 0x04 reads within its deadline and
work cap. An untimed operation returns busy/not-ready. A failed transfer ends
the attempt. If any subsequent CONFIG read returns EEPROM_Busy=1, retain its
flag observations and invalidate the operation's guard. A timed path must
re-establish locked/idle at 0x04 before another write or fresh acquisition
step. An untimed attempt returns busy. BUSY alone leaves stable-field trust
unchanged. CONFIG would consume flags and cannot substitute for 0x04.
[Lifecycle and guard contract](prompts/ENGINEERING.md).

Accepted bind/rebind, end/unbind, reported power loss/reset or a driver reset
invalidates old startup and acquisition evidence. A TMP116/N request for
addressed reset rejects locally. TMP117/M/N and TMP119 specify 2 ms for the
explicit CONFIG bit-1 reset command, which always reads zero. Guard it,
discard prior proof before the command attempt, and on physical success wait
at least 2 ms from the end of that transfer. Recovery establishes a **new**
post-reset FIRST_ACK, waits the startup policy interval, checks 0x04 and
verifies the full owned profile. An ambiguous command leaves trust invalid.
Effects on EUN, EEPROM reload, accompanying CONFIG bits and I²C availability
during addressed reset remain unresolved. The driver issues no general call.
[TMP116 p.26](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26),
[TMP117 p.28](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28),
[TMP119 p.35](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=35).

F13 covers what survives as evidence. An EEPROM-address read observes the
current volatile shadow. A previous writer may have changed the shadow or
programmed a nonfactory word. POR reloads can replace it. Retain old words
only as historical observations and read again for the new generation, even
if the same bits recur. A zero, repeated or unique-looking pattern proves
neither factory identity nor NIST certification. EEPROM data and their
shadows remain read-only in every library path.
[TMP116 p.17](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 p.18](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18),
[TMP119 p.20](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20).

## TEMP, destructive reads and sample publication

TEMP, THIGH and TLOW use MSB-first signed 16-bit counts at 1/128 °C per
count. The endpoints are −256 and +255.9921875 °C. A reset TEMP of 0x8000
has a numeric value of −256 °C and no initial sample validity before a full
conversion, including averaging. A completed result can have this same word,
for example after negative offset saturation on a supported model. The
acquisition history determines validity. A timestamp at the end of a TEMP
read records when the controller observed the result.
[TMP116 pp.11, 25, 28](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=25),
[TMP117 pp.12, 26, 29, 31](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=26),
[TMP119 pp.14, 33, 36, 38](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=33).

Conversions accumulate in nonoverlapping blocks. Until the last constituent
conversion, TEMP holds the prior result and no new DATA_READY is published.
The final conversion publishes the block mean and sets DATA_READY. Aborting
partway through the next block produces no additional mean. An indication
can already be set from an earlier block. The vectors use exactly
representable means because TI does not specify internal rounding.

| Observation | Hardware effect | Evidence the driver can keep |
|---|---|---|
| Successful TEMP read | Consumes DATA_READY. Comparator flags persist. | Latest word, including reset or retained old data. A diagnostic TEMP consumes outstanding readiness. Repeated reads alone cannot claim a fresh sample. |
| Successful CONFIG read | Returns and consumes DATA_READY. In alert mode it acknowledges HIGH and LOW and can release comparator ALERT. | Record returned flags even on diagnostic, preflight, readback or late reads. Keep eligible readiness only for an unconsumed result in the same generation. |
| Failed TEMP or CONFIG read | Flags may already have been consumed. The clear edge is unspecified. | Record possible loss, discard ambiguous ready evidence and preserve previously retained events. No automatic reread. |
| Successful ready observation then failed or late TEMP | The observed update and consumed hardware flags still happened. | Report failure or timeout. Preserve the last-good sample and recorded alerts. |

A bound-only diagnostic dump is a sequence of observations with unknown
profile trust. Its order matters. With ready initially set and no intervening
conversion, TEMP followed by CONFIG returns ready clear in CONFIG. CONFIG
followed by TEMP captures the indication first. Neither sequence produces
an atomic snapshot or a managed fresh sample.

DATA_READY is a single bit. Multiple updates may coalesce, and two valid
successive blocks may have identical temperatures. Even a newly observed
ready bit under trusted continuous settings establishes only that an update
occurred since the baseline. Another update can occur between CONFIG and
TEMP or during the two-byte TEMP transfer. Use one combined pointer and word
read. TI does not specify a TEMP byte latch across a simultaneous update.
Attributing the two received bytes to one CC result assumes word coherence
if the read overlaps an update.
A completed owned one-shot has a stable shutdown result when the owner
excludes further writes and conversions.

A TMP117 user reported a conversion while CONFIG was being read. The read
returned ready clear, the observed ALERT pin deasserted and the next poll also
returned clear. TI recommended one-shot for capturing individual readings.
This establishes a possible missed indication on TMP117. The bit-level
clear edge, frequency and applicability to other parts remain unresolved.
The report describes routing bit 2, but also gives CONFIG as `0x0040`.
The TMP117 register map assigns DR/Alert to `0x0004`. Use the reported
interleaving as a possible observation and the register map for encoding.
Sparse polls limit traffic and can still miss an indication. On a missed
event, a later latest-value read can report the word with freshness unproven.
[TI E2E thread 1019457](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1019457/tmp117-data_ready-flag-cleared-incorrectly-if-data-becomes-ready-during-read-of-configuration-register),
[TMP117 Table 7-6](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27).

### Managed acquisition boundaries

1. **Owned OS.** Require initialized, clean, verified settings, valid startup
   proof and desired SD before I²C. Read 0x04, then read CONFIG and compare
   every stable CONFIG field before writing anything. Drift sets dirty and
   ends the attempt with zero CONFIG writes. Accept observed MOD=11 only with
   retained provenance of this instance's successful trigger and matching
   remaining stable fields. Write explicit SD to abort residual work. Read
   CONFIG in SD to consume stale readiness and retain alerts. Trigger OS once,
   preserving desired SD. Only new ready evidence attributable to this trigger
   admits a TEMP read for fresh publication.
2. **One-shot ownership after errors.** A terminal physical trigger success,
   including one returned after the deadline, establishes provenance for
   observed MOD=01 or 11. Keep it through completion, cancellation or timeout
   until successful explicit SD or invalidation/rebind. A failed ambiguous SD
   or trigger invalidates trust. MOD alone proves neither completion nor abort.
   Cancellation is bus-silent. A later explicit shot starts its own SD
   boundary. A lost indication gives no permission to retrigger.
3. **Apply a continuous profile.** Every managed profile change, including a
   limit-only or pin-only change, uses an operation-local guard, explicit SD,
   apply and verify owned fields, then a CONFIG read to clear stale readiness.
   Enter desired CC and compare every stable field on the final CONFIG read.
   Only a successful match establishes trust and the new generation baseline.
   Suppress fresh publication for readiness consumed by that read and retain
   its alert evidence. Failure leaves dirty state. Discarding one completion
   after a live change cannot establish which settings governed a block.
4. **Read-only continuous try-fresh.** Require trusted CC, initialization and
   startup proof locally, then acquire this attempt's 0x04 guard. No mode or
   register-data writes occur. If the generation has no baseline, its first
   owned CONFIG/TEMP baseline read establishes one without publication.
   Otherwise require a new ready observation and consume it at most once
   with a successful TEMP read. Exclude other readers of this device while
   the sequence depends on its flags. Outside access requires invalidation
   and verified recovery. Cached results preserve their original provenance.

During acquisition and apply verification, compare each CONFIG observation
with the stable fields expected at that step, including the owned-shot MOD
exception. Drift on a baseline,
readback or ready poll latches dirty and ends the operation even if DATA_READY
is set. Retain the flags with their observed mode and provenance. Publish no
sample from that observation.

## Comparator and pin evidence

Alert mode (T/nA=0) latches HIGH on a result strictly above THIGH and LOW on a
result strictly below TLOW. Equality with a limit does not set its corresponding
flag. Returning in range leaves a latched event intact. CONFIG acknowledges
the flags. A later out-of-range
conversion can assert them again even if temperature never re-entered the
window. Software retains returned events until explicit bus-silent consumption.
A later zero cannot erase a pending event. Store the read source, generation
and any uncertainty. When events span generations or modes, mark their
aggregate provenance mixed.

Across a change of T/nA, retain already captured alert events with their
original mode and generation. Interpret subsequent CONFIG flags under the
observed mode. An ambiguous transition leaves attribution uncertain. The
sources specify each comparator mode separately and leave the transfer of
old flag state between modes unspecified.

Therm mode (T/nA=1) sets HIGH above THIGH, clears it below TLOW and holds its
prior level in between, including equality. LOW reads zero. CONFIG reads do
not acknowledge this level. With therm mode maintained and no reset, aborting
an incomplete conversion in SD leaves HIGH at its last comparison. On TMP117
and TMP119 the comparison includes averaging. TMP116 p.14 describes comparison
at conversion end and p.11 specifies TEMP updates at block end. A TMP116
comparison at averaged-block end is an inference requiring physical evidence.
[TMP116 pp.11, 14–16](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=14),
[TMP117 pp.15–17](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=15),
[TMP119 pp.17–19](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=17).

DR/Alert=0 routes threshold alerts in alert mode and the HIGH level in
therm mode. DR/Alert=1 routes DATA_READY in either mode. At POL=0 the
open-drain output sinks on assertion. At POL=1 it releases on assertion and
the board's pullup supplies high. The eight pin-route vectors give both
assertion and acknowledgement observations. Therm pin persistence through SD
with unchanged routing and polarity is inferred. A GPIO level alone is
insufficient evidence of a completed owned shot. TMP117 and TMP119 one-shot
prose mentions an ALERT flag, while the CONFIG map defines DATA_READY and
the two threshold flags. Keep completion and threshold evidence independent.
A third-party SMBus alert response can deassert the winning threshold-alert
pin in alert mode. The timing diagrams show LOW still set after this
acknowledgement. The next CONFIG read captures LOW and clears the latch.
The library issues no alert-response traffic.
[TMP116 pp.15–16, 21, 26](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=15),
[TMP117 Figure 7-5 and pp.15–17, 21–22, 27](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=16),
[TMP119 Figure 7-6 and pp.17–19, 24, 34](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=18).

## Partial writes and remaining unknowns

Limits, CONFIG and supported managed offset occupy separate registers.
Validate the requested therm pair before I²C. Apply it in guarded SD.
For example, raising [TLOW, THIGH] from [0, 10] to [15, 20] °C permits
THIGH first, giving intermediate [0, 20]. A subsequent ambiguous TLOW write
leaves the pair unknown and the desired settings dirty. End the attempt.
Ordered writes reduce exposure to an inverted pair, but provide no atomic
commit or glitch-free comparator guarantee. Explicit recovery must establish
the guard and verify the complete owned profile.

On TMP117/M/N and TMP119 the device adds signed offset after linearization
and saturates TEMP to the signed endpoints. Preserve unmanaged calibration.
On TMP116/N, 0x07 is EEPROM3 and an offset request rejects before I²C.
[TMP116 p.30](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=30),
[TMP117 p.31](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=31),
[TMP119 p.38](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=38).

| Unknown silicon behavior | Boundary for implementations and fixtures |
|---|---|
| Same-value CONFIG writes and mid-block AVG/CONV changes | Do not assume restart, continuation, or a fixed number of results to discard. Use the guarded SD/apply/verify boundary. |
| MOD during/after OS and writing 11 during an active OS | Match stable fields with owned-trigger provenance. Establish SD and a fresh baseline for each new trigger. |
| DATA_READY and alert-mode flags on entry to SD | Consume observed old readiness and retain actual threshold events. Therm HIGH follows its comparison rule. |
| Inverted therm limits and offset ordering relative to the comparator | Refuse an inverted target therm pair. Test offset saturation independently of threshold crossings. |
| Internal rounding of a fractional block mean | Use exactly representable block means in fixtures. Host rounding of a requested limit is a separate codec decision. |
| Read-clear edge and TEMP coherence during simultaneous update | Preserve possible missed events and ambiguous consumption. Do not add silent retries or infer a byte latch. |
| Effects of an interrupted word write or an addressed reset | Preserve uncertainty, invalidate affected trust and verify before resuming managed acquisition. |

Stages 16–23 and 32 consume the applicable vectors. Distinguishing the
unresolved silicon cases requires exact-model vendor clarification or
authorized physical observations. A host fixture can verify the driver's
response to each admissible observation.
