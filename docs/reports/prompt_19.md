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
