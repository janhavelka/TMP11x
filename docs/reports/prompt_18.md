# Prompt 18 — EEPROM-safe initialization and configuration writes

**COMPLETE.** Both public `begin` overloads and `recover` verify volatile
CONFIG and both limit words on all six supported BOM selections. The
[phase d acceptance checks](#phase-d-acceptance) passed on the host.

## Phase a — requirements and inputs, 2026-11-23

**PARTIAL.** All 13 inputs are available for phase b. The clean baseline was
`fc1d1de8f1dbd8844842ae921efaf43eb00ad3b1` on `main`, equal to local
`origin/main`. The parent `2a1a19b`, subject and four changed paths match
the prompt 47 closure request. The wrapper recorded upstream equality at
21:16:23 Prague time. The sequence places 18 immediately after audit 47.
[Stage requirements](../prompts/18_safe-initialize.md), [audit acceptance](prompt_47.md#phase-d-acceptance).

### Verified inputs at the baseline

| Consumed artifacts | Evidence and use in phase b |
| --- | --- |
| `safety-policy`, `behavior-contract`, `behavior-vectors` | The guard truth table and the `startup-anchor-and-reset`, `non-atomic-therm-limits` and `continuous-generation-change` vectors give literal timing, pointer, ordering and read-effect expectations. [Safety policy](../eeprom-and-reset-policy.md), [device behavior](../device-behavior.md), [vectors](../registers/behavior-vectors.json). |
| `api-contract`, `owner-contract` | Binding replacement, first-write intent, terminal status, floor-clock waits and work limits are specified for the shared step engine. [Public contract](../api-contracts.md#startup-apply-and-reset), [owner contract](../owner-operations.md#work-limits), [architecture](../architecture.md). |
| `binding-api`, `public-identity-api`, `transport-health` | Bind currently validates the binding before replacing it. Public probe uses a health-neutral ID read. Apply can pass a tracked transport to the same identity engine. Word writes already require startup and locked/idle evidence from their caller. [Lifecycle](../../src/TMP11x.cpp), [identity engine](../../src/Identity.cpp), [transport](../../src/RegisterTransport.h), [health tests](../../test/test_health.cpp). |
| `temperature-codec`, `config-codec`, `latest-read-engine` | Signed counts cover each entire limit word. Existing helpers validate all steady selectors and the THERM limit pair, encode canonical CONFIG, decode flags and compare registers independently. Public latest already requires managed initialization. [Signed codec](../../src/Temperature.cpp), [CONFIG codec](../../src/Configuration.cpp), [latest engine](../../src/LatestRead.cpp). |
| `register-fake`, `audit-10-17` | The fixture records addressed frames and grants, destructive reads and ambiguous writes. A successful legal write currently updates storage. Phase b needs a scripted ACK with the old word retained to exercise ignored writes. The audit closed the package and read-path prerequisites. [Register fixture](../../test/support/FakeTmp11xDevice.cpp), [audit report](prompt_47.md). |

### Silicon evidence and reference implementation

| BOM selections | Pinned TI evidence |
| --- | --- |
| TMP116, TMP116N | SBOS740A p.17 §7.5.1.1 allows reads during reset loading, describes all writes as ignored and describes BUSY as clear. Tables 6 and 10 on pp.26 and 29 instead describe BUSY as asserted at power-up. Exercise the clear-BUSY case. See pp.15–17 for comparator and startup behavior, pp.24, 26–29 for the map, CONFIG, limits and EEPROM_UL. [TMP116 datasheet](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17). |
| TMP117, TMP117M, TMP117N | SNOSD82D pp.16–18 describe comparator behavior, asserted startup BUSY and ignored CONFIG/limit writes during load. See pp.25, 27–30 for the map and fields. The load-time response to an offset write is unspecified. [TMP117 datasheet](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18). |
| TMP119 | SNIS236 pp.18–20 and pp.32, 34–37 document the corresponding behavior and register fields. The same offset uncertainty applies. The device can load programmed values other than the factory example. [TMP119 datasheet](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20). |

TI gives about 1.5 ms for startup without a guaranteed maximum. The 3 ms
wait below is project policy. Readback detects a suppressed write when the
old word differs from the target. A matching factory word alone cannot show
that a write took effect. [Reference manifest](../reference/manifest.json), [register ledger](../registers/tmp11x.json).

The clean local TMP1x2 checkout was at `0791429f8fce1170e2082f1b011456fc461318b8`.
Its `src/TMP1x2Configuration.cpp`, `test/test_core.cpp` and
`test/jobs/test_jobs.cpp` match the project pin `afd50889312f6e218a6a34eca687e3e8547d8b40`.
Reuse the one-transfer structure of `beginApply`/`stepApply` and the mismatch
and partial-write test patterns. TMP11x requires ID-first access, its own
startup clock, guarded shutdown and limit ordering. The peer starts with
CONFIG, settles extended-mode changes and writes TLOW first. [Sibling comparison](../library-comparison.md).

### Phase b deliverables and acceptance traces

`safe-apply-engine` belongs in the private core. `sync-initialize-recover`
adds `begin(const Config&)`, `begin()` and `recover()` to the existing device
class. All three drive the same resumable engine. Register its focused tests
with the host suite and document the delivered API, read effects and scope
of verification. [Public class](../../include/TMP11x/TMP11x.h), [test entrypoint](../../CMakeLists.txt), [coverage rows R04, R13, F05, F11 and X02](../prompts/FULL_COVERAGE.md).

| Planned case | Required observable result |
| --- | --- |
| Admission and target retention | Validate the entire candidate before replacing the binding. Invalid selectors or inverted THERM limits cost zero callbacks and preserve the prior route, target and evidence. Bind-only diagnostics allow a missing clock. Timed begin checks its clock and budget before replacement. An accepted replacement retains its own target even if I/O fails. For a settings candidate on an existing binding, retain the old target until immediately before the first mutating callback, including shutdown. |
| FIRST_ACK and bounded waits | Read ID at `0x0F` first. An incompatible word costs one tracked physical success and stops dependent I/O. With proof absent, anchor only on this operation's compatible ID completion. At floor tick 50 the first eligible startup tick is 54 for 1 ms resolution or 70 for 10 ms resolution. Use the wide interval `q + ceil(d/q) × q`. Round the operation budget down to whole ticks, clip callback grants to remaining time and expire at equality. Test wrap, a frozen clock, delayed power after bind and end/begin. Discard unfinished origins. Established proof requires continuously stable power and exclusive ownership. |
| Guard and forbidden frames | Read EEPROM_UL at `0x04` before mutation. Mask with `0xC000`. EUN set, including `0xC000`, latches unsafe immediately. Ordinary recovery, a zero diagnostic word or rebind cannot legitimize a known unlocked target without external correction. With EUN clear and BUSY set, wait at least 1 ms from completion using the floor-clock rule, then reread only `0x04`. Failures end the attempt. One safe word covers dependent writes only within uninterrupted ownership of this operation. BUSY observed in CONFIG cancels it. Assert zero premature writes for busy timeout and clear-BUSY TMP116 load, zero data writes outside CONFIG/THIGH/TLOW and zero general-call or alert-response traffic. |
| Resume and work limits | Inspect successive private steps and compare their combined transcript with public begin/recover. Each dispatch costs zero or one callback. A parked wait retains a due time and returns pending. Cap the operation at 256 dispatches and the same-tick unsuccessful wait checks at 1,000,000. A late complete callback retains its physical outcome and read effects. Expiry prevents any subsequent transfer or publication. |
| Ordered limits and comparator mode | Observe the current pair as needed. In THERM, moving [0,10] °C to [15,20] writes THIGH `0x0A00` before TLOW `0x0780`. Lower TLOW first if the new THIGH is below the old TLOW. Before changing THERM [0,10] to ALERT [20,10], write and verify SD/ALERT CONFIG `0x0620` under the guard. Failed, mismatched or late verification permits zero inverting writes. In reverse, verify TLOW `0x0000` and THIGH `0x0500` before enabling THERM. Unknown or inverted initial limits require full replay. Independent limit writes can change the pin between transfers. |
| Final verification and read effects | For every profile change, including repeat begin, limit-only and pin-only changes, establish guarded SD, apply and verify stable CONFIG and both limits, consume the SD CONFIG baseline, write the desired mode, then read CONFIG again. Check exact pointer order and both 16-bit limit values. Compare canonical stable fields with the existing MOD-aware comparator. If final CONFIG matches but reports BUSY, retain its flags, pace a new safe `0x04` and repeat final CONFIG. A timely full match clears dirty and advances the verified generation. Every CONFIG read consumes readiness. Accumulate ALERT HIGH/LOW until consumption, track THERM HIGH as a level, and preserve historical events and possible loss on failed reads. Final DATA_READY is the new baseline. |
| Partial failure, recovery and latest | Inject an ambiguous first or second write, an independently mismatched limit, and a late or mismatched final CONFIG. Retain the target, dirty state and partial evidence. A first failed initialization cannot admit latest. Explicit recover replays the target under a fresh guard without reset, rollback or transport retry. On timely success for each of the six models, public latest costs one tracked TEMP read, reports freshness false and keeps offset provenance unknown. Already initialized dirty reads retain untrusted provenance. Compare physical health with the callback count, including at deadline equality. |

Stage 19 owns public invalidation and expanded trust snapshots. Use fresh
binding or private state for proof-renewal tests here. Stage 22 owns public
alert consumption, stage 23 adds offset, stage 29 connects the private steps
to jobs, and stage 31 extends fault coverage. Stage 18 verifies CONFIG and both
limits and leaves `0x07` untouched on every model. [Handoff boundaries](../prompts/HANDOFFS.md).

### Phase a checks

| Command | Result |
| --- | --- |
| `cmake --build build/p47d/host --config Release --parallel 4`, then `cmake -E chdir build/p47d/host ctest -C Release --output-on-failure` | PASS, the existing Release build was current and all 14 prerequisite tests passed with CMake/CTest 4.0.1. |
| `python -B scripts/check_references.py` and `python -B tools/check_register_ledger.py` | PASS with Python 3.12.10, all 14 pinned PDF hashes and six BOM model maps. |
| `python -B scripts/check_promptset.py` | PASS, 55 stages. |

Phase b can begin. The acceptance traces above await implementation. Adapter
links and wire timing, physical qualification, installed-package qualification
and remote CI retain their respective checkpoints in stages 34–36, 40–41, 42
and 43.

## Phase b implementation

**PARTIAL, 2026-11-23.** Phase b started at
`1587c6b8d5280610e4254efaa53bd2ac566d98ab` on clean `main`. The phase-a
commit changed only this report and has parent `fc1d1de`. The wrapper
recorded upstream equality at 22:45:23 Prague time. No unexpected files
were present. The pinned TI startup, CONFIG and EEPROM_UL pages and the
TMP1x2 reference revision listed above were rechecked.

`sync-initialize-recover` implements both `begin` overloads and `recover`.
`bind` validates all settings before replacement. The timed overload also
checks clock availability and budget. A successful replay enables a tracked
latest read with verified CONFIG/limit provenance and unknown offset. A
later ambiguous write retains the desired target and leaves an already
initialized instance available for explicitly dirty latest observations.
[Public API](../../include/TMP11x/TMP11x.h), [synchronous driver](../../src/TMP11x.cpp).

`safe-apply-engine` verifies identity, anchors a missing startup wait to
compatible ID completion, and obtains a fresh EEPROM_UL guard. Each step
costs at most one sensor callback. The clock, pending interval and remaining
dispatch count bound waits. The engine stages a settings candidate until
the first shutdown write attempt. Explicit recovery uses the initial CONFIG
and limits as starting values and verifies each owned field after writing.
[Private phases](../../src/Apply.h), [apply implementation](../../src/Apply.cpp).

A normal ALERT replay of the 0–10 °C test profile uses 14 callbacks. The
trace below starts with the family check and ends with the generation
baseline. Each write lists the complete word in wire order.

```text
R0F R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 R01 W01=0220 R01
```

The engine verifies the first limit before the second write can depend on
it. A suppressed but acknowledged THIGH when raising a THERM pair leaves
the old THIGH in place and stops replay before TLOW. The reverse order
protects a descending pair. Transitions into THERM verify both ordered
limits first. A transition out of THERM verifies shutdown in ALERT mode
before the first inverting write. All CONFIG reads retain alert evidence,
including at deadline equality. Only a timely final match with a valid guard
publishes a generation. The final read consumes readiness.

### Phase b host evidence

| Named cases in `apply_cpp` | Result |
| --- | --- |
| `allModelsBeginRecoverLatest`, `admissionPreservesBinding` | PASS. Public replay, health and latest on all six BOM selections. Invalid profile or timing preserved the original binding. An accepted replacement survived a subsequent ID mismatch. |
| `startupFromCompatibleCompletion`, `workLimitsWrapAndLoadSuppression` | PASS. Delayed power, end/begin, wrap, 1 and 10 ms clocks, frozen-clock exhaustion, 256-dispatch BUSY loop and a TMP116 clear-BUSY, ignored-write fixture. |
| `busyAndUnlockedGuards`, `busyRecheckAndFinalBusy` | PASS. Immediate EUN refusal, paced BUSY rereads, guard failure and a new guard plus final CONFIG after BUSY at verification. |
| `comparatorTransitions`, `ignoredLimitCannotInvertThermPair`, `pinOnlyChangeUsesSameGuardedReplay` | PASS. Both cross-mode transitions, raised and lowered THERM pairs, lost first writes, and routing/polarity-only replay. |
| `stagedIntentAndIndependentReadback`, `finalVerificationAndAlertEvidence`, `replayAndStagedChangeOnDrift` | PASS. First-write intent, partial changes, low-bit limit mismatch, retained alerts, late or short final reads, continuous alias, staged drift refusal and recovery in one call. |

The register fixture can now acknowledge a scripted write while keeping the
previous word. The new cases compare literal addressed frames, read effects
and transfer counts. [Tests](../../test/test_apply.cpp), [fixture](../../test/support/FakeTmp11xDevice.cpp).

| Command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4`, then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 15/15 with GCC 15.1.0 and CMake/CTest 4.0.1, including register-ledger validation and regressions. |
| `cmake --build build/consumer --config Release --parallel 4`, then `build/consumer/tmp11x_native_consumer.exe` | PASS, tests-off source consumer linked and printed `0.1.0`. |
| `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -` | PASS, seven public headers in isolation and three README examples. |
| `doxygen -` over `include/TMP11x` with XML output and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2 documented both begin overloads and recover. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, all 14 pinned PDFs, 55 stages and whitespace, with Python 3.12.10. |

Phase c owns fresh review of these changes. The public invalidation path is
due in stage 19. Until then, a latched EEPROM unlock blocks subsequent
managed writes on that instance across ordinary replay, teardown and rebind.
MCU links, physical qualification, package qualification and remote CI were
not run in this phase.

## Phase c — startup proof, guard renewal and wait limits

**PARTIAL, 2026-11-23.** Reviewed the phase-b changes at
`719a10368120a470bf995f5fd681c8153573d7d7` on clean `main`. The
15 changed paths and parent `1587c6b` matched the implementation request.
The wrapper verified upstream equality at 23:04:40 Prague time. Two
read-only reviewers examined timing and guard-test coverage in parallel.
The silicon cross-check covered TMP116 pp.17, 26, 29, TMP117 pp.18, 30 and
TMP119 pp.20, 37 from the phase-a source table.

- Reproduced a wait-limit error in `workLimitsWrapAndLoadSuppression`.
  After the clock advanced one tick during the startup wait, the 1,000,000th
  subsequent unchanged check returned `WORK_LIMIT` one check early.
  `Apply::step` now charges only checks on an unchanged tick. The advancing
  check and the next 1,000,000 unchanged checks stay pending. The following
  check stops with only the ID read in the transcript. [Work limits](../owner-operations.md#work-limits).
- `recoveryStartupProof` expired one operation after its compatible ID
  and stepped the next attempt at a later tick. The new guard first ran
  four ticks after the new ID completion on a 1 ms clock. After successful
  initialization, public recovery completed at a frozen tick and read a
  fresh guard before writing.
- `busyShutdownReadbackRequiresNewGuard` observed HIGH, DATA_READY and
  BUSY during shutdown verification. The next transfer read EEPROM_UL.
  Its `0xC000` response stopped both limit writes, retained HIGH and left
  the target dirty with no verified baseline. `busyRecheckAndFinalBusy`
  now exercises the policy's `0x3FFF` locked/idle word. [Guard vectors](../eeprom-and-reset-policy.md#transcript-vectors-for-implementation).

The focused `apply_cpp` test failed on the new wait-limit assertion before
the fix. Afterward, `cmake --build build/host --config Release --parallel 4`
and `cmake -E chdir build/host ctest -C Release --output-on-failure`
passed all 15 tests (GCC 15.1.0, CMake/CTest 4.0.1).
`python -B scripts/check_references.py` verified 14 PDFs,
`python -B scripts/check_promptset.py` passed all 55 stages and
`git diff --check` passed.

Phase c continues with comparator and limit ordering, staged first-write
intent versus explicit recovery, and late CONFIG effects on alerts, profile
generation and latest-read provenance.

### Comparator ordering and partial-write recovery

**PARTIAL, 2026-11-23.** The clean review baseline was
`e5c0d65002c08029bf054baafe91e90df8ee595b`. Its parent, subject and three
paths matched the preceding commit request. The wrapper recorded upstream
equality at 23:14:57 Prague time. Two read-only reviewers traced comparator
transitions and staged intent independently. The lead checked their findings
against the signed limit comparisons, shutdown readback and first-write
commit in the apply engine. The three missing fault cases passed against
the existing implementation. [Apply engine](../../src/Apply.cpp),
[non-atomic THERM vector](../registers/behavior-vectors.json).

- `ignoredLimitPreventsThermEntry` starts with an inverted ALERT pair of
  [20, 10] °C and requests THERM [0, 8] °C. A fixture ACK that leaves
  either limit unchanged produces `CONFIG_MISMATCH` on its readback.
  The transcript ends there with the device in shutdown and ALERT mode.
  Both target words differ from their original values.
- `stagedThermLimitsAndPartialRecovery` expires a staged change just before
  the shutdown write. The five preflight reads preserve the verified
  [0, 10] °C target. A second attempt to raise it to [15, 20] °C verifies
  THIGH first, then receives a data NACK on TLOW. The fixture retains a
  partial TLOW word of `0x0700`. The driver stops in shutdown with the
  requested pair retained, dirty trust and no baseline. Explicit public
  recovery acquires a new guard and verifies both words and final CONFIG.
  [Native cases](../../test/test_apply.cpp), [apply contract](../api-contracts.md#startup-apply-and-reset).

The source cross-check covered ALERT and THERM behavior in TMP116 pp.15–16,
TMP117 pp.16–17 and TMP119 pp.18–19 from the phase-a source table.
`cmake --build build/host --config Release --parallel 4` and
`cmake -E chdir build/host ctest -C Release --output-on-failure` passed
all 15 tests with GCC 15.1.0 and CMake/CTest 4.0.1. Python 3.12.10 passed
`python -B scripts/check_references.py` (14 PDFs) and
`python -B scripts/check_promptset.py` (55 stages). `git diff --check` passed.

The next phase-c slice reviews late and failed CONFIG observations across
generations, retained alert provenance, the latest sample cache and unknown
offset. Phase d owns final acceptance.

### Final CONFIG read after a verified profile change begins

The review started on clean `main` at
`5fcd9a249d107dd9d2eb8b0b914e88cf5af2375c`. The commit's parent, subject
and two paths matched the comparator review request. The wrapper recorded
upstream equality at 23:29:46 Prague time on 2026-11-23. Two read-only
reviewers traced alert retention and sample publication independently.
The implementation matched both contracts. The existing final-read fault
test started before the first successful initialization, so it could not
detect replacement of an earlier sample or loss of a verified ALERT event.
[Publication contract](../api-contracts.md#acquisition-and-read-effects),
[comparator behavior](../device-behavior.md).

`finalThermReadAfterVerifiedAlert` exercises an initialized TMP117 with a
cached latest sample. Preflight captures LOW in the verified ALERT profile.
After the staged change to THERM, final CONFIG returns HIGH and DATA_READY
at the deadline, or returns one byte with possible destructive read effects.
The complete late word records a HIGH level. The short read leaves the
level unknown and records possible loss. In both cases the original LOW
keeps its generation and timestamp. The device stays initialized with dirty
trust, no baseline and its original cached sample. [Native test](../../test/test_apply.cpp).

A subsequent public latest read records the old generation with dirty trust.
Explicit recovery obtains a fresh guard, observes THERM HIGH clear and
establishes a baseline at final CONFIG completion. The historical LOW and
any possible loss survive recovery. The dirty sample stays in the cache until
the next TEMP read, which reports the same raw value in the next verified
generation. Both latest reads report unconfirmed freshness and unknown offset.
[Apply engine](../../src/Apply.cpp), [CONFIG observations](../../src/TMP11x.cpp),
[latest reads](../../src/LatestRead.cpp).

The source check covered TMP116 SBOS740A pp.15–16, TMP117 SNOSD82D pp.16–17
and TMP119 SNIS236 pp.18–19. The fixture supplies the THERM level at final
readback because the flags' behavior across the mode transition is unspecified.
`cmake --build build/host --config Release --parallel 4` and
`cmake -E chdir build/host ctest -C Release --output-on-failure` passed
all 15 tests with GCC 15.1.0 and CMake/CTest 4.0.1.
Python 3.12.10 passed `python -B scripts/check_references.py` (14 PDF hashes)
and `python -B scripts/check_promptset.py` (55 stages). `git diff --check` passed.
Phase c review is finished. Phase d owns final acceptance and stage closure.

## Phase d acceptance

The acceptance baseline on 2026-11-23 was
`ea27650b993410fb28e509e9d2d938645838956c`. The worktree was clean.
The parent `5fcd9a2`, subject and two changed paths matched the last
phase-c request. The wrapper recorded equality with `origin/main` at
23:45:01 Prague time.

The 17 cases in `apply_cpp` cover the two stage outputs.
`safe-apply-engine` verifies identity, waits for startup when needed and
checks EEPROM_UL before the first write. Its scripted traces exercise
BUSY and EUN, suppressed writes, ordered limit verification, and a late
final CONFIG read that retains flags and leaves the profile dirty.
`sync-initialize-recover` uses those same steps for both `begin` overloads
and explicit recovery. On all six model selections the tests observe a
successful public latest read after initialization, with unknown offset
and unconfirmed freshness. [Private engine](../../src/Apply.cpp),
[public implementation](../../src/TMP11x.cpp), [acceptance cases](../../test/test_apply.cpp).

| Acceptance command | Result |
| --- | --- |
| `cmake --build build/host --config Release --clean-first --parallel 4` then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS. All 15 tests passed with GCC 15.1.0 and CMake/CTest 4.0.1, including apply, latest, health and the register-ledger checks. |
| `cmake --build build/consumer --config Release --clean-first --parallel 4` then `build/consumer/tmp11x_native_consumer.exe` | PASS. The tests-off source consumer linked and printed `0.1.0`. |
| `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -` | PASS. Seven headers compiled in isolation. Each of the three README C++ examples compiled with the public umbrella header. |
| `doxygen -` with public headers as input, XML output under `build/p18d-docs` and `WARN_AS_ERROR=YES` | PASS. Doxygen 1.13.2 emitted no warnings. The XML documents both overloads and recovery. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS. Python 3.12.10 verified the 14 pinned PDFs and the 55-stage promptset. The closure diff passed the whitespace check. |

Stage 19 takes the engine and retained observations forward for public
invalidation and trust snapshots. Stage 23 owns managed offset and stage 29
owns job integration. MCU links (34–36), physical qualification (40–41),
installed-package qualification (42) and remote CI (43) remain open.
