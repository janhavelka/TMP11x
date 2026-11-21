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

## Phase c review, 2026-11-21

Reviewed the phase-b diff at `b3ae08de268819dd72e64f6cab627cbccda107e5`
on clean `main`. Its parent, message and two paths matched the preceding
commit request. The wrapper recorded upstream equality at 20:35:51 Prague
time, and the local tracking ref still matched at review entry. Two
independent read-only reviewers covered timing/work bounds and partial
effects/result ownership. The lead checked their findings against the
stage 6 vectors, stage 7 guard, stage 8 API and stage 28–31 requirements.
The peer paths, clean local revisions and GitHub tips matched the phase-a
source table.

| Finding | Correction and review evidence |
| --- | --- |
| A physical gap shorter than one wrap can still alias on a coarse clock. | At q=2, physical times 1 and 4294967296 ms both return zero. A naïve check leaves all 10 ms of a job available. Deadline and cache-age subtraction require an unwrapped **floor-reading** difference below one wrap. A physical gap of at most `2^32 - q` ms suffices. For pacing from an older same-epoch baseline, the low-word difference is a conservative credit toward a minimum wait. |
| A ready bit could survive final apply or an ambiguous read as credit for a later acquisition. A post-trigger poll also needed an explicit stable-field comparison. | Final apply consumes readiness as its generation baseline. A failed CONFIG or TEMP discards ambiguous readiness. In the revised one-shot trace, `AE40` carries HIGH, ready and owned MOD=11, but its AVG differs from desired `0620`. The sixth callback latches dirty and ends with `CONFIG_MISMATCH`, zero TEMP reads and the prior cache intact. |
| Copying or assigning a device could duplicate or orphan its result slot. Consuming instance alerts could erase evidence needed by a pending result. | The API and owner contract keep the device at a stable identity and disallow copy/move construction and assignment. Tokens and results remain values. The retained record carries the job's own read effects independently of the alert consumer. The stale-token trace now attempts retrieval while a newer result is pending after rebind. |

The reviewers found the first-mutating-attempt rule consistent across
configure, cancel and callback failure. A pre-write cancellation preserves
the prior target. An ambiguous first write retains the candidate and dirty
evidence. A physically successful late trigger retains its provenance.
The slot table preserves the terminal result on a foreign token, repeated
poll/cancel or teardown. Only a matching take frees it. Stage 28 can
check the device's four copy/move type traits alongside its slot tests.

| Focused verification | Phase-c result |
| --- | --- |
| `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -` | GCC 15.1.0 passed seven published and 273 generated assertions for remaining time and floor-clock waits, including q=1 and `INT32_MAX`. Inline Python reproduced the two wrap aliases at q=2 and 10 and checked sub-tick admission, maximum duration and due-equals-expiry. |
| Inline `python -B -` timing and work traces | 7,728 small-clock cases checked the service bound and conservative wait credit at half-tick phases and multi-wrap gaps. Six dispatch traces checked transfer budgets 0/1/255, a 33rd step in a new turn, success on step 256 and refusal of step 257. The contract now gives a terminal outcome when the turn and job caps coincide. |
| Inline `python -B -` register and PDF checks | Matched the seven-callback TMP117 sequence to the stage 6 vector, decoded the AVG drift, HIGH, ready and 25 °C, and rechecked the timeout table and held-line prose on the six phase-a PDF pages. |
| Repository checks | Python 3.12.10 passed `python -B scripts/check_promptset.py`, `python -B scripts/check_references.py`, `python -B tools/check_register_ledger.py` and `python -B tools/test_check_register_ledger.py` for 55 stages, 14 pinned PDFs, six BOM models and 31 regressions. Local Markdown targets, anchors and PDF pages resolve in both contracts and this report. Git 2.52.0.windows.1 passed working and empty-index whitespace checks. |

The checked artifacts are the owner contract, its companion API
clarifications and this report. These are design traces. Native driver
tests, MCU links, remote CI and physical qualification were NOT RUN.
Phase d owns final acceptance, the index and the progress ledger. Stage 9
remains open with no phase-c blocker.

## Phase d acceptance, 2026-11-21

**COMPLETE.** The `owner-contract` handoff is [Cooperative owner
operations](../owner-operations.md), paired with the [API contract](../api-contracts.md).
Acceptance started on clean `main` at `a6a18bdc1c9ce1767f39fe3ddec30c43d2de8f68`.
Git confirmed the requested phase-c message, three paths and parent
`b3ae08de268819dd72e64f6cab627cbccda107e5`. The wrapper recorded a successful
commit and verified `main` equals `origin/main` at 20:49:09 Prague time.
The local refs still matched at entry. This closure's commit and upstream
verification are pending wrapper processing.

Rechecked the stage 6 behavior and vectors, stage 7 guard policy and stage 8
API and register ownership artifacts listed above. The three sibling
checkouts were clean at the phase-a revisions. The phase-c independent
reviews cover partial writes and retained result lifetime. The following
acceptance walk used all eleven traces and the current contracts.

| Scope | Accepted evidence |
| --- | --- |
| Admission and teardown, retained identity | All four starts and three read kinds have local gates, including unsupported operations, immutable binding and verified desired shutdown for one-shot. Start, cancel and take are bus-silent. Both occupied states block unrelated I²C and lifecycle calls. A matching issuer and serial frees the slot exactly once, including for a failed result. Foreign and stale tokens, repeated cancellation and serial exhaustion preserve existing ownership. |
| Wrap and equality, coarse clock, work and transport | The clock table separates optional observations and same-epoch age from deadline-bearing operations. Caller `nowMs` has no timing role. The hook is sampled before and after callbacks, with floor-clock budget rounding, unsigned elapsed time, timeout clipping and expiry at equality. At q=10, tick 70 is the first eligible guard after FIRST_ACK at tick 50 and no callback is allowed at expiry. Zero-budget turns still expire. The turn, job and same-tick limits bound work. |
| Two routes, drift after trigger, missing or unsafe evidence | The first five turns of A's shot spend one callback each, including distinct read and write turns. B can read between them. Each callback serializes route selection and transfer and releases the lock. Mux traffic has separate accounting. The successful shot has seven callbacks and one trigger. The drift trace stops on callback six, preserves HIGH and consumed readiness and never reads TEMP. EUN, BUSY and pre-trigger drift have explicit refusal or guard-renewal outcomes. |
| Cancellation around a write, late verification or trigger | Pre-write cancellation preserves the prior target. A partial write retains candidate intent and dirty evidence. Late physical success updates health and observations while the operation times out. Late verification cannot initialize or clear dirty. Successful trigger provenance survives timeout or cancellation. An ambiguous trigger leaves uncertain effects. |
| Continuous baseline and destructive reads | Final apply consumes readiness as the new baseline. A new eligible observation is required for fresh publication. Failed CONFIG/TEMP reads discard ambiguous readiness, preserve historical alerts and retain possible loss. Late TEMP completion leaves the last-good sample intact. Inspecting or consuming alerts preserves the job's own retained evidence. |

Coverage rows F04, F14 and F18 retain the serial timeout distinction,
the typed job mapping and the deadline, health and ownership rules.
[Coverage inventory](../prompts/FULL_COVERAGE.md).

| Check | Phase-d result |
| --- | --- |
| `python -B scripts/check_promptset.py`, `python -B scripts/check_references.py`, `python -B tools/check_register_ledger.py`, `python -B tools/test_check_register_ledger.py` | PASS with Python 3.12.10. Checked 55 stages, 14 PDF hashes, six BOM models and 31 ledger regressions. |
| Inline `python -B -` with `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -` | PASS with GCC 15.1.0. Compiled 163 published and generated deadline/wait assertions. Python checked 105 coarse-clock service cases, a wrap alias and the admission budget boundaries. |
| Inline `python -B -` register and PDF checks | PASS. Matched the one-shot words and transfer counts to the vectors and ledger, decoded the AVG drift and checked four turn budgets. Rechecked the timing table, held-line prose, CONFIG and EEPROM field pages in each pinned datasheet. |
| Local Markdown and Git checks | PASS. Checked local targets and anchors for the acceptance artifacts, the phase-d diff and empty staged diff with Git 2.52.0.windows.1. |

Stage 46 owns the next audit. Stages 18 and 21 implement the shared steps,
28 implements the retained slot, and 29–31 integrate jobs and test failure
paths. Native driver tests and MCU links were NOT RUN because driver and
firmware implementation are later work. Remote CI was NOT RUN under the
stage 43 checkpoint policy. Physical qualification was NOT RUN. The exact
CONFIG clear edge, concurrent TEMP latching and one-shot MOD readback
remain open silicon evidence. No stage 9 acceptance blocker remains.
