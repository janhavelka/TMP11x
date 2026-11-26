# Prompt 23 — Opt-in hardware temperature offset

## Phase a — Requirements and baseline, 2026-11-26

**PARTIAL.** Phase a closes on clean input `main` at
`5169ad6efebd4bccee129a5bcabc2475d11db912`. Prompt 22's closing commit
has the requested subject and four paths, with parent `8a2aaf9`. The wrapper
verified equality with `origin/main` at 01:30:41 Prague time, and both local
refs matched at entry. Prompt 23 is the next software stage. Its deliverable,
`offset-feature`, remains pending. [Stage requirements](../prompts/23_offset-feature.md)
and [sequence](../prompts/SEQUENCE.json).

### Inputs verified at the baseline

| Consumed artifact (producer) | Source and existing evidence |
| --- | --- |
| `variant-contract` (4) | [BOM model selections](../variants.md) and [command table](../../src/CommandTable.cpp) distinguish four offset-capable models and two EEPROM3 models. |
| `register-ledger` (5) | [Register map](../registers/tmp11x.json) identifies access policy and reset domain for each selection. |
| `safety-policy` (7) | [Startup and EEPROM rules](../eeprom-and-reset-policy.md) and [transport](../../src/RegisterTransport.cpp) restrict offset writes to managed intent with startup and locked-idle proof. |
| `temperature-codec` (16) | [Encoder](../../src/Temperature.cpp) and [signed-word tests](../../test/test_temperature.cpp) cover finite range, sign and rounding. |
| `config-trust` (19) | [DeviceEvidence](../../src/DeviceEvidence.cpp) and [trust tests](../../test/test_trust.cpp) track observations, attempted writes and drift for CONFIG and both limits. |
| `sync-acquisition` (21) | [Acquisition](../../src/Acquisition.cpp) establishes fresh-conversion evidence. [LatestRead](../../src/LatestRead.cpp) decodes TEMP and gates output/cache publication on timely success. |
| `safe-apply-engine` (18) | [Apply](../../src/Apply.cpp) and [replay tests](../../test/test_apply.cpp) establish startup, read the guard, verify shutdown and limits, consume baseline flags and verify final CONFIG. |
| `behavior-vectors` (6) | [Literal vectors](../registers/behavior-vectors.json), especially `host_quantization` and `offset_result`, supply the expected wire words and saturated TEMP observations. |
| `register-fake` (12) | [Fixture](../../test/support/FakeTmp11xDevice.cpp) records bus frames, can ignore an acknowledged write or apply a failed one, and currently places the block mean directly into TEMP. [Transport tests](../../test/test_register_transport.cpp). |
| `audit-20-21` (49) | [Acceptance](prompt_49.md#phase-d-acceptance) covers recovery, single-use readiness, one-shot guard renewal and deadline clipping. [Prompt 22](prompt_22.md#phase-d-acceptance) subsequently checked the shared apply and alert paths. |

### Silicon and codec evidence

TMP116 and TMP116N map `0x07` to EEPROM3. A typed offset read, setter or
managed profile (including zero) must return `UNSUPPORTED` before any bus
callback. The same pointer remains available to a checked EEPROM shadow
reader under its own policy. [SBOS740A, pp. 24, 30](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=30).

TMP117/M/N and TMP119 use a signed 16-bit offset at `0x07`, with 1/128 °C
per count. The factory value is zero. The map footnotes allow a programmed
power-up value, so the driver must read the live calibration. The device adds
the offset after linearization and saturates TEMP at the signed endpoints.
The startup prose names CONFIG and limit writes during EEPROM load and leaves
the OFFSET write response unspecified. Comparator ordering and addressed-reset
effects remain open. [SNOSD82D, pp. 18, 25, 31 §7.6.9](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=31),
[SNIS236, pp. 20, 32, 38 §8.5.9](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=38).

The setter can reuse `temperature::encodeCelsius`. Accept −256 through
+255.9921875 °C inclusive, round half counts away from zero, and reject
nonfinite and out-of-range requests before I/O. The TMP1x2 measurement codec
uses the same rounding convention with a different 1/16 °C unit. Its source
at clean local HEAD `0791429f` matched the project pin `afd5088`. Test wire
values against independent TMP11x literals. [TMP1x2 codec at pin](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2Measurement.cpp).

### Phase-b implementation and acceptance

1. **Offset observation and ownership.** Extend the shared observation path,
   snapshots and field masks to carry the signed OFFSET word on capable parts.
   Connect `readTemperatureOffset(int16_t&)` and the typed setter
   `setTemperatureOffset(double)`. The setter opts into management.
   `SensorSettings::managedOffsetCounts` is an optional signed count,
   disengaged by default. Preserve nonzero startup calibration when unmanaged
   or releasing ownership. An engaged zero must write, verify and recover zero.
   Read supported OFFSET during full verification and record completed words
   on partial failure. [Public contract](../api-contracts.md),
   [settings](../../include/TMP11x/Config.h), [public entry points](../../src/TMP11x.cpp).
2. **Guarded apply and recovery.** Exercise begin, settings changes and
   explicit recovery through the shared apply engine. Before the first
   mutation preserve the old desired target. From that attempt retain the
   candidate and dirty evidence until full readback succeeds. In verified
   shutdown, observe unmanaged OFFSET or write and verify the managed word,
   then establish the baseline before resuming continuous conversions. Check
   zero OFFSET writes with missing startup proof, EUN set, or persistent BUSY.
   Trace BUSY clearing before a permitted write. Inject an ambiguous write,
   an acknowledged ignored write and readback failure. Verify recovery from
   a nonfactory starting word with both ownership choices. Limit and CONFIG
   reads retain their existing effects. [Owner contract](../owner-operations.md).
3. **Unexplained drift.** Start verified and continuous with unmanaged zero
   and retained DATA_READY. A complete `0x0040` OFFSET observation returns
   +64 counts to the typed reader. Retain that word as evidence while
   invalidating initialization, startup proof and readiness. A verification
   detecting that change reports mismatch. The next fresh request must make
   zero transfers. After ownership is restored, recover, observe +64 without
   writing OFFSET and consume stale readiness at the baseline. Attribute +64
   only to a subsequent confirmed conversion. Put this rule in the shared
   observation path for stage 26's diagnostic reader.
4. **Sample attribution and saturation.** Preserve historical samples across
   offset changes. A latest read of unchanged TEMP cannot establish the new
   attribution. Exercise observed unmanaged and verified managed counts on
   fresh samples, and unavailable offset on TMP116/N. An expired successful
   callback leaves caller output and cached sample untouched. Decode the
   returned TEMP once. Check +255 °C with +2 °C gives `0x7FFF`, and −255 °C
   with −2 °C gives `0x8000` (numeric −256 °C on confirmed completion).
   Include the ordinary vector 3200 counts with offset `0xFF80` giving
   `0x0C00`. Supply literal TEMP/ready observations without prescribing a
   comparator result. [Sample type](../../include/TMP11x/TMP11x.h),
   [acquisition tests](../../test/test_acquisition.cpp), [latest tests](../../test/test_latest.cpp).

Phase b starts with the typed unmanaged reader, shared observation storage,
model gate and tests for zero callbacks on TMP116/N and unexplained drift on
capable parts. Introduce writable management together with guarded replay and
readback. Update existing UNKNOWN-provenance assertions as their paths gain
evidence. Register focused tests in CMake and update Doxygen, user guidance
and coverage for R16, F10, F11 and F17. Describe volatile calibration and its
accuracy limits. Stage 25 owns addressed reset, 26 diagnostics, 29 cooperative
replay, 38 CLI integration and 41 physical evidence. [Handoffs](../prompts/HANDOFFS.md).

### Phase-a checks

- `python scripts/check_references.py` passed with Python 3.12.10. All 14 pinned PDFs matched their sizes and SHA-256 hashes.
- `cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(register_transport_cpp|temperature_cpp|configuration_cpp|apply_cpp|trust_cpp|acquisition_cpp|latest_cpp|limits_cpp|register_ledger)$'` passed all nine prerequisite cases in the existing Release build with CMake 4.0.1.
- Two read-only reviews checked the pinned silicon sources and the producer artifacts. No software prerequisite is blocked. Feature acceptance and physical observations remain pending. Stage 43 owns the first remote CI checkpoint.

## Phase b — Unmanaged offset readback

**PARTIAL.** This slice started on clean `main` at
`3e11f374fa202c46419eba13a6b80513567dec4f`. Its parent, subject and sole
report path match the phase-a request. The wrapper confirmed upstream
equality at 01:45:03 Prague time.

The [typed reader](../../src/TMP11x.cpp) reads one tracked word on an
initialized TMP117/M/N or TMP119. The model's command table gates access
before any transfer on TMP116/N. The [shared observation path](../../src/DeviceEvidence.cpp)
records raw bits, signed counts and completion time. A short read preserves
the prior observation and caller output. An unexplained change clears live
device state and retained readiness while preserving historical verification
and the newly observed offset.

The [trust suite](../../test/test_trust.cpp) adds `typedOffsetObservations`
for all six BOM selections and `unexplainedOffsetDriftInvalidatesReadiness`.
With DATA_READY already captured, an offset change from zero to `0x0040`
returned +64 counts. The subsequent fresh read refused before I2C. Recovery
renewed the startup wait and a fresh read still required a new conversion.

- `cmake --build build/host --config Release --parallel` passed with CMake 4.0.1.
- `cmake -E chdir build/host ctest -C Release --output-on-failure` passed all 22 registered tests.
- `python scripts/check_references.py` verified all 14 pinned PDFs.

The first typed read currently establishes the offset baseline. Next, read
supported unmanaged OFFSET during guarded apply/recovery and explicit profile
verification. Cover a nonfactory startup value, a partial verification scan,
and drift detected during verification. Managed writes, explicit zero,
recovery of owned calibration and conversion attribution remain later phase-b
work. Remote CI and physical trials remain at stages 43 and 41.

## Phase b — Unmanaged offset in replay and verification

**PARTIAL.** Clean input `a8853af13af71a1d1bc2d9ae366dff952f9fd812`
matched the previous request's parent, subject and ten paths. The wrapper
verified upstream equality at 02:00:38 Prague time.

[Apply](../../src/Apply.cpp) reads OFFSET after both verified limits in
shutdown, before enabling THERM or consuming baseline CONFIG. Full
[verification](../../src/TMP11x.cpp) reads OFFSET fourth on TMP117/M/N and
TMP119. Both paths skip pointer `0x07` on TMP116/N. A short fourth read
preserves the earlier calibration, its timestamp and the completed scan
prefix. A complete changed word invalidates initialization and remains in the
snapshot alongside the last successful verification time. Recovery establishes
a new offset baseline once the owner restores exclusive access.

- In `apply_cpp`, `allModelsBeginRecoverLatest` checks exact wire frames for all six models. `recoveryAfterReportedReset` preserves the new power-up calibration. `interruptedOffsetObservationInShutdown` stops on a short or changed word before resuming conversions.
- `offsetObservedBeforeIncompleteInitialization` checks that a successful calibration read still establishes continuity when the following CONFIG read fails. A changed OFFSET on retry reports drift.
- In `trust_cpp`, `shortOffsetVerificationRetainsPriorCalibration` checks the failed fourth read. `unexplainedOffsetDriftInvalidatesReadiness` exercises typed and verification reads, bus-silent fresh refusal and recovery with zero OFFSET writes even if calibration changed again.

CMake 4.0.1 built the Release tree with `cmake --build build/host --config Release --parallel`.
`cmake -E chdir build/host ctest -C Release --output-on-failure` passed all 22 suites.
With Python 3.12.10, `python scripts/check_references.py` verified all 14 pinned PDFs
and `python scripts/check_promptset.py` passed all 55 stages. `git diff --check` passed.

Next phase-b slice connects the optional desired count and typed setter to
guarded volatile write/readback and recovery, including explicit zero and
TMP116/N refusal. Sample attribution and saturation vectors follow.

## Phase b — Managed offset writes and recovery

**PARTIAL.** Clean input `44367d1028dcf0334063fd162a8ff90bc764eb51`
matches the prior 15-path request. The wrapper recorded upstream equality at
02:15:57 Prague time. The source check reconfirmed TMP116 EEPROM3 at SBOS740A
p. 30 and signed OFFSET at SNOSD82D p. 31 and SNIS236 p. 38. The clean
TMP1x2 sibling codec at `0791429f` still matches its pin for that file.

`SensorSettings::managedOffsetCounts` defaults to disengaged. An engaged
value or a successful `setTemperatureOffset(celsius)` opts into management.
After the shared startup and EEPROM checks, replay verifies shutdown and both
limits, writes the selected counts to OFFSET and reads them back before the
CONFIG baseline. A managed mismatch retains the observed word and marks the
target dirty. An ambiguous write discards the previous OFFSET observation.
Recovery retains the desired counts, including zero. Releasing ownership in
the settings engine preserves the live word. [Apply](../../src/Apply.cpp),
[register evidence](../../src/DeviceEvidence.cpp).

The [apply tests](../../test/test_apply.cpp) exercise all six model selections.
`managedOffsetApplyRecoverRelease` traces busy-to-clear, explicit zero over
nonfactory calibration, recovery after reload, negative half-count rounding,
managed drift and release. `managedOffsetWriteFailures` injects a write that
took effect despite a NACK, an ignored ACK and a short readback. Each stops
in shutdown and recovers the retained value. `managedOffsetRefusals` and
`busyAndUnlockedGuards` check invalid Celsius, zero-transfer model refusal
and startup, unlock and persistent-busy guards.

- CMake 4.0.1: `cmake --build build/host --config Release --parallel` passed.
- `cmake -E chdir build/host ctest -C Release --output-on-failure` passed all 22 suites.
- Python 3.12.10: `python scripts/check_references.py` verified 14 PDFs and `python scripts/check_promptset.py` passed 55 stages. `git diff --check` passed.

Next phase-b slice connects offset provenance to sample publication. Check a
latest read before a new conversion, a confirmed conversion after a managed
change or unmanaged drift/recovery, and outputs/cache after late success.
Exercise the ordinary and both saturated TEMP vectors from the phase-a brief.
Stage 27 owns the public full-profile configure entry point. Physical trials
and remote CI retain their stage 41 and 43 gates.

## Phase b — Sample offset attribution

Clean baseline `ae906178adef69078c3fd25ca768a8690cad1dcb` matches the prior
request's subject and 17 paths. The wrapper recorded upstream equality at
02:31:23 Prague time. The pinned TMP117 p. 31 and TMP119 p. 38 specify
saturation after adding the offset. TMP116 p. 30 assigns this address to EEPROM3.

[Sample publication](../../src/LatestRead.cpp) uses the offset observed during
profile verification and the existing fresh-conversion evidence. It adds no bus
transactions. A timely continuous or one-shot read publishes the signed counts
with their ownership. A latest read on a capable model leaves calibration
unknown. TMP116/N reports unavailable on every successful sample.

- [Acquisition tests](../../test/test_acquisition.cpp): `offsetResultVectors`
  injects the ordinary 24 °C TEMP result, both saturated endpoints for ±255 °C
  with ±2 °C offset, and both extreme-offset ledger vectors on all four
  capable selections. `oneShotWireAndTiming` checks verified managed zero.
  `lateContinuousTransfers` keeps the historical sample and caller output
  across late CONFIG and TEMP success after an offset change.
- [Trust tests](../../test/test_trust.cpp): `unexplainedOffsetDriftInvalidatesReadiness`
  verifies stale readiness is consumed on recovery. The same TEMP word carries
  +64 or −128 counts of unmanaged calibration only after a new ready event.
- CMake 4.0.1: `cmake --build build/host --config Release --parallel` passed.
  `cmake -E chdir build/host ctest -C Release --output-on-failure` passed all 22 suites.
- Python 3.12.10: `python scripts/check_references.py` verified 14 pinned PDFs.
  `python scripts/check_promptset.py` and `git diff --check` passed.

Phase b ends here. Phase c reviews the full stage diff from `5169ad6` and
its tests against the source and model gates. Physical arithmetic and
offset/comparator ordering remain open for stage 41. Stage 43 owns remote CI.

## Phase c review

**PARTIAL, 2026-11-27.** Reviewed `5169ad6..c85fffe` on clean `main`.
The phase-b request matched the subject and all 12 paths of
`c85fffe8ea7209be9f224385baff1d7beb650bd2`, whose parent is `ae90617`.
The wrapper recorded upstream equality on November 26 at 02:45:41 Prague time.
Both local refs still matched at review entry.

Two independent read-only reviewers traced profile ownership and sample
publication. Both finished during this phase. The review covered all six
model gates, startup and EEPROM guard refusals, explicit zero, release,
ambiguous writes, unmanaged drift, single-use readiness and preservation of
historical samples after late callbacks. The original TI pages cited in
phase a confirm the model-specific role of `0x07`, its power-up source and
saturation of the signed TEMP result. The clean TMP1x2 comparison codec
at `0791429f` still matches its pinned file.

| Review finding | Fix and regression evidence |
| --- | --- |
| A setter could overwrite an externally changed managed offset and report success. | With a verified target of zero and a live value of +64 counts, `managedOffsetDriftBeforeSetter` reproduced the failure by requesting +64. Settings changes now compare the live word with the previous owned target before the first write. The trace ends at `R0F R04 R01 R03 R02 R07` with `CONFIG_MISMATCH`. The snapshot retains desired zero and observed +64. Explicit recovery restores zero. [Apply engine](../../src/Apply.cpp), [regression](../../test/test_apply.cpp). |
| The synthetic ADC path published the uncorrected mean on a calibrated TMP117. | `offsetResultVectors` failed for 3200 input counts and a −128-count offset. The fixture now adds the signed offset to the completed mean and clamps the sum. On all four capable selections the driver returned 24 °C for this vector and the expected signed endpoints for both saturation directions. The test supplies comparator flags independently. Nonzero EEPROM3 leaves TMP116/N conversions unaffected. [Fixture](../../test/support/FakeTmp11xDevice.cpp), [acquisition tests](../../test/test_acquisition.cpp). |
| The exact deadline was untested for an unmanaged OFFSET read that exposed drift in shutdown. | Extended `interruptedOffsetObservationInShutdown`. Physical success at expiry retains the changed word and its completion tick, invalidates initialization and leaves the previous verification generation intact. The call returns `OPERATION_TIMEOUT` and the trace ends at the OFFSET read. [Apply tests](../../test/test_apply.cpp). |

- `cmake --build build/host --config Release --parallel` and `cmake -E chdir build/host ctest -C Release --output-on-failure` passed all 22 suites after the fixes with GCC 15.1.0 and CMake/CTest 4.0.1.
- `doxygen -` with public-header XML output and `WARN_AS_ERROR=YES` passed with Doxygen 1.13.2. The generated setter entry includes the pre-write mismatch behavior.
- `python scripts/check_references.py` verified 14 pinned PDFs. `python scripts/check_promptset.py` passed 55 stages with Python 3.12.10. `git diff --check` passed.

Phase d owns final software acceptance. Stage 26 integrates diagnostic
reads and stage 27 exposes full-profile configuration. Physical saturation
and comparator ordering remain open for stage 41. Stage 43 owns remote CI.

## Phase d acceptance

**COMPLETE, 2026-11-27.** Accepted `offset-feature` on clean `main` at
`1cde78c58a522a1aa8e7a1ed978b782d3b8c486c`. Its parent `c85fffe`, subject
and 11 paths matched the phase-c request. The wrapper verified upstream
equality at 00:18:08 Prague time, and both local refs matched at entry.
The TMP1x2 comparison codec at clean `0791429f` still matched its pin.
The pinned TMP116 register map and EEPROM3 description (pp. 24, 30), TMP117
offset map and arithmetic (pp. 25, 31), and TMP119 equivalents (pp. 32, 38)
were rechecked against the phase-a sources.

| Software criterion | Passing Release evidence |
| --- | --- |
| Model gate, startup and EEPROM guard | `typedOffsetObservations` and `managedOffsetRefusals` verified zero callbacks on TMP116 and TMP116N, including an explicit zero request. With startup unproven, only the ID read occurred. `busyAndUnlockedGuards` verified zero writes with EUN set or persistent BUSY. On all four capable selections, `managedOffsetApplyRecoverRelease` traced BUSY clearing before the first write. [Trust](../../test/test_trust.cpp), [apply](../../test/test_apply.cpp). |
| Calibration ownership and recovery | Unmanaged recovery observed the nonfactory word in shutdown. Managed zero was written, read back and restored after reload. `managedOffsetWriteFailures` retained dirty intent after an applied NACK, ignored ACK or short readback. `managedOffsetDriftBeforeSetter` caught a changed live word before the first write and recovered the previous target. [Apply tests](../../test/test_apply.cpp). |
| Drift and conversion attribution | `unexplainedOffsetDriftInvalidatesReadiness` observed unmanaged drift from zero to +64 counts with DATA_READY retained. The next fresh request made zero transfers. Recovery consumed stale readiness and preserved calibration. Only a new conversion carried the new counts. `lateContinuousTransfers` preserved both caller output and the cached sample after late CONFIG or TEMP success. [Trust](../../test/test_trust.cpp), [acquisition](../../test/test_acquisition.cpp). |
| Signed input and reported TEMP | The setter rejected nonfinite and out-of-range inputs before I²C and rounded a negative half-count to −1. `offsetResultVectors` verified 25 °C with −1 °C calibration returned 24 °C. The simulated +255 °C with +2 °C and −255 °C with −2 °C returned `0x7FFF` and `0x8000` on all four capable models. The confirmed lower endpoint decoded to −256 °C. [Apply](../../test/test_apply.cpp), [acquisition](../../test/test_acquisition.cpp). |

| Acceptance check | Result |
| --- | --- |
| `cmake -S . -B build/host -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build/host --config Release --clean-first --parallel 4`, `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 22/22 suites including the register ledger and regressions. GCC 15.1.0 and CMake/CTest 4.0.1. |
| [Tests-off consumer configuration](../testing.md#source-consumer) with the MinGW generator in fresh `build/p23d-consumer`, then `cmake --build build/p23d-consumer --config Release --parallel 4` and `build/p23d-consumer/tmp11x_native_consumer.exe` | PASS, linked version `0.1.0`. |
| One include per header with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -`, then `doxygen -` on `include/TMP11x` with XML and `WARN_AS_ERROR=YES` | PASS, all eight public headers compiled independently. Doxygen 1.13.2 documented both operations, desired and observed offset, and sample provenance. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and the acceptance diff. Python 3.12.10. |

The [public operations and snapshots](../../include/TMP11x/TMP11x.h),
[guarded replay](../../src/Apply.cpp), [offset observations](../../src/DeviceEvidence.cpp)
and [sample publication](../../src/LatestRead.cpp) supply stages 25–27, audit
50 and stages 29, 32 and 38. Stage 41 owns physical saturation and evidence
for comparator ordering. MCU links, physical measurements and remote CI were
not run in this software checkpoint. Stage 43 owns remote CI.
