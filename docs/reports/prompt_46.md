# Prompt 46 — audit of the public API and cooperative owner contracts

## Phase a — scope and inputs, 2026-11-21

Both required inputs are available. The audit asks whether one application
owner can implement the public API across all six BOM models and obtain an
unambiguous outcome when a transfer consumes flags, partially writes a
register, or returns at the deadline. Stage 10 consumes the finished
`audit-8-9` report before production code begins. The audit remains open.
[Original prompt 8](../prompts/8_api-architecture.md),
[original prompt 9](../prompts/9_owner-deadline-contract.md),
[audit mandate](../prompts/46_audit-8-9.md), [handoffs](../prompts/HANDOFFS.md).

The clean entry revision on `main` was
`cab9a08836c6d3b91c0b8f9702911c7eedc03fbd`, with zero divergence from
local `origin/main`. Its parent `a6a18bd`, subject and three paths match
the stage 9 closure request. The wrapper recorded upstream equality at
21:00:24 Prague time. Stages 8 and 9 have individual COMPLETE rows.
[Sequence](../prompts/SEQUENCE.json), [progress ledger](../prompts/PROGRESS.md).

| Input | Revisions and material to review |
| --- | --- |
| Stage 8 · `api-contract` | Review `521752a..193f8f3` for the original surface, register ownership, two shared-bus sketches and 46 R/F/X mappings. Then include `193f8f3..cab9a08`. Stage 9 amended the API at `a6a18bd` when it clarified clock wrap, readiness and instance identity. Architecture last changed at `125fb76`. [API](../api-contracts.md), [architecture](../architecture.md), [stage 8 acceptance](prompt_8.md). |
| Stage 9 · `owner-contract` | Review `193f8f3..cab9a08`. The admission and slot tables, clock arithmetic, work limits, register steps and eleven acceptance traces are present. Reconcile the current owner table with the public operation gates for the same call and state. [Owner operations](../owner-operations.md), [stage 9 acceptance](prompt_9.md). |

### Source inventory

Two parallel read-only checks located the peer and silicon inputs. All 23
selected peer files match their source pins in the clean local checkouts.
GitHub tips below identify branch revisions observed at entry. The stage
reports above name the implementation paths and test cases in detail.

| Repository | Source pin / local HEAD / GitHub tip | Selected paths |
| --- | --- | --- |
| TMP1x2 | `afd5088 / c91bac6 / main@3da8dd9` | `include/TMP1x2/{Config,Status,TMP1x2}.h`, `src/TMP1x2*.cpp`, `docs/owner-operations.md`, core, helpers and jobs tests |
| OPT4001 | `441ef57 / f17d494 / main@f17d494` | `include/OPT4001/{Config,OPT4001}.h`, `src/OPT4001.cpp`, `test/test_basic.cpp` |
| ADS1115 | `e07622a / 1aab07b / main@1aab07b` | `include/ADS1115/{Config,ADS1115}.h`, `src/ADS1115.cpp`, `examples/02_owner_safe_poll/main.cpp`, `test/test_basic.cpp` |
| FieldCore-node | `c40ea83 / c40ea83 / rodtester@453db51` | `include/TunnelMonitor/i2c/I2cOwnerTransport.h`, `src/devices/coil_head/CoilHeadModule.cpp`, `test/host/rodtester/coil_head_module.cpp` |

| Pinned TI source | Held-line timeout table / prose | Register map / CONFIG / EEPROM guard / pointer 0x07 / ID |
| --- | --- | --- |
| [TMP116/N · SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24) | pp. 6 / 21 | pp. 24 / 26 / 29 / 30 / 31 |
| [TMP117/M/N · SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25) | pp. 7 / 22 | pp. 25 / 27 / 30 / 31 / 32 |
| [TMP119 · SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32) | pp. 9 / 24 | pp. 32 / 34 / 37 / 38 / 39 |

The source check opened 32 selected pages. Eleven literal behavior traces
and nine safety transcripts are available for comparison. Preserve the
TMP116 POR BUSY conflict, the unknown CONFIG clear edge and TEMP byte
latching, MOD readback around one-shot, offset/comparator ordering,
addressed-reset effects and the TMP117 DATA_READY routing discrepancy.
[Variant contract](../variants.md), [register ledger](../registers/tmp11x.json),
[device behavior](../device-behavior.md), [vectors](../registers/behavior-vectors.json),
[safety policy](../eeprom-and-reset-policy.md).

### Audit brief

| Phase-b comparison | Evidence to record |
| --- | --- |
| Public surface and model gates | Walk R01–R18, F01–F20 and X01–X08 from each applicable family map to the typed call, job, diagnostic or application owner. Trace TMP116/N refusal of offset and addressed reset, supported unmanaged versus explicitly zero offset, and ID mismatch during a dump. Check the bind-only observation path, core/package boundaries and the two consumer sketches. [Coverage inventory](../prompts/FULL_COVERAGE.md), [feature matrix](../feature-coverage.md). |
| Lifecycle, replay and acquisition | Trace FIRST_ACK, the operation-local lock/busy guard, immutable binding, both signed limits and trust at the first attempted write. For latest, continuous fresh and owned one-shot, record the exact CONFIG/TEMP effects, retained alerts, sample provenance and cache outcome. Compare preflight drift before SD, drift after the trigger, ambiguous transfer and late success. [Engineering contract](../prompts/ENGINEERING.md). |
| Scheduling and result ownership | Trace zero- and one-callback turns, two mux routes, coarse ticks across wrap, a due time equal to expiry and a callback that returns late. Check callback timeout clipping and work caps. Follow cancellation on either side of the first write, a foreign or stale token, serial exhaustion and matching take of a failed result. Record physical status separately from job status and show which calls preserve the pending result. [Owner traces](../owner-operations.md#acceptance-traces). |

Phase b records the claim, source page or code path, observed outcome and
disposition for each discrepancy. In phase c, reread both original prompts
and run two independent read-only reviewers in parallel. One covers the
public surface, model gates, replay and destructive reads against TI and
the ledgers. The other covers clocks, transport ownership, cancellation
and result lifetime against the peer implementations and traces. The lead
verifies findings, owns any corrections and repeats affected checks. Phase d
repeats the originating checks, including coverage mappings, register and
transfer traces and C++17 deadline assertions, before the audit decision.
[Review procedure](../prompts/DELEGATION.md), [verification](../prompts/VERIFICATION.md).

### Phase-a checks

Python 3.12.10 passed `python -B scripts/check_promptset.py` (55 stages),
`python -B scripts/check_references.py` (14 pinned PDFs),
`python -B tools/check_register_ledger.py` (six BOM models) and
`python -B tools/test_check_register_ledger.py` (31 regressions).
An inline Python inventory confirmed the 8, 9, 46, 10 queue order, both
completed prerequisites, all 46 architecture rows, eleven owner traces
and 25 local report links. Git 2.52.0.windows.1 passed whitespace checks
for the new report, working diff and empty index.
There is no phase-b input blocker. Native driver tests, MCU links, remote
CI and physical qualification were NOT RUN. The two source inventories
leave the phase-c independent review outstanding.

## Phase b — engineering evidence, 2026-11-21

The audit found one missing public operation. The shared CLI needs to
inspect the current job without advancing it. A zero-transfer poll can
expire an active job, changing both the reported state and what a later
cancel returns. The corrected contracts give the owner a passive
`getOperationSnapshot() const` query on the existing slot.

Phase b began on clean `main` at
`73097aa6cac42388a0c1f2e1e862df089c2a6623`. Its parent, subject and
sole path matched the phase-a commit request. The wrapper recorded upstream
equality at 21:15:36 Prague time. Local divergence at entry was 0/0.
The source comparisons use the stage 8 and 9 revision ranges above. Two
read-only contributors checked the model mappings and the scheduling traces
in parallel. The phase-c independent review remains due.

### Public surface and model gates

| Requirement and source | Trace outcome and disposition |
| --- | --- |
| R01–R18 against the three pinned register maps and field pages listed above | All 18 rows have an owner. R01–R04 cover TEMP, flags and BUSY. R05–R12 cover CONFIG, the command bit and both limits. R13–R18 cover the guard, shadows, offset, identity and pointer validation. TMP116/N accepts the full ID `1116`, reads four shadows at `05/06/07/08` and rejects typed offset (including zero) and addressed reset before any callback. TMP117/M/N and TMP119 accept low twelve bits `117`, retain revision and read shadows at `05/06/08`. At `07`, unmanaged replay observes and preserves a value such as `0040` (+0.5 °C). Explicitly managed zero writes and verifies `0000`. Only the supported parts admit guarded CONFIG bit 1. The model gates agree. |
| F01–F20 and X01–X08 against the [architecture](../architecture.md), [feature matrix](../feature-coverage.md) and [coverage inventory](../prompts/FULL_COVERAGE.md) | All three documents contain the same 46 distinct IDs. Each included row reaches a typed call, job or named integration owner. F14 needed the passive query described below. The eight exclusions cover persistence and shadow writes, broadcast, ARA, raw access, board ownership, provenance claims, foreign families and undocumented features. |
| Bind-only observation and coexistence, stage 8 diff and TMP1x2 `afd5088` public headers | Bind costs zero callbacks. On a selected TMP117, a dump returning `1116` at `0F` stops after one neutral callback. The partial snapshot includes the raw incompatible ID, live trust is invalidated, and the dump never reaches TEMP or CONFIG. A complete full dump costs ten reads. An EEPROM dump costs six on TMP116/N and five on the other models. The two bring-up sketches share one application bus at addresses `48` and `49` hex and return each package's own status. The observation-only path ends bring-up at bind. The portable headers, framework adapters and vendor documents have distinct package roles. |

### Replay and acquisition

Hex words below come from the [behavior vectors](../registers/behavior-vectors.json),
[guard and recovery transcripts](../eeprom-and-reset-policy.md#transcript-vectors-for-implementation)
and the [stage 9 acceptance traces](../owner-operations.md#acceptance-traces).
The CONFIG flag tables are on SBOS740A p.26, SNOSD82D p.27 and SNIS236 p.34.

| Requirement | Outcome against the public and owner contracts |
| --- | --- |
| Startup and operation-local guard | A compatible ID completing at 50.9 ms on a 1 ms floor clock permits the first guard at tick 54. Prior bind time and a cancelled operation's unfinished origin earn no wait credit. Independently verified `0x04` mask `C000` accepts `0000/3FFF`. `4000` permits a paced reread only on a timed path. `8000/C000` stops guarded work immediately. A failed guard ends the attempt. A later CONFIG.BUSY requires a renewed guard for timed work. The TMP116 power-up prose on p.17 conflicts with pp.26/29, so BUSY clear alone cannot establish startup. |
| Binding, limits and first-write boundary | A changed route context or timeout in configure rejects before token assignment and I2C. With an accepted candidate, cancellation before the SD callback preserves the old target. From the first attempted SD write, even a data NACK retains the candidate, dirty state and physical status. For therm limits [0,10] → [15,20] °C, THIGH `0A00` first gives [0,20]. An ambiguous TLOW `0780` ends apply there. The reverse transition writes TLOW first. Recovery verifies both signed words (the recovery transcript uses `FB00` = −10 °C and `1280` = +37 °C), owned offset where applicable, and final stable CONFIG before committing a generation. |
| Latest and continuous fresh | Latest uses one tracked TEMP and consumes readiness. An initialized dirty instance can return a latest value with dirty trust. Its timestamp records observation and its freshness is false. The final apply CONFIG consumes its ready bit as the generation baseline. A later eligible diagnostic CONFIG can retain HIGH and same-generation readiness. A safe guard plus one TEMP then uses that readiness once. A later fresh conversion can return the identical `0C80`. Failed TEMP discards ambiguous readiness and preserves HIGH and cache. Failed CONFIG can also lose alert history. A matching BUSY/ready CONFIG (`B220` for desired `0220`) ends an untimed try with no TEMP. AVG drift (`A240`) additionally latches dirty. |
| Owned one-shot, drift and lateness | At desired SD `0620`, guard then CONFIG `0640` or unowned `0E20` ends preflight with zero writes. On the accepted route, guard, preflight and explicit SD precede the SD baseline and one `0E20` trigger. A timely poll `AE20` captures HIGH and new ready, then TEMP `0C80` publishes 25 °C. Substitute `AE40` and the changed AVG stops callback six with dirty trust, HIGH retained, zero TEMP reads and the prior cache. A late physical trigger success retains owned MOD provenance and returns timeout. An ambiguous trigger leaves dirty uncertainty. A late TEMP consumes readiness and preserves the prior sample's generation, trust, offset and timestamp. |

### Scheduling and result ownership

| Requirement | Evidence and disposition |
| --- | --- |
| Bus and work budgets | Five budget-one turns on route A reach its one-shot trigger. B can read TEMP on another route between turns. A wait turn spends zero callbacks, then two further A reads complete the seven-callback trace. Each callback owns route selection through the sensor transfer and releases the bus. Mux writes are charged separately. A turn permits 32 dispatches and a timed operation 256. Step 256 can succeed. If more work remains it returns `WORK_LIMIT` before step 257. A frozen positive-budget wait fails on its 1,000,001st unsuccessful same-tick check. |
| Coarse tick across wrap | At q=10, admit at `FFFFFFF0` and complete FIRST_ACK at `FFFFFFFA`. The next two floor readings are 4 and 14. Tick 14 is the first eligible guard. A 39 ms request rounds down to 30 ms and expires at 14, so zero guard callbacks. A 49 ms request rounds to 40 ms. At 14 the guard gets 10 ms of a 50 ms callback cap. Physical OK at tick 24 counts one success for that read. The job retains `OPERATION_TIMEOUT` and sends no further transfer. FieldCore-node `c40ea83`, `CoilHeadModule::remaining` and its transfer path supply the clipping precedent. The authoritative post-callback clock remains necessary. |
| Result lifetime | With A:1 pending, B:1 fails the issuer check. After rebind, A:1 fails against A:2. Both preserve the caller's output and pending record. Repeated poll returns the saved status. Repeat cancel, unrelated I2C and teardown leave the record intact. One matching take returns `OK` with the job's failure, then a second take returns `RESULT_NOT_AVAILABLE`. After issuing `UINT32_MAX`, the next otherwise valid start returns `WORK_LIMIT` without spending a callback or changing its output token. |

**F14 correction.** At the baseline, the slot table offered only poll,
cancel and take. Inspect a LATEST request after its deadline but before
its first owner poll. A `jobs` query implemented with `poll(now, 0)`
would retain `OPERATION_TIMEOUT` and make cancel return `BUSY`. Inspection
must preserve the owner's ability to cancel. TMP1x2 at `afd5088` provides
a passive getter in `src/TMP1x2Operations.cpp` with an admission/result
test in `test/jobs/test_jobs.cpp`. The TMP11x query copies only slot
state, occupied identity/kind, READ selector and any terminal status.
It takes zero clock readings and zero transfers. The new trace compares
two inspections followed by cancel with the same inspections followed by
a zero-budget poll. The first retains `CANCELLED`, the second timeout.
Stage 28 owns the slot and its test. Stage 37 already consumes
`job-lifecycle` and owns the passive command and fixture transcript.
[Public API](../api-contracts.md#operation-gates),
[slot contract](../owner-operations.md#slot-and-result-lifetime),
[CLI contract](../prompts/CLI_CONTRACT.md),
[stage 28](../prompts/28_cooperative-admission.md),
[stage 37](../prompts/37_cli-baseline.md).

### Phase-b checks

- Python 3.12.10 passed `python -B scripts/check_promptset.py` (55 stages),
  `python -B scripts/check_references.py` (14 pinned PDFs),
  `python -B tools/check_register_ledger.py` (six models), and
  `python -B tools/test_check_register_ledger.py` (31 regressions).
- Inline `python -B -` compared all 46 rows across the three maps, the
  existing 28→37 artifact edge, the new snapshot rows and acceptance cases,
  five guard words, signed limit ordering, seven shot callbacks, two drift
  outcomes and the retained diagnostic readiness vector.
- GCC 15.1.0 passed the read-only timing check with seven published and
  296 generated C++17 assertions across seven resolutions. The lead
  compiled the published example again with four assertions for the
  coarse-clock trace above, using
  `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -`.
- Git 2.52.0.windows.1 passed working and empty-index whitespace checks.
  Native driver tests, MCU links, remote CI and physical qualification
  were NOT RUN. Production code starts at stage 10. The silicon evidence
  gates recorded in phase a remain open.

## Phase c — independent review, 2026-11-21

Review began on clean `main` at `2c7572989577cc1d38d579139633b6e9ec2a15f7`.
The parent, subject and eight paths match the phase-b request. The wrapper
recorded upstream equality at 21:34:13 Prague time, and local divergence
was 0/0. The lead and two parallel read-only reviewers reread prompts 8
and 9 and checked the design diffs through that revision. Both reviewers
then checked the corrected working diff.

| Finding | Verified outcome and implementation owner |
| --- | --- |
| A therm-to-alert transition could invert the limits before the comparator mode changed. | Starting in therm at [0,10] °C, the accepted alert target [20,10] needs TLOW `0x0A00`. Stages 18/22 must first verify SD/alert (`0x0620` in this trace). Failed, mismatched or late readback permits zero inverting writes. Returning to therm requires verified TLOW `0x0000` and THIGH `0x0500` first. These are software ordering requirements. The silicon response to inverted therm limits remains unknown. [Apply contract](../api-contracts.md#startup-apply-and-reset), [literal vectors](../registers/behavior-vectors.json). |
| A changed unmanaged offset could leave old readiness eligible. | Under verified continuous operation, retain ready at offset zero, then observe `0x0040` with no known cause. The word is evidence of changed calibration. Its scope is unknown, so the existing external-interference rule invalidates live state and the next managed fresh request spends zero callbacks. Explicit recovery preserves the +0.5 °C calibration and consumes stale readiness at its baseline. A later conversion can use that calibration. Historical samples keep their original provenance. Stage 23 tests the typed observation and stage 26 the diagnostic route, including TMP116/N's EEPROM3 exception. [Public observation rules](../api-contracts.md#operation-gates), [interference policy](../eeprom-and-reset-policy.md#external-interference). |
| A late TEMP had no raw-word field in the retained result. | Seed the cache with `0x0D00`. TEMP `0x0C80` at equality gives physical OK, terminal timeout, and a `lastRead` with pointer zero and valid tick 4. The prior cache survives. A short read at the same tick retains physical failure and possible readiness loss. Expiry before dispatch has zero callbacks. Neither case supplies a raw word. The existing slot holds one optional register observation. The owner may inspect status repeatedly and take the record once. Stages 28, 30 and 31 own the slot and real-transfer tests. [Slot and traces](../owner-operations.md#acceptance-traces). |
| Stage 30 described LATEST as continuous-only and applied fresh-read gates to it. | An initialized shutdown target left dirty by a failed settings write still admits LATEST. One timely TEMP yields a sample with dirty trust and `freshConversion=false`. Fresh and one-shot retain their clean-profile gates. [Corrected stage 30 acceptance](../prompts/30_cooperative-measurement.md). |

The silicon reviewer checked model gates, replay, read effects, the two
consumer sketches and the 46 coverage rows. The lead rechecked T/nA, the
signed limits and pointer `0x07` in SBOS740A pp.26/28/30, SNOSD82D
pp.27/29/31 and SNIS236 pp.34/36/38. The supporting parts add the offset
after linearization. A later offset read cannot establish which conversion
produced the stored TEMP. [Pinned sources](../reference/manifest.json).

The owner reviewer checked the single slot, cancel versus zero-budget poll
at unobserved expiry, foreign and exhausted tokens, per-callback bus
ownership, budget clipping and late results. Peer comparisons included
TMP1x2 `afd5088` (`src/TMP1x2Operations.cpp` and `test/jobs/test_jobs.cpp`),
OPT4001 `441ef57`, ADS1115 `e07622a` and FieldCore-node `c40ea83`
(`CoilHeadModule::remaining`, `transfer` and the overrun tests). The
selected peer checkouts were clean and the four GitHub branch tips still
matched phase a. Stage 28 supplies the passive slot query and stage 37
consumes it through the existing `job-lifecycle` dependency.

| Phase-c check | Result |
| --- | --- |
| `python -B scripts/check_promptset.py`, `python -B scripts/check_references.py`, `python -B tools/check_register_ledger.py`, `python -B tools/test_check_register_ledger.py` | PASS with Python 3.12.10 for 55 stages, 14 pinned PDFs, six BOM models and 31 regressions. |
| Inline `python -B -` | PASS for all 46 IDs in three maps, the affected producer edges, the two limit-transition vectors and their refusal boundary, all three families' CONFIG/limits/offset-or-shadow pages, and the raw TEMP and offset values above. |
| `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -` | GCC 15.1.0 passed 71 lead assertions and 147 independent reviewer assertions, each set including the published example. The checked cases cover unsigned wrap, equality and floor-clock waits. |
| Git and documentation | Working and empty-index whitespace checks passed with Git 2.52.0.windows.1. The changed documents' local links resolve. |

Native driver tests, MCU links, remote CI and physical qualification were
NOT RUN. Phase d owns final acceptance, the report index and the progress
row. The silicon uncertainties in phase a remain open.

## Phase d acceptance, 2026-11-21

**COMPLETE.** Stage 46 delivers the `audit-8-9` handoff. The two independent
phase-c reviews are recorded above. Acceptance began on clean `main` at
`2ba9c775fea013498fd05bdcd6a464b69b47033e`, with zero divergence from the
local `origin/main`. Git confirmed the parent `2c7572989577cc1d38d579139633b6e9ec2a15f7`,
subject and all 13 paths against the phase-c commit request. The wrapper
logged upstream equality at 21:50:02 Prague time. The phase-d commit and
push are pending wrapper processing.

Rechecked the [public API](../api-contracts.md), [architecture](../architecture.md)
and [owner contract](../owner-operations.md) against the [pinned TI sources](../reference/manifest.json),
[register ledger](../registers/tmp11x.json) and [literal vectors](../registers/behavior-vectors.json).
The four peer checkouts remain clean at the phase-a local revisions. All
21 peer blobs selected for this acceptance match their phase-a source pins.
The documented coexistence sketch shares an application bus at distinct
addresses and maps each adapter outcome to its package's own status.
The clock clipping, passive inspection and dirty-state precedents were used
as design evidence. The TMP1x2 register map and scheduler remain outside
the TMP11x core.

| Scope | Accepted evidence |
| --- | --- |
| Public surface, model and register ownership | Matched all 46 ordered R/F/X rows in the architecture, feature matrix and coverage inventory. The six BOM selections use three maps. TMP116/N exposes EEPROM3 at `0x07` and refuses offset and addressed reset before I²C. The other models expose offset there and support the guarded addressed reset path. ID-first incompatible dumps stop after one neutral callback. Complete register dumps use ten reads and EEPROM dumps use six or five. Observation-mode firmware can bind without settings writes. |
| Guarded replay and comparator transitions | FIRST_ACK and the operation-local lock/busy guard precede mutation. For therm [0,10] to alert [20,10] °C, verify guarded SD/alert `0x0620` before writing TLOW `0x0A00`. Failed, mismatched and late readbacks permit zero inverting writes. For the reverse transition, verify TLOW `0x0000` and THIGH `0x0500` before entering therm. A partial write leaves the candidate dirty for explicit recovery. |
| Acquisition and destructive reads | Dirty, initialized shutdown admits LATEST for one timely TEMP with dirty trust and freshness false. An unexplained unmanaged OFFSET change from `0x0000` to `0x0040` preserves the observed word and historical calibration, invalidates live state and prevents a fresh read from spending any callbacks until recovery. Recovery preserves unmanaged calibration and consumes stale readiness at its baseline. The owned shot uses seven callbacks with one trigger. AVG drift at callback six retains HIGH and consumed readiness and stops before TEMP. Failed CONFIG or TEMP discards ambiguous ready evidence and preserves historical alert observations. |
| Deadlines, bus ownership and cancellation | The twelve owner traces cover hook timing, budget clipping, mux serialization, zero-budget expiry, bounded work, and the first attempted write boundary. At wrap, 12 ms admitted from `0xFFFFFFF8` leaves 3 ms at tick 1 and expires at tick 4. Complete TEMP at expiry retains raw `0x0C80` and tick 4 as a physical success. An adapter-detected short read reports physical failure and possible readiness loss. Expiry before dispatch has zero callbacks. All three retain the timeout and the prior cache. Only the complete read supplies `lastRead`. |
| Slot and retained result | Passive snapshots do no I²C or clock sampling. Before timeout is observed, cancel can retain `CANCELLED`. A zero-budget poll can retain timeout. Unrelated I²C and lifecycle operations remain gated while active or pending. Foreign and stale tokens preserve the record and output. One matching take transfers the result, including failure, and frees the slot. |

| Check | Phase-d result |
| --- | --- |
| `python -B scripts/check_promptset.py`, `python -B scripts/test_check_promptset.py -q` | PASS with Python 3.12.10. 55 stages and 36 regressions. |
| `python -B scripts/check_references.py`, `python -B scripts/test_check_references.py -q` | PASS. 14 pinned PDFs and nine regressions. |
| `python -B tools/check_register_ledger.py`, `python -B tools/test_check_register_ledger.py` | PASS. Six BOM models and 31 regressions. |
| Inline `python -B -` source and contract checks | PASS. Checked 24 pinned pages with PyMuPDF 1.27.2.2 across the timeout table/prose, maps, CONFIG, limits, guard, pointer `0x07` and ID. Checked all three coverage maps, the 8/9/46/10 sequence and dependencies, both limit alternates, the seven-callback shot and the retained read and slot traces. |
| `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -` | PASS with GCC 15.1.0. Compiled 150 published and generated assertions for unsigned wrap/equality and minimum waits. Inline Python also checked admission rounding and the coarse-clock service bound. |
| Local Markdown and Git | PASS. Checked local links and anchors in the closure files, the committed phase-c diff, working diff and empty index with Git 2.52.0.windows.1. |

Stage 10 consumes this report with the stage 8 and 9 contracts, the tooling
plan and the register validator. Native driver tests and MCU links were
NOT RUN. Remote CI was NOT RUN under the stage 43 checkpoint policy.
Physical qualification was NOT RUN and the fixture is unconfirmed. The
phase-a silicon uncertainties remain open, including TMP116 POR BUSY,
the CONFIG clear edge, TEMP byte latching, one-shot MOD readback,
offset/comparator ordering, addressed-reset effects and TMP117 DATA_READY
routing. None blocks this design audit.
