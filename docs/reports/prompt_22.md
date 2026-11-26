# Prompt 22 — Temperature limits, therm mode and ALERT

## Phase a — Requirements and baseline, 2026-11-26

**PARTIAL.** All nine inputs for `limits-alert` are available on clean
`main` at `546b80eb0c9099bf253d9ebcd92ccb286b53a062`. This commit closes
audit 49. Its parent `0da112d474ea0583889e882e73226ab06b70ffaa`, subject
and four changed paths match the preceding wrapper run and commit request.
The wrapper verified equality with `origin/main` at 00:30:04 Prague time,
and both local refs matched at entry. Stage 22 follows audit 49 in the
sequence. Phase b owns implementation and tests for all six BOM selections.
[Prompt](../prompts/22_limits-alert.md), [sequence](../prompts/SEQUENCE.json),
[progress](../prompts/PROGRESS.md).

### Confirmed inputs

| Artifact | Evidence inspected at the baseline |
| --- | --- |
| `register-ledger` (5) | [Register inventory](../registers/tmp11x.json) records CONFIG flags and routing, THIGH at 0x02 and TLOW at 0x03, including the two one-shot wording conflicts. [Stage 7 policy](../eeprom-and-reset-policy.md) limits writes to guarded volatile changes. |
| `behavior-vectors` (6) | [Literal vectors](../registers/behavior-vectors.json) give negative limits, latch acknowledgement and re-arm, therm hysteresis, eight pin routes and the non-atomic limit transition. |
| `temperature-codec` (16) | [Signed conversion](../../src/Temperature.cpp) and [temperature tests](../../test/test_temperature.cpp) establish the common signed 16-bit, 1/128 °C format for both limits. |
| `config-codec` (17) | [CONFIG codec](../../src/Configuration.cpp) validates all selectors and therm ordering, decodes mode-specific read effects and compares both complete limit words. [Codec tests](../../test/test_configuration.cpp). |
| `config-trust` (19) | [DeviceEvidence](../../src/DeviceEvidence.cpp) records per-word observations and drift, retains alert events across reads and flags possible loss after an ambiguous CONFIG read. [Trust tests](../../test/test_trust.cpp). |
| `sync-acquisition` (21) | [Acquisition](../../src/Acquisition.cpp) records CONFIG poll evidence before a subsequent TEMP or deadline failure can suppress sample publication. [Acquisition tests](../../test/test_acquisition.cpp). |
| `safe-apply-engine` (18) | [Apply](../../src/Apply.cpp) checks the startup/EEPROM guard and current words, verifies shutdown and each ordered limit write, then verifies the final CONFIG before advancing the generation. [Apply tests](../../test/test_apply.cpp). |
| `register-fake` (12) | [Register fixture](../../test/support/FakeTmp11xDevice.cpp) models CONFIG/TEMP read effects, scripted transfer faults and alert-mode block completion. Its conversion path currently refuses therm mode. |
| `audit-20-21` (49) | [Acceptance report](prompt_49.md#phase-d-acceptance) covers recovery, one-shot guard renewal, deadline clipping and CONFIG read effects. Stage 22 supplies the public consumer for retained events. |

### Sources and interpretation

| TI datasheet, printed pages | Comparator and one-shot behavior | Register encoding and limits | Pin and external alert response |
| --- | --- | --- | --- |
| [TMP116/N, SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=15) | 14–16 | 25–26, 28 | 3, 21 |
| [TMP117/M/N, SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=16) | 15–17 | 26–29 | 4, 21–22 |
| [TMP119, SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=18) | 17–19 | 33–36 | 5, 24 |

CONFIG acknowledges DATA_READY and, in alert mode, HIGH and LOW. TEMP
acknowledges DATA_READY alone. In therm mode, HIGH sets strictly above THIGH
and clears strictly below TLOW at a completed comparison. At equality it
keeps its previous state. LOW is zero. Therm HIGH survives shutdown or an
aborted block while therm stays selected and no reset occurs. Pin persistence
through shutdown with routing and polarity held fixed is inferred. TMP116/N
block-end comparison under averaging is also an inference. Shutdown retention
of DATA_READY and alert-mode latches remains open. TMP117 p. 15 and TMP119
p. 17 mention ALERT at one-shot completion. The detailed flag tables make
threshold flags conditional on the comparison. [Behavior contract](../device-behavior.md).

Under threshold routing, an outside winning SMBus alert response can release
ALERT in alert mode while LOW remains latched until a CONFIG read. Therm
hysteresis persists. Direction with both flags latched and the response
under DATA_READY routing remain unspecified. Use addressed CONFIG for
acknowledgement and preserve uncertainty after outside access. Other open
physical questions include offset/comparator ordering on offset-capable
models and the exact clear edge during a concurrent conversion.
[Coverage, rows F09–F10 and X03](../feature-coverage.md).

Both sibling checkouts were clean. The reviewed files matched the project
pins at their local HEADs. TMP1x2 HEAD was `0791429f8fce1170e2082f1b011456fc461318b8`.
Its staged setters and partial-read handling are useful precedents. TMP11x
uses its own signed word format and conditional write order. OPT4001 HEAD
was `f17d4944640145773d3331e155a5bc728fcc4c66`. Its flag reader retains
evidence across later zero reads. TMP11x uses the addressed CONFIG word
for those observations. Fault queues, extended temperature mode, interrupt
queues and GPIO-derived freshness are outside the TMP11x contract.
[TMP1x2 configuration at pin](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2Configuration.cpp),
[OPT4001 implementation at pin](https://github.com/janhavelka/OPT4001/blob/441ef579d7fa3f7bca7665f52632fdaddd306300/src/OPT4001.cpp).

### Phase-b scope and acceptance cases

1. **Limit and alert setters.** Connect `setLimits(Limits)` and
   `setAlert(AlertMode, Polarity, PinFunction)` to the existing
   `applySettings` path. Exercise a limit-only change and a routing-only
   change from continuous mode through guarded shutdown, readback, baseline
   and verified return to continuous. Invalid selectors and an inverted
   therm target, including entry into therm, must fail before I²C. Equality
   is valid. For `readLimits(Limits&)`, use at most two tracked reads.
   Check exact signed words and drift on a complete pair. If the second read
   fails, retain the first observation in the settings snapshot and leave
   the caller's pair untouched. [Public contract](../api-contracts.md), [setter entry point](../../src/TMP11x.cpp).
2. **Write order and recovery.** Trace therm [0,10] °C to [15,20] °C
   with THIGH written and verified first, then the reverse with TLOW first.
   For therm [0,10] to alert [20,10], a failed, mismatched or late SD/alert
   readback must prevent the inverting `W03=0A00`. On return, verify
   `TLOW=0000` and `THIGH=0500` before enabling therm. An unknown or
   unexpectedly inverted current pair requires full guarded recovery and
   verification before therm operation. Separate writes expose intermediate
   states. An ambiguous write leaves the target dirty for explicit replay.
   Reuse the existing `comparatorTransitions` and ignored-write tests.
   [Apply tests](../../test/test_apply.cpp).
3. **Read effects and event ownership.** Expose `getAlertStatus()` and
   `takeAlertEvents(AlertStatus&)` over stage 19's retained record. Inspect
   repeatedly with zero callbacks. Only take consumes pending events and the
   loss indication. A copy carries the source, generation/mode, time validity,
   mixed/unknown provenance and read effects. A therm level needs its own
   observation validity and time when a pending alert came from an older
   read. Exercise ambiguous CONFIG, late success and a change of profile.
   Run an above-high owned one-shot whose readiness poll acknowledges HIGH
   and whose TEMP transfer then fails. Two takes must deliver HIGH exactly
   once while the previous sample survives. An in-range shot only signals
   readiness. [Evidence owner](../../src/DeviceEvidence.cpp), [public device](../../include/TMP11x/TMP11x.h).
4. **Comparator and GPIO.** Use the literal negative limits `FAC0` and
   `FCC0`. Verify equality, a crossing, return in range before CONFIG,
   acknowledgement and reassertion on the next out-of-range completion.
   Extend the fake's block completion for therm. Check in-band and equality
   retention, LOW=0 and a shutdown midway through an average. Use all eight
   `pin_routes` rows to check logical flags and physical levels separately.
   POL=1 asserts by releasing the open-drain line into its pullup. DR/Alert=1
   routes DATA_READY. Exercise `readAlertPinLevel(bool&)` unbound, without
   the optional hook, and with successful and failing hook calls. The owner
   samples GPIO outside ISR context with zero I²C, pin setup or changes to
   event history, readiness or health. Check that no transaction goes to
   the ARA address (0x0C). [Binding hook](../../include/TMP11x/Config.h),
   [literal traces](../registers/behavior-vectors.json).

Register new cases in [CTest](../../CMakeLists.txt) and update public Doxygen,
the user guide and the [feature ledger](../feature-coverage.md) with executed
evidence. Stages 26–27 integrate diagnostics and synchronous snapshots,
30 integrates measurement jobs and 38 exposes CLI operations. Physical pin
and read-clear evidence belongs to stages 40–41.

### Baseline checks

| Command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 20/20 existing suites, GCC 15.1.0 and CMake/CTest 4.0.1. |
| `python -B scripts/check_references.py` | PASS, all 14 pinned PDF paths, sizes and SHA-256 hashes. |
| `python -B scripts/check_promptset.py` | PASS, 55 stages, Python 3.12.10. |

Both read-only source reviews finished. Stage 22 feature acceptance remains
open. Hardware and MCU tests were not run. The first remote CI checkpoint
belongs to stage 43.

## Phase b implementation and native evidence

**PARTIAL, 2026-11-26.** Phase b began on clean `main` at
`51619bf012f4fb36b5b85f76bbce584c60905dd3`. Its parent, subject and
single report path match the preceding wrapper request. The wrapper verified
upstream equality at 00:45:30 Prague time. Both local refs matched on entry.
The two delegated test implementations used disjoint files and finished
before integration closed.

The `limits-alert` implementation adds six public operations. `setLimits` and
`setAlert` stage a candidate on the existing guarded apply engine. `readLimits`
observes TLOW then THIGH through the same drift bookkeeping as CONFIG. A
short THIGH transfer preserves the caller's previous pair and records any
TLOW drift. Both limit reads are independent of flag acknowledgement.
[Public declarations](../../include/TMP11x/TMP11x.h), [entry points](../../src/TMP11x.cpp).

`getAlertStatus` returns a copy. `takeAlertEvents` clears pending events and
their loss/provenance indicators after copying them. The latest complete
CONFIG read has its own source, timestamp and read-clear mask, so a newer
therm observation can coexist with an older alert event. A reported loss of
device state marks a gap in alert history and invalidates the therm level.
`readAlertPinLevel` samples the bound GPIO hook once, including before
initialization. Its result is independent of sensor flags, transport health
and fresh-read evidence. [Read effects](../../src/DeviceEvidence.cpp),
[API contract](../api-contracts.md), [user guide](../../README.md#set-limits-and-observe-alert).

| Executed host case | Evidence |
| --- | --- |
| `signedLimitsAndRouting`, `admissionAndThermOrdering`, `partialReadsAndDrift` | Signed `FAC0/FCC0` readback, both 14-transfer continuous-mode setter traces and local rejection of invalid selectors or inverted therm limits. First- and second-read failures preserve the caller's pair. [Limit suite](../../test/test_limits.cpp) |
| `interruptedThermPair`, `invertedStartingPairRequiresRecovery`, `comparatorTransitions` | Upward and downward crossings verify the first limit before writing the second. An ambiguous second write leaves a dirty target. A failed, mismatched or late SD/alert readback stops the write that would invert the pair. Recovery verifies both limits before enabling therm. [Limit suite](../../test/test_limits.cpp), [apply suite](../../test/test_apply.cpp) |
| `alertWindowAckAndRearm`, `thermHysteresisAndShutdown`, `eightPinRoutes`, `gpioObservationOnly` | All six models exercised signed crossings, equality, CONFIG versus TEMP acknowledgement, therm retention through an aborted average, and all eight pin routes. GPIO failures preserved output and added zero transport outcomes. Transfers used only the bound address. [Comparator suite](../../test/test_alert_comparator.cpp), [block fixture](../../test/support/FakeTmp11xDevice.cpp) |
| `takeOneShotAlertsAfterTempFailure` | On all six models the readiness poll returned `AE00`, acknowledging HIGH and READY. A subsequent TEMP timeout preserved the previous sample. Two bus-silent takes delivered HIGH once. An in-range shot returned `2E00` and produced no threshold event. [Acquisition suite](../../test/test_acquisition.cpp) |
| `finalThermReadAfterVerifiedAlert`, `retainedAlertsAcrossGenerations` | Earlier LOW retained its original generation and time when a later therm read completed at expiry. An ambiguous CONFIG read marked possible loss. A second take preserved therm HIGH's observed value, time and read effects. [Apply suite](../../test/test_apply.cpp) |

| Local check | Result |
| --- | --- |
| `cmake -S . -B build/host -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build/host --config Release --parallel 4`, `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 22/22 suites, including both newly registered suites. GCC 15.1.0 and CMake/CTest 4.0.1. |
| `cmake --build build/consumer --config Release --parallel 4` and `build/consumer/tmp11x_native_consumer.exe` | PASS, tests-off consumer linked and printed `0.1.0`. |
| Isolated `TMP11x.h` compile with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -`. Public header XML with `doxygen -` and `WARN_AS_ERROR=YES`. | PASS, Doxygen 1.13.2 indexed all six operations. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and working diff. Python 3.12.10. |

The comparator pages and unchanged sibling revisions in phase a were
rechecked. The fixture uses exact block means and leaves inverted therm
comparisons unspecified. The production API contains no fault queue,
interrupt queue, extended temperature mode or SMBus alert-response command.
Physical ALERT levels, TMP116/N averaged comparator timing, offset/comparator
ordering on supporting parts and concurrent read-clear timing remain open.
MCU, remote CI and physical fixtures were not run. Phase c owns fresh review
of this implementation. Phase d owns stage acceptance and the progress row.

## Phase c review

**PARTIAL, 2026-11-26.** Review began on clean `main` at
`ca6d0912af45eadbd1dae655c9584add897fa40e`. The parent, subject and all
18 changed paths matched the phase-b commit request. The wrapper recorded
upstream equality at 01:06:00 Prague time. Both local refs still matched at
entry. Two independent read-only reviewers examined the limits and alert
paths in parallel. Both finished before the handoff.

The limit review traced the two setters through guarded shutdown, ordered
per-word readback and final CONFIG verification. It checked signed
endpoints, local inversion refusal, partial observations, the three
SD/alert readback failures and recovery. The shared apply engine satisfied
these cases. The alert review traced CONFIG and TEMP effects, first-event
and latest-read attribution, the two takes after a failed one-shot TEMP,
and the GPIO hook. The pinned TI comparator descriptions and limit tables
agree with the literal vectors cited in phase a. The two sibling sources
still matched their recorded pins. [Limit tests](../../test/test_limits.cpp),
[apply tests](../../test/test_apply.cpp), [acquisition tests](../../test/test_acquisition.cpp).

| Finding | Resolution and regression evidence |
| --- | --- |
| An incompatible ID after startup invalidated live state while leaving `possibleLoss` clear. Previously latched but unread alerts on the original sensor could be unreachable. | `identityMismatchBreaksAlertHistory` failed against phase b. The shared invalidation now records the gap before clearing startup proof. Both `probe()` and `recover()` preserve the captured HIGH, generation and timestamp. After a take, checking the same incompatible ID again leaves the loss indication clear. [Implementation](../../src/TMP11x.cpp), [API contract](../api-contracts.md). |
| The eight pin routes used a fixture pin computed from CONFIG. They did not exercise the external ARA vector where a released pin coexists with a latched LOW. | `externalAlertResponse` supplies an independent GPIO level for the documented active-low, threshold-routed case on all six selections. After reported outside access, the pin reads high. The first addressed CONFIG in recovery still observes LOW. A take delivers LOW and possible loss once. Every driver transfer uses the bound sensor address. [Comparator suite](../../test/test_alert_comparator.cpp), [source and vector](../device-behavior.md#comparator-and-pin-evidence). |

| Local check | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4`, then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS after the fix, 22/22 suites with GCC 15.1.0 and CMake/CTest 4.0.1. |
| `cmake --build build/consumer --config Release --parallel 4`, then `build/consumer/tmp11x_native_consumer.exe` | PASS, the tests-off consumer printed `0.1.0`. |
| Isolated `TMP11x.h` compile with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -`, then `doxygen -` on the public headers with XML and `WARN_AS_ERROR=YES` | PASS. Doxygen 1.13.2 indexed all six operations and the revised `possibleLoss` documentation. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and working diff. Python 3.12.10. |

Phase d owns final software acceptance. Physical pin behavior, concurrent
flag-clear timing, TMP116/N block-end comparison and offset/comparator
ordering retain their existing evidence gates. MCU, CI and fixture checks
were not run in this phase.

## Phase d acceptance

**COMPLETE, 2026-11-26.** Software acceptance ran on clean `main` at
`8a2aaf92326c05bda8d516015f25f24a7d3ccbc0`. The parent `ca6d091`,
subject and five changed paths matched the phase-c request. The wrapper
verified equality with `origin/main` at 01:17:04 Prague time. Both local
refs still pointed to that commit at entry. The two sibling source files
listed in phase a still matched their pinned revisions.

The Release tests verified that both continuous-mode setters pass through
guarded shutdown and readback. A failed, mismatched or late SD/alert
readback stopped the write that would have inverted the limit pair.
On all six model selections, an above-limit one-shot followed by a TEMP
timeout preserved the previous sample and delivered HIGH on the first take
only. The external alert-response vector retained LOW after the sampled
pin went high. An incompatible identity preserved captured events and
reported possible loss once.
[Limit tests](../../test/test_limits.cpp), [apply tests](../../test/test_apply.cpp),
[acquisition tests](../../test/test_acquisition.cpp), [comparator tests](../../test/test_alert_comparator.cpp).

| Acceptance check | Result |
| --- | --- |
| `cmake -S . -B build/host -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build/host --config Release --clean-first --parallel 4` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 22/22 suites with GCC 15.1.0 and CMake/CTest 4.0.1. Includes register-ledger validation and its regressions. |
| [Tests-off consumer configuration](../testing.md#source-consumer) in fresh `build/p22d-consumer`, using this checkout and disabling Python and Git discovery. Ran `cmake --build build/p22d-consumer --config Release --parallel 4` and `build/p22d-consumer/tmp11x_native_consumer.exe`. | PASS. The linked consumer printed `0.1.0` and exited successfully. |
| One include per public header with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -`. Ran `doxygen -` on `include/TMP11x` with XML enabled and `WARN_AS_ERROR=YES`. | PASS. All eight headers compiled independently. Doxygen 1.13.2 produced documented public entries for all six limit and alert operations in `build/p22d-docs/xml`. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, local Markdown targets and `git diff --check` | PASS. Python 3.12.10 verified the paths, sizes and hashes of all 14 pinned PDFs and the 55-stage promptset. |

The accepted `limits-alert` artifact supplies the [public operations](../../include/TMP11x/TMP11x.h)
and [retained alert evidence](../../src/DeviceEvidence.cpp) to stages 26 and
27 for diagnostics and snapshots, audit 50, stage 30 for measurement jobs,
and stage 38 for CLI operations. Stages 40–41 own physical ALERT levels and
read-clear timing, TMP116/N comparator timing during averaging and offset
ordering on offset-capable models. MCU links, physical fixtures and remote
CI were not run. Stage 43 owns the first remote CI checkpoint.
