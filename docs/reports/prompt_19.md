# Prompt 19 — Observed configuration, verification and dirty state

## Phase a — Requirements and baseline, 2026-11-24

**PARTIAL.** All seven inputs to `config-trust` are available. The clean
baseline on `main` was `21eba162292da25f80fc7e80514b57bdc57c5449`.
Its parent `ea27650b993410fb28e509e9d2d938645838956c`, subject and four
paths match the stage 18 closure request. The wrapper recorded upstream
equality at 23:59:30 Prague time on November 23. The local upstream still
matched at entry. Stages 7, 8, 15, 17 and 18 have completion evidence.
Audit 48 follows stage 19. [Requirements](../prompts/19_config-trust.md),
[sequence](../prompts/SEQUENCE.json), [progress](../prompts/PROGRESS.md).

### Inputs at this revision

| Input | What phase b can use |
| --- | --- |
| `safe-apply-engine` | `Apply::readConfig` selects the expected profile for the current step, records flags even at expiry and renews a revoked guard. `readLimit` records complete words separately. The final timely readback advances the generation. [Apply phases](../../src/Apply.h), [implementation](../../src/Apply.cpp). |
| `sync-initialize-recover` | Both public begin overloads and recover use those phases. The first mutating callback commits staged intent. An accepted bind clears observations and health. Identity mismatch clears live state while keeping the historical sample and pending alerts. [Lifecycle implementation](../../src/TMP11x.cpp), [stage 18 acceptance](prompt_18.md#phase-d-acceptance). |
| `config-codec` | Reuse `decodeConfiguration` and `compareExpectedRegister` for raw MOD, flags, stable fields and full-word limits. The comparator already accepts an explicit `OwnedTrigger` argument. [Public codec](../../include/TMP11x/Configuration.h), [implementation](../../src/Configuration.cpp), [stage 17 cases](../../test/test_configuration.cpp). |
| `transport-health` | The tracked path records each physical callback once. `TransferResult` separates a local refusal, a failed attempt and a terminal success with optional completion tick. A logical mismatch or elapsed operation deadline adds no transport failure. [Transport](../../src/RegisterTransport.h), [health tests](../../test/test_health.cpp). |
| `safety-policy` | Each operation that needs a write or fresh result acquires its own locked/idle observation at 0x04 after startup proof. An observed CONFIG busy bit revokes it. The owner must report loss of exclusive access or stable power. [Guard and invalidation policy](../eeprom-and-reset-policy.md). |
| `behavior-vectors` | Use the literal traces for alert acknowledgement, THERM hysteresis, continuous generation changes, destructive diagnostics, startup anchoring and one-shot provenance. [Behavior vectors](../registers/behavior-vectors.json). |
| `register-fake` | The fixture records pointer, data, timing and physical outcome, can consume flags before a failed read, and can apply or suppress a scripted write independently of the callback result. [Fixture](../../test/support/FakeTmp11xDevice.cpp), [transport cases](../../test/test_register_transport.cpp). |

### Source audit

The pinned CONFIG tables document the continuous alias, READY acknowledgement
on CONFIG or TEMP reads, ALERT latch acknowledgement on CONFIG reads, and
THERM HIGH as a level. They specify the one-shot command and leave its
subsequent MOD readback unspecified. TMP117/TMP119 Soft_Reset reads zero.
TMP116 has no supported addressed reset. CONFIG bit 12 reports BUSY.
The lock and busy decision uses bits 15 and 14 of EEPROM_UL at 0x04.
See TMP116 pp.25–26, 29, TMP117 pp.26–28, 30 and TMP119 pp.33–35, 37.
[TMP116 Table 6](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26),
[TMP117 Table 7-6](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27),
[TMP119 Table 8-5](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34).

TMP1x2's `src/TMP1x2Configuration.cpp` and `test/test_core.cpp` show why
drift must latch as each word arrives. A later threshold timeout preserves
the mismatch already observed. ADS1115's `src/ADS1115.cpp`,
`include/ADS1115/Config.h` and `test/test_basic.cpp` distinguish masked
readback from optional ALERT/GPIO ownership. ADS1115 still writes both
thresholds when the comparator is disabled. TMP11x has its own stable mask,
guarded replay order and opt-in offset policy. The two local trees were
clean at `0791429f8fce1170e2082f1b011456fc461318b8` and
`1aab07bb84d7494422632eda500913ebf3bee660` respectively. The listed files
matched the project pins. [Pinned sibling comparison](../library-comparison.md).

### Implementation brief and acceptance cases

Stage 18 stores CONFIG with a completion time, optional raw limit words,
trust, generation and a ready baseline. Its private alert record collects
apply reads. The latest-read engine reports the outcome of a TEMP attempt
and protects the sample cache at expiry. Stage 19 needs one read-effect
path for both callers, timed observations for each owned field, retained
dirty and verification evidence, and the public inspection and invalidation
APIs. Keep the time of full verification independent of the ready baseline.
[Current device state](../../include/TMP11x/TMP11x.h), [latest engine](../../src/LatestRead.cpp),
[public contracts](../api-contracts.md), [owner operation limits](../owner-operations.md).

| Phase-b case | Required evidence |
| --- | --- |
| Observation versus verification | With a bound, initialized device, `readConfiguration(out)` makes one tracked CONFIG read. A complete changed word reaches the caller and latches drift. `verifyConfiguration()` checks CONFIG and both limits in at most three tracked reads and reports `CONFIG_MISMATCH` on disagreement. No writes. Change one stable selector and each limit's low bit independently. Check zero callbacks on unbound or uninitialized refusal. |
| Partial verification | In the bus-silent `SettingsSnapshot`, distinguish the desired target, valid raw/decoded observations and their times, current trust, first unresolved dirty reason, affected fields, and the last verified generation/time. Preserve a completed prefix and distinguish earlier observations from this attempt. A transfer failure supplies no new word. Later matching observations cannot clear dirty or adopt the sensor's changed target. Timely complete verification can confirm a clean profile. |
| Ambiguous writes and recovery | Inject an error on the first SD write and on a middle limit write, including when the fake actually applied it. Preserve the candidate, mark affected fields uncertain, and retain the first unresolved cause through subsequent reads. Already initialized latest reads expose DIRTY and unconfirmed freshness. A failed first initialization cannot admit latest. Explicit guarded replay of all three owned words restores trust only on full timely readback. |
| Destructive reads and provenance | Feed ALERT HIGH and LOW on different reads, then clear flags, switch generations/modes, and read THERM HIGH followed by zero. Preserve old events with source, mode, time and generation, mark mixed or unknown origins when needed, and keep the live THERM level separate. A failed or short CONFIG attempt can lose alert and ready history. An ambiguous TEMP can lose readiness alone. A refusal has no read effect. Deadline-equality success still records the physical word/effects and cannot initialize, clear dirty, publish a sample or replace the cache. Only explicit consumption drains retained events. |
| BUSY and stable comparison | Against desired continuous `0x0220`, accept observed alias `0x0A20`; flag/status changes alone preserve stable trust. A matching CONFIG with BUSY and READY records the consumed indication and revokes any current guard. Require safe 0x04 again before dependent mutation or freshness. Stable drift plus BUSY still latches mismatch. No additional CONFIG read solely to inspect BUSY. |
| Owned one-shot | Against desired SD `0x0620`, unowned `0x0E20` is drift. With terminal physical trigger success, including at expiry, accept either MOD=01 or 11 while comparing every other stable field. A failed trigger cannot establish ownership. An ambiguous SD write dirties trust. Keep provenance through timeout/cancel until successful explicit SD or invalidation/rebind. Exercise the comparison and evidence transitions here. Stage 21 supplies the guarded SD/baseline/trigger sequence. |
| Owner-reported invalidation | `invalidateDeviceState()` on a bound instance costs zero callbacks. Preserve binding, desired target, physical health history, historical samples and pending events with their provenance. Clear startup proof, initialization, live trust, ready and trigger evidence, and live THERM level. Distinguish this from end/rebind and identity mismatch. After external correction of a latched EUN, explicit invalidation permits another guarded recovery. Under restored stable power, time the startup wait from this recovery's first compatible read. With no proof, refuse dependent access until its gates are satisfied. |

Extend the existing apply, latest and codec regressions, including
`stagedIntentAndIndependentReadback`, `finalVerificationAndAlertEvidence`,
`finalThermReadAfterVerifiedAlert` and `normalizedModeComparison`. Register
public trust tests in CTest and document the new calls, their bus cost,
exclusive ownership and the caller's invalidation duty. [Apply tests](../../test/test_apply.cpp),
[latest tests](../../test/test_latest.cpp), [host test entrypoint](../../CMakeLists.txt).

Stage 20 adds mode/cycle/averaging setters. Stage 21 enforces clean admission
before fresh I/O. Stage 22 exposes limit/alert wrappers and the explicit
alert consumer. Stage 23 adds supported managed offset to F11 and observes
unmanaged calibration. Through stage 19 the profile covers CONFIG and both
limits on all six BOM selections, with zero offset traffic. Stage 26 routes
diagnostic reads through the same bookkeeping. Stage 28 adds BUSY rejection
without state changes while an active or retained job owns the slot.
[Artifact owners](../prompts/HANDOFFS.md), [coverage R02–R04, F11–F12](../prompts/FULL_COVERAGE.md).

### Baseline checks

| Command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 15/15 existing tests, including register-ledger validation and regressions. CMake/CTest 4.0.1, GCC 15.1.0. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py` | PASS, 14 pinned PDF hashes and 55 prompt stages, Python 3.12.10. |

Phase b owns implementation and new test results. Physical qualification
remains open for stages 40–41. Stage 43 owns the first remote CI checkpoint.

## Phase b implementation

**PARTIAL, 2026-11-24.** Implementation baseline on `main` was
`9227badfb19a3701200afc90a7fadad687ad06c4`. Its subject, parent and sole
changed path match the phase-a commit request. The wrapper recorded upstream
equality at 00:15:30 Prague time. The checkout was clean at entry.
The pinned PDFs and sibling revisions listed above still matched.

The public `readConfiguration` returns a complete CONFIG observation in
one callback. `verifyConfiguration` reads CONFIG, TLOW and THIGH, compares
each word on arrival, and retains the first mismatch and affected fields.
A later transport failure stops the scan and leaves the observed prefix
in the snapshot. A matching scan while dirty returns `CONFIG_DIRTY`.
Only full, timely, guarded replay advances the generation and restores
trust. `getSettingsSnapshot` and `invalidateDeviceState` require zero
callbacks. Invalidation retains the historical verification time, sample
and pending alerts. [Public API](../../include/TMP11x/TMP11x.h),
[device implementation](../../src/TMP11x.cpp), [user guide](../../README.md#inspect-configuration-trust).

The `config-trust` artifact extends the stage 18 apply engine and the
stage 16 TEMP reader. Both record physical effects before deciding the
operation outcome. A failed write invalidates the affected register
observation. A complete CONFIG records raw MOD and the consumed flags even
at the deadline. A new sticky event keeps the first event's attribution and
marks mixed origins where applicable. A short CONFIG can lose alert and
ready history. A short TEMP can lose ready history. [Device evidence](../../src/DeviceEvidence.cpp),
[apply](../../src/Apply.cpp), [latest reads](../../src/LatestRead.cpp).

| Host cases | Observed result |
| --- | --- |
| `admissionAndObservations`, `verifyLimitDrift`, `partialVerificationRetainsEvidence` | All six models passed public admission and observation. Independent one-bit changes to CONFIG, TLOW and THIGH latched drift. On a final THIGH timeout the scan preserved the new CONFIG and TLOW, the old THIGH timestamp, the two-bit completion mask and the prior verification time. |
| `ambiguousWritesKeepTheTarget`, `interruptedReadbackKeepsItsFirstCause`, `invalidationAfterExternalCorrection` | First and middle write faults preserved intent. A later fault preserved the first unresolved cause. After external EUN correction, invalidation retained physical health and forced a new FIRST_ACK wait and guard before replay. |
| `retainedAlertsAcrossGenerations`, `finalVerificationAndAlertEvidence`, `busyAndAmbiguousReadEvidence`, `tempConsumesRetainedReadiness` | HIGH and LOW survived zero observations and replay with the first event's time and generation. BUSY revoked eligible readiness. Late and short reads preserved physical effects separately from publication and verification. Invalidation preserved the historical sample. |
| `physicalTriggerEvidence`, `normalizedModeComparison` | Injected physical trigger outcomes exercised the owned MOD exception and remaining stable bits. An ambiguous SD write dirtied trust. An actual successful SD callback at the deadline ended ownership. Stage 21 owns the guarded trigger command and acquisition timing. |

These cases run in the registered [public trust](../../test/test_trust.cpp),
[apply](../../test/test_apply.cpp), [latest](../../test/test_latest.cpp) and
[codec](../../test/test_configuration.cpp) suites.

| Local command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 16/16, CMake/CTest 4.0.1 and GCC 15.1.0. Includes register-ledger validation and regressions. |
| `cmake --build build/consumer --config Release --parallel 4` then `build/consumer/tmp11x_native_consumer.exe` | PASS, tests-off consumer linked and printed `0.1.0`. |
| `doxygen -` over the public headers with `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. XML under `build/p19b-docs` contains all four new public members. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, Python 3.12.10 verified the 14 pinned PDFs and the 55-stage promptset. |

The next run reviews this implementation against the phase-a cases.
Stage 22 exposes explicit alert consumption, stage 23 extends the owned
profile to managed offset, and stage 28 adds active/result-slot gates.
MCU links, physical qualification and remote CI retain their later checkpoints.

## Phase c review

**PARTIAL, 2026-11-24.** Reviewed the 20-path phase-b diff at
`dac11421a44130e6404be03930496fa5858c974d` on clean `main`. Its parent,
subject and paths matched the implementation request. The wrapper recorded
upstream equality at 00:36:18 Prague time. Two read-only reviewers traced
destructive reads and profile trust in parallel. The review checked the
seven phase-a cases against the public API, private engines and registered
tests, using the pinned CONFIG and EEPROM_UL pages listed above. Both
sibling comparison trees were clean at their phase-a revisions.

- After a short CONFIG read, a successful TEMP read left the continuous
  readiness baseline empty. The next CONFIG could therefore discard a new
  DATA_READY indication. `tempRestoresContinuousBaseline` failed at the
  missing baseline before the fix. The TEMP reader now records its completion
  tick as the new boundary in a verified continuous profile. The latest
  sample still carries `freshConversion == false`. The regression also
  checks shutdown, dirty trust and retained alert-loss evidence.
  [Physical read effects](../../src/DeviceEvidence.cpp), [latest tests](../../test/test_latest.cpp).
- `preflightDriftAtDeadline` returns a changed CONFIG or TLOW exactly at
  the recovery deadline. Both traces ended on that read with
  `OPERATION_TIMEOUT`, zero writes and one tracked success per callback.
  The snapshot retained the changed word, completion tick, dirty cause and
  prior verification time and generation. CONFIG also retained HIGH and
  discarded ready credit. [Apply tests](../../test/test_apply.cpp).
- Clarified when replay clears the last explicit verification's completed
  fields. The preflight-only failures retain the mask.
  `stagedIntentAndIndependentReadback` checks that even an ambiguous first
  shutdown write clears it. [Snapshot contract](../api-contracts.md),
  [public header](../../include/TMP11x/TMP11x.h).

The existing cases also covered partial verification after limit drift,
ambiguous first and middle writes, full replay, continuous MOD aliases,
owned one-shot MOD comparison, BUSY guard renewal, sticky alerts and
owner-reported invalidation. The source review found no further changes
needed for those paths. Guarded trigger dispatch, explicit alert consumption
and managed offset remain with stages 21, 22 and 23.

| Local command | Result on the reviewed changes |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 16/16 with GCC 15.1.0 and CMake/CTest 4.0.1. The focused configuration, latest, apply and trust suites also passed together. |
| `cmake --build build/consumer --config Release --parallel 4` and `build/consumer/tmp11x_native_consumer.exe` | PASS, tests-off consumer printed `0.1.0`. |
| `doxygen -` over the public headers with `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. The generated XML documents the first replay write attempt as the mask-clearing boundary. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py` and `git diff --check` | PASS, 14 pinned PDFs, 55 stages and clean whitespace. |

Phase d owns final acceptance and the progress and report-index updates.
Stage 43 owns the first remote CI checkpoint. Physical qualification remains open.
