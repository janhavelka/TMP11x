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
