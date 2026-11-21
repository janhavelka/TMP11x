# Prompt 9 — Cooperative ownership, timing and results

## Phase a, 2026-11-21

Stage 9 is eligible. The clean baseline was `main` at
`193f8f3bca62231e1ff06bddcda80a93ac55c135`. The preceding stage 8
closure commit matches the parent, message and three paths expected from
the handoff and wrapper request. The wrapper verified upstream equality at
20:00:42 Prague time. At entry the index was empty and the local
`HEAD...origin/main` count was 0/0. Neither the stage 9 report nor
`docs/owner-operations.md` existed.

Phase b owns `docs/owner-operations.md`, the `owner-contract` artifact.
Stage 46 consumes it alongside the stage 8 API before production code
starts. Stages 18 and 21 later implement the shared register steps, 28
implements admission and retained results, and 29–30 connect real jobs.
[Stage requirements](../prompts/9_owner-deadline-contract.md),
[sequence](../prompts/SEQUENCE.json), [handoff inventory](../prompts/HANDOFFS.md).

### Inputs verified at the baseline

| Artifact | Evidence that constrains stage 9 |
| --- | --- |
| 8 · `api-contract` | The operation-gate table freezes bound INITIALIZE, immutable binding, the three read kinds, output preservation and BUSY on lifecycle calls while a job or terminal result owns the slot. Architecture assigns one callback per register step. [API contract](../api-contracts.md), [architecture](../architecture.md), [acceptance](prompt_8.md). |
| 6 · `behavior-contract` | Ready evidence belongs to a generation or an owned trigger. The first one-shot poll is paced from trigger completion, and continuous polls from their baseline or previous poll. The literal vectors include late destructive reads and missed readiness. [Device behavior](../device-behavior.md), [vectors](../registers/behavior-vectors.json), [acceptance](prompt_6.md). |
| 7 · `safety-policy` | Each mutation or fresh acquisition needs startup proof and its own locked/idle word at `0x04`. EUN refuses immediately. Timed work can pace BUSY rereads within budget. Untimed try-fresh stops on BUSY. Failure of the transfer ends the attempt. [Safety policy](../eeprom-and-reset-policy.md), [acceptance](prompt_7.md). |

### Source checks

Two read-only source checks covered peer implementations and TI silicon.
The selected peer files matched the [stage 1 source pins](../library-comparison.md).
The following revisions distinguish the clean local checkouts from the
GitHub `main` tips observed through the read-only connector.

| Peer · local HEAD / GitHub tip | Paths and findings for the owner contract |
| --- | --- |
| TMP1x2 · `c91bac6` / `3da8dd9` | `src/TMP1x2Operations.cpp` and `docs/owner-operations.md` retain a terminal token, prevent token reuse across rebind and refuse admission on counter exhaustion. `test/jobs/test_jobs.cpp` exercises budget, wrap, cancellation, late publication and partial configuration writes. `src/TMP1x2.cpp` normalizes a nonterminal transport status to an I2C error with detail. TMP11x requires the hook for timed jobs, an explicit read kind and matching take before teardown. |
| OPT4001 · `f17d494` / `f17d494` | `src/OPT4001.cpp`, especially `poll` and `_captureReadinessFromFlags`, demonstrates budgeted owner turns and retaining evidence from a clearing read. TMP11x CONFIG and TEMP each have their own read effects. |
| ADS1115 · `1aab07b` / `1aab07b` | `examples/02_owner_safe_poll/main.cpp` accounts for lock and pointer time, maps known address/data NACK and timeout, and checks short reads. Check the adapter's actual timeout granularity before admitting a transfer. In `src/ADS1115.cpp`, token recycling, modifying the output token on refusal and signed-delta deadline tests conflict with the TMP11x requirements. |

The timing row and held-line prose are on pp. 6/21 of SBOS740A, 7/22 of
SNOSD82D and 9/24 of SNIS236. The 20–40 ms serial-interface timeout
(35 ms typical in prose) concerns the documented held-line condition
between START and STOP. The bus owner enforces each software callback
budget and releases the bus during conversion waits. For shutdown, CONFIG
read effects and the EEPROM guard, see respectively pp. 13–14/26/29,
14–15/27/30 and 17/34/37. The exact CONFIG clear edge, TEMP latching
during a concurrent update and MOD readback after a trigger remain open.
[TMP116/N](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=6),
[TMP117/M/N](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=7),
[TMP119](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=9).

### Phase-b brief and acceptance cases

| Decision to document | Trace or boundary to check |
| --- | --- |
| **Admission and ownership.** Tabulate `startInitialize`, `startConfigure`, `startRecover` and `startRead` by binding, initialized/trusted state and read kind. Check all binding fields, supported kind, target and deadline before assigning a token or copying the request into the slot. | Refuse a changed address, callback/context, clock or transport policy with the caller's token intact and zero transfers. During ACTIVE and RESULT_PENDING, probe, identity, CONFIG/TEMP, EEPROM, rebind, end, unbind and invalidation return BUSY. Passive getters remain available. |
| **Clock requirements and finite work.** List each API, including clockless binding, local queries, identity, bounded diagnostics and latest. Untimed continuous try-fresh gets at most guard + CONFIG + TEMP and never waits. Startup/apply/recover/reset waits, blocking acquisition and every job require the hook. Age requires a valid acquisition timestamp in the same epoch. | A missing required hook refuses locally. Time zero is a valid timestamp. Vary the caller's `nowMs` independently of the hook. Specify numeric CPU-step and work caps and the exhaustion result for blocking and poll paths, including a non-advancing clock, repeated equal ticks and paced guard rereads. |
| **Deadline and callback accounting.** Use unsigned elapsed differences, positive durations through `INT32_MAX`, a whole-job budget rounded down to whole ticks, and expiry at equality. Resample before and after every callback. Clip the callback budget to the lesser of the per-transfer cap and remaining job time. Account for the owner's clock continuity and service-gap obligations. | Start near 32-bit wrap. Test zero, a sub-tick budget, the maximum duration and one above it. On a 10 ms floor clock, FIRST_ACK at tick 50 permits the 3 ms wait to finish at tick 70. A due time equal to expiry permits zero callbacks. A physically successful callback returning at or after expiry retains its status and effects under `OPERATION_TIMEOUT`. |
| **Per-turn budget and mux.** Specify `poll(nowMs,maxTransfers)` for zero, one and larger budgets. Count a combined pointer/word read as one callback and a word write as one. Serialize route selection and the sensor transfer in one application-owned critical section. Charge mux traffic separately to the board bus budget. Maintain exclusive access to the same sensor across dependent steps. | Trace a one-read turn followed by a one-write turn, an interleaved second sensor on another route, and a zero-budget turn reaching expiry. Release the lock after each callback. No sleep, retry, reentry or retained callback buffer. |
| **Result and token lifetime.** Specify free, active and retained transitions for poll/cancel/take, including wrong token, double take and terminal poll. Set token scope per issuing instance, define exhaustion/wrap across rebind and document what the caller must retain to identify the issuing instance. | Start, cancel and matching take use zero I2C. A rejected second start, stale token, repeated cancel or teardown cannot replace a terminal record. Retrieval of a matching failed job succeeds once and returns its original operation status. Test the final allocatable token and the next admission. |
| **Partial effects and publication.** Keep physical callback status/count, dirty target and observations separately from job status and last good sample. Preserve a proposed configuration from its first mutating attempt. Retain returned CONFIG flags, sticky alert provenance and possible loss on ambiguous CONFIG/TEMP reads. | Cancel immediately before and after the first write. Inject an ambiguous write, a late successful final CONFIG readback and a late TEMP success. Check health, desired target, dirty/initialized state, caller output and cache separately. A later zero CONFIG cannot erase captured alerts. |
| **Owned one-shot.** Require initialized, verified, desired shutdown, then locked/idle guard, stable CONFIG comparison, guarded SD write, stale-ready CONFIG baseline, exactly one trigger, paced ready observation and TEMP. | Drift or unowned MOD=11 stops before any CONFIG write. After a successful trigger, including a late one, retain owned MOD=01/11 provenance across timeout/cancel. An ambiguous write leaves uncertain effects. Reconciliation needs an explicit new boundary. Neither MOD, elapsed time nor a cached sample proves fresh conversion. |

Phase b should express these cases as short pseudocode or fake-transfer
traces with expected counts and outcomes. Phase c independently reviews
ambiguous writes and operation-result lifetime against the authored
contract. Coverage rows F04, F14 and F18 tie the cases to silicon timing,
the public jobs and bus ownership. [Engineering contract](../prompts/ENGINEERING.md),
[coverage rows](../prompts/FULL_COVERAGE.md).

### Phase-a verification

Python 3.12.10 passed `python -B scripts/check_promptset.py`,
`python -B scripts/check_references.py` (14 pinned PDFs) and
`python -B tools/check_register_ledger.py` (six BOM models). Git
2.52.0.windows.1 was used for the history, status and whitespace checks.
No phase-a blocker. The owner contract and its acceptance traces remain
open for phase b. Native driver execution, MCU links, remote CI and
physical qualification were NOT RUN.

## Phase b delivery, 2026-11-21

The phase-a request named only this report. Git history confirms the requested
message and path at `14fcdbd6cf65d78e54892cfde35e1acd0bf07653`, parent
`193f8f3bca62231e1ff06bddcda80a93ac55c135`. The wrapper recorded upstream
equality at 20:15:36 Prague time. Phase b started at that clean HEAD on
`main` with zero divergence from the local `origin/main` and an empty index.
The stage 6, 7 and 8 inputs above were present. Stage 9 sits between 8 and
46 in the manifest.

The [owner contract](../owner-operations.md) supplies the `owner-contract`
handoff. Its admission and slot tables specify the four start methods, all
three READ kinds and the result of every poll/cancel/take transition.
The trace with two live instances requires matching both issuer and serial.
The last serial is usable, exhaustion returns `WORK_LIMIT`, and taking a
matching failed operation succeeds exactly once.

The clock table distinguishes untimed, finite-transfer calls from timed
jobs and blocking wrappers. Phase b selected 32 dispatches per poll, 256
per timed operation and 1,000,000 unsuccessful same-tick wait checks. The
last allowed step can succeed. A further required step beyond the operation
limit gives `WORK_LIMIT`. Each successful BUSY guard observation starts a
minimum 1 ms interval before another guard read. Polling caps and pacing
are library policy. Each callback receives at most the remaining budget.

Two read-only engineering checks informed these decisions. One compared
the time and work bounds with the API, behavior vectors and TMP1x2 blocking
loops. The other traced admission, cancellation and partial effects through
the stage 8 API and the stage 28–31 requirements. Both finished in phase b.
Phase c owns independent review of the authored contract.

| Focused check | Phase-b evidence |
| --- | --- |
| `python -B scripts/check_promptset.py` and `python -B scripts/check_references.py` | PASS with Python 3.12.10. All 55 stages and 14 pinned PDF hashes passed. An inline manifest check confirmed stage 9's three inputs and one output. |
| `python -B tools/check_register_ledger.py` and `python -B tools/test_check_register_ledger.py` | PASS for six selected BOM models and all 31 ledger regressions. |
| `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -` | PASS with GCC 15.1.0. Compiled the published deadline/wait example with its six static assertions and 73 generated assertions for unsigned elapsed time and floor-clock minimum waits across seven resolutions. Separate arithmetic checks covered admission at 0, sub-tick, one tick, maximum, maximum plus one and due-equals-expiry. |
| Inline Python vector and PDF checks | PASS. Matched the seven A callbacks in the two-route trace to the guard, preflight, SD word, cleared baseline, single OS word, returned HIGH/ready and 25 °C TEMP. Checked the serial timeout table and held-line prose on the six pinned pages listed above. |
| Markdown and Git | PASS. Both documents are UTF-8 and all 26 local links resolve, including section and PDF page anchors. Whitespace checks covered the new file, working diff and empty staged diff. |
| Named failure traces | The contract gives expected outcomes for a changed binding, foreign/stale token, zero-budget expiry, pre-/post-write cancel, ambiguous trigger, late final CONFIG and missed destructive-read evidence. These are inputs to future tests. |

Stage 46 audits the two design contracts. Stages 18/21 implement the shared
steps, 28 implements the slot and 29–31 exercise real jobs and fault cases.
No blocker for phase c. Native driver tests, MCU links, remote CI and
physical qualification were NOT RUN. The production driver and CI workflow
are later-stage artifacts.
