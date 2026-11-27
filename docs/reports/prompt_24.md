# Prompt 24 — Read-only EEPROM and traceability metadata

## Phase a — Requirements and baseline, 2026-11-27

**PARTIAL.** Prompt 24 is the first unfinished stage in sequence order. All
eight inputs are present on clean `main` at
`365677be60a38d60a4c40857494c56ccdab993c5`. Prompt 23's closing commit
matches its requested subject and four paths, with parent `1cde78c`. The
wrapper recorded equality with `origin/main` at 00:31:09 Prague time and
both refs matched at entry. The public EEPROM methods and snapshot type are
pending. [Stage requirements](../prompts/24_readonly-eeprom.md) and
[sequence](../prompts/SEQUENCE.json).

### Inputs verified

| Artifact (producer) | Evidence at the baseline |
| --- | --- |
| `variant-contract` (4) | The [variant guide](../variants.md#eeprom-shadow-and-traceability) identifies all six BOM selections. The [model descriptor](../../src/ModelDescriptor.h) holds the ordered pointers and word counts. |
| `register-ledger` (5) | The [ledger](../registers/tmp11x.json) records the two physical maps, source conflicts and access policy. Its validator and regression suite passed. |
| `safety-policy` (7) | The [policy](../eeprom-and-reset-policy.md) defines diagnostic reads, persistent unsafe evidence after an external unlock, and prohibited data writes. |
| `register-transport` (12) | [readWord and writeFrame](../../src/RegisterTransport.cpp) enforce one explicit pointer per transfer, decode MSB first, and check writes against the [command table](../../src/CommandTable.cpp). |
| `public-identity-api` (14) | [readIdentity and probe](../../src/TMP11x.cpp) use the [stage 13 engine](../../src/Identity.cpp). A complete mismatch retains the observed ID and invalidates live state. The binding and identity suites passed. |
| `config-trust` (19) | [DeviceEvidence](../../src/DeviceEvidence.cpp) latches EUN and records interrupted guards. [Trust tests](../../test/test_trust.cpp) exercise recovery after external correction. |
| `register-fake` (12) | The [fixture](../../test/support/FakeTmp11xDevice.h) exposes register words, scripted transfer outcomes and callback traces. The [transport suite](../../test/test_register_transport.cpp) already sweeps all pointer bytes and four write purposes on six selections. |
| `audit-20-21` (49) | [Audit acceptance](prompt_49.md#phase-d-acceptance) and the [acquisition suite](../../test/test_acquisition.cpp) cover guard renewal, single-use readiness and late callbacks. [Stage 23 acceptance](prompt_23.md#phase-d-acceptance) verifies typed offset access at the shared pointer. |

### Datasheet and traceability evidence

Each index selects one 16-bit shadow. On the wire the pointer is followed
by a repeated START and two received bytes, high byte first. The snapshot
reads the following pointers individually, in order, after ID and EEPROM_UL.

| Selected BOM model | Shadow pointers for one-based indices | Map and conflicting reset descriptions |
| --- | --- | --- |
| TMP116, TMP116N | 1 → `05`, 2 → `06`, 3 → `07`, 4 → `08` (hex) | Table 3 lists zero for all four words. The detailed entries give unknown, unknown, zero and unknown respectively. [SBOS740A pp. 19–20, 24, 29–31](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24). |
| TMP117, TMP117M, TMP117N | 1 → `05`, 2 → `06`, 3 → `08` | Table 7-3 assigns `07` to Temp_Offset. The overview and EEPROM1 text imply four words. The EEPROM2 heading says zero while the map and field table say unknown. [SNOSD82D pp. 18, 20–21, 25, 30–32](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25). |
| TMP119 | 1 → `05`, 2 → `06`, 3 → `08` | Table 8-2 assigns `07` to Temp_Offset. The four-word prose and EEPROM2 reset heading have the same discrepancies. [SNIS236 pp. 20, 22–23, 32, 37–39](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32). |

EEPROM_UL at `0x04` reports EUN in bit 15 and BUSY in bit 14, which
mirrors CONFIG bit 12. Bits 13:0 are unused. TI documents read-clear
effects on CONFIG and TEMP and specifies none for EEPROM_UL or the data
shadows. A locked data write can still change a volatile shadow. TMP116's
power-up description on p. 17 conflicts with its BUSY tables on pp. 26
and 29, so an idle observation cannot establish startup readiness.
[TMP116 p. 29](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=29),
[TMP117 p. 30](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=30),
[TMP119 p. 37](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=37).

TI describes unique data programmed during manufacture. Its Tier 1B
traceability procedure also relies on the lot code from the shipment label.
An application can bind the known route and BOM model, read the snapshot,
and retain the lot code alongside the ordered observations. Prior external
writes can change what it reads. Every bit pattern, including an all-zero
set, is valid observation data. Factory provenance, physical cell contents
and a calibration certificate require independent evidence.
[TI SBAT024 pp. 2–4, Table 3](../reference/source/NIST-Traceability-for-Temperature-and-Humidity-Sensors-Overview-SBAT024.pdf#page=2).

The 24Cxx `readByte`/`readOnce` path checks range, performs a bounded
addressed read and publishes its scalar after success. Those conventions
apply here. TMP11x has its own pointer map, health-neutral diagnostic
transport and write policy. The sibling's clean HEAD `139e76cd` and the
stage 7 pin match for the public header, implementation and integration guide.
[24Cxx source at the pin](https://github.com/janhavelka/24Cxx/blob/0bb27f46beb4fbe907beb11d9134de4e1797347d/src/EEPROM24Cxx.cpp).

### Phase-b implementation and acceptance

1. Start with `readEepromWord(uint8_t index, uint16_t& out)` on the
   [public driver](../../include/TMP11x/TMP11x.h). Use the existing model
   descriptor and health-neutral transport. A bound, clockless caller may
   read one word with unknown identity. A cached complete mismatch blocks
   the scalar locally. Test all six selections, unbound access, invalid
   indices including zero and count + 1, repeated reads after an outside
   change, asymmetric bytes, all-zero data and a short read. Check the exact
   pointer, one callback on admission, and preserved output on failure.
2. Implement `readEepromSnapshot(EepromSnapshot&)` for a bus-silent bind
   followed by explicit observation. Read `0x0F` even with a cached ID, then
   `0x04`, then the mapped words. A full pass takes six callbacks on TMP116/N
   and five on TMP117/M/N or TMP119. Preserve the selected model, address,
   actual ID, ordered completed words, lock/busy validity and per-read
   optional completion times. The observations span separate transfers.
   On a failed callback return the completed prefix and first failed pointer.
   A complete incompatible ID ends the snapshot after one callback and
   remains available in the result. Local rejection preserves the caller's
   prior snapshot. Test failures at every position, including a short lock
   read, without leaking old or unobserved fields into the new result.
3. Exercise lock words `0x0000`, `0x4000`, `0x8000`, `0xC000` and
   reserved-bit noise. A diagnostic reports BUSY once and continues observing
   shadows. A complete EUN observation latches the existing unsafe condition
   immediately, even if a later word fails. A later diagnostic zero leaves
   guarded mutation and freshness blocked until owner correction and
   invalidation. Diagnostic reads confer no write authority. Check neutral
   health on both success and failure, clockless timestamps and a valid tick
   of zero. The neutral transport currently omits completion time. Preserve
   READY/ALERT evidence by avoiding CONFIG and TEMP throughout the snapshot.
4. Extend the private write-policy tests with varied payloads including
   relock, unlock and all bits set. Check **zero callbacks** to EEPROM_UL
   or any mapped shadow on denied write attempts, including TMP116/N
   `0x07`. On the other four selections only guarded managed-offset intent
   may write `0x07`. Trace existing begin, temperature read and EEPROM
   routes at the bound seven-bit address. Check shadow contents across each
   trace, zero protected data writes, zero general calls and zero diagnostic
   reset attempts. Use the callback trace to detect even a failed attempt.

Add the focused public cases to the [host test entrypoint](../testing.md),
document both methods and snapshot failure semantics in public Doxygen, and
update the user guide and [feature coverage](../feature-coverage.md) for
R13–R15, F13, the EEPROM portion of F15, and X01/X02/X04/X06. Stage 26
integrates diagnostic dumps, stage 28 adds occupied-job admission, and
stages 37/38 exercise CLI observation and mutation boundaries. Physical
observation belongs to stage 40. [Public contracts](../api-contracts.md),
[owner limits](../owner-operations.md) and [handoffs](../prompts/HANDOFFS.md).

### Phase-a checks

- `python scripts/check_references.py` and `python scripts/check_promptset.py`
  passed with Python 3.12.10. All 14 pinned PDFs and the 55-stage manifest
  passed their respective checks.
- `python tools/check_register_ledger.py` and
  `python tools/test_check_register_ledger.py` passed on six BOM selections
  and all 31 ledger regressions.
- `cmake --build build/host --config Release --parallel` and
  `cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(value_types_cpp|register_transport_cpp|identity_cpp|binding_cpp|health_cpp|apply_cpp|trust_cpp|acquisition_cpp)$'`
  passed with CMake/CTest 4.0.1 in the existing MinGW Release build. All
  eight prerequisite suites passed. Public EEPROM tests await phase b.

Two parallel read-only assessments checked the pinned silicon sources and
the transport, tests and sibling precedent. Phase b has no software blocker.
Physical and remote CI checks were not run. Stage 43 owns the first CI
checkpoint.

## Phase b implementation

Phase b implemented `readonly-eeprom` on clean `main` at input revision
`7761b4ed4d9bab482c8902810ebd80d6f17fadc8`. Git history, the prior commit
request and the wrapper log agree on the phase-a report commit and upstream
synchronization. The eight inputs listed above were present. The exact-part
maps, EEPROM_UL definitions, locked shadow-write behavior and TI shipment
lot guidance were rechecked in the pinned sources. The 24Cxx checkout
remained clean at `139e76cd`.

The [public methods and snapshot types](../../include/TMP11x/TMP11x.h)
and [implementation](../../src/TMP11x.cpp) reuse the selected model's
pointer table, bound identity diagnostic and checked neutral transport.
A scalar publishes only a complete word. A snapshot publishes each
complete observation, including an incompatible ID, with a separate
optional tick. An unsuccessful scan identifies the first failed pointer.
During the snapshot, only complete observations sample the optional clock.
The [evidence owner](../../src/DeviceEvidence.cpp) retains an observed
unlock immediately and handles observed BUSY from EEPROM_UL and CONFIG
consistently. A diagnostic lock read can revoke a guard, while every
later guarded operation still obtains its own lock word.
[Using the API](../../README.md#read-eeprom-traceability-metadata).

| Host suite and named cases | What the assertions verified |
| --- | --- |
| [eeprom_cpp](../../test/test_eeprom.cpp), `scalarBoundsAndFrames`, `snapshotsAndPartialResults` | All six BOM selections with literal frame order, MSB-first bytes and timeout. Bounds, externally changed and zero words, local output preservation, five/six-read limits, short transfers at every position, absent clock, valid tick zero and independent completion times. |
| `identityChangesBetweenObservations`, `snapshotsPreserveAlertsAndRespectBusy`, `unlockEvidenceSurvivesPartialSnapshot` | ID-first mismatch and cache recovery, scalar mismatch refusal, retained alert and ready evidence, neutral success and failure, all EUN/BUSY combinations and unused-bit noise. A failed later shadow preserved EUN. A subsequent locked observation left freshness and recovery blocked until explicit invalidation. |
| [register_transport_cpp](../../test/test_register_transport.cpp), `protectedWritesDenyEveryPayload`, `publicRoutesPreserveEeprom` | All 65,536 payloads × four write purposes × 26 protected model/pointer pairs returned UNSUPPORTED with zero callbacks. Callback traces for begin, latest, scalar and snapshot used only the bound address, preserved all seeded shadow words and attempted no unlock, relock or reset. Existing typed-offset gates remained in force. |
| [acquisition_cpp](../../test/test_acquisition.cpp), `diagnosticBusyBeforeContinuousTemp` | BUSY observed by an EEPROM snapshot between acquisition steps revoked the prior guard. The untimed path stopped before TEMP. The timed path issued a new guard read and required fresh readiness. |

| Local command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 6` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 23/23 including the six-model ledger and its 31 regressions. CMake/CTest 4.0.1, GNU C++ 15.1.0, Python 3.12.10. |
| `doxygen -`, input `include/TMP11x`, XML output in `build/p24b-docs` with `WARN_AS_ERROR=YES` | PASS with Doxygen 1.13.2. |
| `python scripts/check_references.py`, `python scripts/check_promptset.py`, `git diff --check` | PASS, 14 PDF hashes, 55 stages and the working diff. |

Phase c reviews the implementation and tests from a fresh session. Stage 26
owns general register dump integration, stage 28 occupied-job rejection,
and stages 37/38 the CLI. MCU, physical and remote CI checks were not run.
Physical observation belongs to stage 40 and the first remote checkpoint
to stage 43. The wrapper owns the staged diff, commit and push.

## Phase c review

Reviewed the 14-path implementation at `b29b6c48f01501903fb5b0bfba7963ff86eb77fe`
on clean `main`. Its parent, subject and paths matched the phase-b request.
The wrapper recorded synchronization at 01:03:16 Prague time, and the
local upstream ref matched at entry. The eight inputs listed in phase a
were available. Two parallel read-only reviewers examined runtime evidence
and the physical maps, write policy, tests and documentation.

**Freshness correction.** After a CONFIG read retained DATA_READY, an
untimed fresh read could observe BUSY at EEPROM_UL and return not-ready.
When BUSY cleared, the next request could publish the previously cached
conversion. The new public regression failed on that second request at
the review baseline. A snapshot observing the same BUSY word already
discarded cached readiness. All complete EEPROM_UL observations now use
the same BUSY handling and completion tick. EUN still takes precedence
when both bits are set. [Evidence handling](../../src/DeviceEvidence.cpp).

`busyGuardDiscardsCachedReady` covers both an untimed fresh read and
recovery interrupted on its guard reread before any write. In both cases
the next idle attempt reads only EEPROM_UL and CONFIG, returns
`MEASUREMENT_NOT_READY` and preserves the caller's output. A new conversion
then produces the trace `R04 R01 R00` and returns 25 °C.
[Acquisition regression](../../test/test_acquisition.cpp).

**Snapshot and write policy.** The ID-first scan, mismatch stop, optional
timestamps including tick zero, partial outputs, immediate EUN retention
and neutral transport accounting matched the public contract. The README
now distinguishes local refusal, which preserves the previous output,
from a scan that stops after I²C starts. The pinned map and traceability
pages cited in phase a agree with the six-model pointer and byte-order
tests. The protected-write sweep and public traces cover both meanings
of `0x07`. [EEPROM tests](../../test/test_eeprom.cpp),
[transport tests](../../test/test_register_transport.cpp),
[application guide](../../README.md#read-eeprom-traceability-metadata).

The private apply engine relies on exclusive ownership between steps.
Interposing a diagnostic manually after a safe guard would violate that
assumption. Both public callers drive the engine synchronously to completion.
Stage 28 owns refusal of unrelated diagnostics while a job or result occupies
the slot. Stage 29 exercises that gate with stepped configuration.
[Apply engine](../../src/Apply.h), [admission contract](../prompts/28_cooperative-admission.md),
[configuration jobs](../prompts/29_cooperative-config.md).

| Local check | Review result |
| --- | --- |
| `cmake -E chdir build/host ctest -C Release --output-on-failure -R '^acquisition_cpp$'` with the new regression before the fix | FAIL at the second fresh request after BUSY cleared. |
| `cmake --build build/host --config Release --parallel 6` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 23/23 suites, including acquisition, apply, EEPROM and the 6,815,744 protected-write refusals. CMake/CTest 4.0.1, GNU C++ 15.1.0. |
| `doxygen -` on `include/TMP11x` with XML and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. The generated snapshot entry documents the effect of BUSY on cached readiness. |
| `python scripts/check_references.py`, `python scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and the working diff. Python 3.12.10. |

Phase d owns final software acceptance and the progress entry. MCU, physical
and remote CI checks were outside this review. Physical observation belongs
to stage 40 and remote CI to stage 43.

## Phase d acceptance

**COMPLETE, 2026-11-27.** Accepted `readonly-eeprom` on clean `main` at
`0a0ac78c4d2cea5fddc66f30b9469a0b1f9afc3f`. Its parent was `b29b6c4`.
The subject and nine paths matched the phase-c request. The wrapper verified
upstream equality at 01:17:23 Prague time, and both refs matched at entry.

| Exit criterion | Evidence from the final Release run |
| --- | --- |
| Bounds, pointer order and repeated reads | Across all six models, `scalarBoundsAndFrames` rejected indices zero, count + 1 and 255 before I²C. It read changed and zero values from the fixture. TMP116/N included EEPROM3 at `0x07`. TMP117/M/N and TMP119 skipped that pointer. [EEPROM tests](../../test/test_eeprom.cpp). |
| Snapshot, lock and BUSY | The ID-first scan used six reads on TMP116/N and five on the other selections. Faults at each position preserved the completed prefix. The lock tests covered all EUN/BUSY combinations, including an unlock followed by a failed shadow read. `busyGuardDiscardsCachedReady` required a new conversion after BUSY discarded cached readiness. [EEPROM tests](../../test/test_eeprom.cpp), [acquisition tests](../../test/test_acquisition.cpp). |
| Prohibited writes and preservation of shadow data | `protectedWritesDenyEveryPayload` refused 6,815,744 attempts before any callback. `publicRoutesPreserveEeprom` traced begin, latest and both EEPROM APIs at the bound address. Each trace preserved the seeded words and contained zero protected writes, unlocks or reset commands. [Transport tests](../../test/test_register_transport.cpp). |

The pinned register maps, lock definitions and shipment-label guidance in
phase a were rechecked. The API reports non-atomic observations of volatile
shadows. An outside write can change those observations even with EEPROM
locked. The application guide covers recording the ordered words and lot
code together. [Traceability guide](../../README.md#read-eeprom-traceability-metadata).

| Acceptance check | Result |
| --- | --- |
| `cmake -S . -B build/host -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build/host --config Release --clean-first --parallel 4`, `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 23/23 suites with GCC 15.1.0 and CMake/CTest 4.0.1. |
| [Tests-off consumer configuration](../testing.md#source-consumer) with the MinGW generator in fresh `build/p24d-consumer`, then `cmake --build build/p24d-consumer --config Release --parallel 2` and `build/p24d-consumer/tmp11x_native_consumer.exe` | PASS, linked version `0.1.0`. |
| One include per header with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -`, then `doxygen -` on `include/TMP11x` with XML and `WARN_AS_ERROR=YES` | PASS, all eight headers compiled. Doxygen 1.13.2 emitted both EEPROM method entries, including bounds, partial results and BUSY effects. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and the acceptance diff. Python 3.12.10. |

Stage 26 owns diagnostic dump integration, stages 37/38 own CLI enforcement
and stage 40 owns physical observation. MCU, fixture and remote CI checks
were not run at this software checkpoint. Stage 43 owns remote CI.
