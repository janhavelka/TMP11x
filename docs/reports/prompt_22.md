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
