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
