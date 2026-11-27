# Prompt 25 — Per-device reset and invalidation

## Phase a — Requirements and baseline, 2026-11-27

Stage 25 is open. Clean `main` and its local upstream `origin/main` both
pointed to `d80da0faf4ffeb39a65ca2092026b8b964ebe2f8`. The parent,
subject and three changed paths matched the stage 24 closure request. The
wrapper recorded upstream equality at 01:29:54 Prague time. In manifest
order, 25 follows 24. All eight direct prerequisites have COMPLETE rows
and acceptance reports. The deliverable for phase b is the explicit,
synchronous `reset()` operation. [Prompt](../prompts/25_per-device-reset.md),
[progress ledger](../prompts/PROGRESS.md).

| Input | Evidence at the baseline |
| --- | --- |
| Stages 5 and 7, register ledger and `safety-policy` | The [device ledger](../registers/tmp11x.json), [six-model matrix](../variants.md#model-selection-and-capabilities) and [reset policy](../eeprom-and-reset-policy.md#addressed-reset-and-recovery) already record the model split, encoding and unknown effects. |
| Stages 9 and 14, `owner-contract` and `binding-api` | The [owner contract](../owner-operations.md) defines the single deadline and clock resolution. The [driver](../../src/TMP11x.cpp) owns the selected binding, live state and retained history. |
| Stages 18 and 19, `sync-initialize-recover` and `config-trust` | [Apply](../../src/Apply.cpp) establishes FIRST_ACK, waits, guards and verifies replay. [DeviceEvidence](../../src/DeviceEvidence.cpp) records complete and uncertain read effects. External invalidation is already bus-silent. |
| Stages 21 and 23, `sync-acquisition` and `offset-feature` | [Acquisition](../../src/Acquisition.cpp) tracks readiness and owned triggers. [Offset recovery cases](../../test/test_apply.cpp) exercise managed replay and preservation of unmanaged calibration. |
| Stage 12, `register-fake` | The [fixture](../../test/support/FakeTmp11xDevice.cpp) records frames and injects faults. The [transport tests](../../test/test_register_transport.cpp) already exercise protected pointers. The [checked transport](../../src/RegisterTransport.cpp) currently admits stable CONFIG and one-shot CONFIG writes only at the CONFIG pointer. |

### Silicon decision

| Selected BOM model | Addressed reset evidence |
| --- | --- |
| TMP116, TMP116N | CONFIG bits 1:0 are unused. Return `UNSUPPORTED` before the first callback. The POR description says BUSY clears during load, while the register tables say it sets. An idle word alone cannot prove startup. [SBOS740A pp. 17, 26, 29](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26). |
| TMP117, TMP117M, TMP117N | Table 7-6 defines bit 1 as R/W. Writing one starts a 2 ms reset and the bit always reads zero. Figure 7-14 labels it R-0. Use the detailed field definition. [SNOSD82D pp. 27–28](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27). |
| TMP119 | Table 8-5 gives the same command, duration and readback. Figure 8-7 has the same R-0 discrepancy. [SNIS236 pp. 34–35](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34). |

EEPROM reload, EUN after the command, the effect of companion CONFIG bits
and bus availability during addressed reset remain unknown. For the separate
POR/general-call fixture, load only MOD0 and bits 9:2 from persistent CONFIG
(`0x07FC`), along with the programmed limits, supported offset and mapped
shadows. Seed those words with nonfactory values. [Source pages by reset domain](../eeprom-and-reset-policy.md#protected-data-and-traceability).

The clean TMP1x2 sibling at `0791429f8fce1170e2082f1b011456fc461318b8`
provides the external-reset precedent. Its argument-free invalidation issues
no transfer and leaves recovery to the owner. TMP11x retains historical sample
and alert evidence and excludes the sibling's bus-wide reset helper.
[TMP1x2 implementation](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/src/TMP1x2.cpp#L187).

### Phase-b implementation and acceptance cases

1. **Command admission.** Start with a private reset write purpose in
   `src/RegisterTransport.{h,cpp}` and literal six-model tests in
   `test/test_register_transport.cpp`. Require a supported model, CONFIG
   pointer, startup proof and locked-idle guard. Accept only bit 1 plus
   canonical stable bits from the last verified settings. Check frames
   `01 02 22` (continuous) and `01 06 22` (shutdown). Reject missing bit 1,
   MOD=10/11 payloads, status/reserved bits, other pointers and ordinary
   write purposes used with the command. Exercise protected words for the
   added purpose. Count callbacks and target addresses.
2. **Public preflight and invalidation.** Require an initialized, verified
   device, valid clock/budget and retained startup proof. Read a new
   `0x04` guard before issuing the single addressed write. EUN refuses
   immediately, even with BUSY set. Only successful BUSY reads may be
   repeated at paced intervals within budget. Immediately before the command
   callback, invalidate initialization, startup origin/proof, live profile,
   guard, ready/shot and live alert evidence. Keep the desired profile,
   health history, last sample and pending historical alerts with their old
   generation and reset/unknown provenance. Test an applied write returning
   an error and a late completion. Neither permits a retry or reuse of old
   proof. Assert zero general-call traffic.
3. **Post-command observations.** From successful callback completion wait
   at least 2 ms, rounded for the clock. Read compatible ID, take a new
   FIRST_ACK at completion, then wait the separate 3 ms startup policy under
   stable power. Read a new locked-idle guard and the full profile (CONFIG,
   both limits and supported OFFSET). Use one operation deadline throughout.
   Count preflight and post-command reads separately. Preserve partial read
   evidence on failure. Exercise changed and unchanged settings, EUN, BUSY,
   ID mismatch, deadline and transport failure. A zero command bit never
   establishes completion. Matching observations leave initialization clear.
4. **Explicit recovery.** Compare addressed-reset outcomes using nonfactory
   THIGH `0x1280`, TLOW `0xFB00`, OFFSET `0x0040` and distinctive EEPROM
   shadows. A later explicit recovery restores owned settings, including a
   managed offset. When offset is unmanaged, observe the post-reset value
   and assert zero offset writes. Check the reset trace for zero replay and
   the begin/recover traces for zero reset commands. Reported external reset
   uses the existing argument-free invalidation followed by explicit recovery.
   [Public operation contract](../api-contracts.md#startup-apply-and-reset).

### Checks and handoff

| Baseline check | Result |
| --- | --- |
| `python scripts/check_references.py` and `python scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages. Python 3.12.10. |
| `python tools/check_register_ledger.py` and `python tools/test_check_register_ledger.py` | PASS, six model rows and 31 validator regressions. |
| `cmake -E chdir build/host ctest -C Release -R '^(register_transport\|binding\|apply\|trust\|acquisition\|eeprom)_cpp$' --output-on-failure` | PASS, six existing Release suites in the stage 24 build, CTest 4.0.1. These exercise the inputs to stage 25. |

Phase b starts with command admission (case 1). The public operation,
addressed-reset fixtures and their acceptance tests remain open. Stage 27
consumes the synchronous API and stage 38 owns CLI reset integration.
Physical reset behavior remains unverified. Remote CI belongs to stage 43.

## Phase b — Private reset command admission, 2026-11-27

Case 1 passed on a working tree based on `b253326808bb88686e0ebcb2830934fd2d4d6d5d`.
The phase-a commit matched the wrapper request and local upstream. The
wrapper recorded upstream equality at 01:45:09 Prague time.

The [private transport](../../src/RegisterTransport.cpp) admits an addressed
reset only at CONFIG on TMP117/M/N and TMP119. The command must contain bit 1,
with all remaining set bits inside the canonical stable mask (`0x07FC`).
The typed caller supplies the startup and locked-idle proofs. The public
operation still owns deriving the word from the last verified profile and
acquiring a new guard. The [fixture](../../test/support/FakeTmp11xDevice.cpp)
accepts the frame and models one possible outcome in which the register image
stays unchanged.

In [addressedResetAdmission](../../test/test_register_transport.cpp), each
accepted word produced one write to the selected address `0x4B`. The literal
frames included `01 02 22` and `01 06 22`. TMP116/N returned `UNSUPPORTED`
before I²C. All six models refused the other 255 pointers and incorrect write
purposes at CONFIG. On supported models each missing proof, missing command,
noncanonical MOD, status bit and reserved bit produced zero callbacks.
`protectedWritesDenyEveryPayload` also swept all 65,536 payloads at each
protected pointer with the new purpose.

| Local check | Result |
| --- | --- |
| `cmake --build build/host --config Release --target tmp11x_register_transport_test --parallel` | PASS with CMake 4.0.1 and GCC 15.1.0. |
| `cmake -E chdir build/host ctest -C Release -R '^register_transport_cpp$' --output-on-failure` | PASS, 1/1. |
| `python scripts/check_references.py` and `python scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages, Python 3.12.10. |
| `python tools/check_register_ledger.py` and `python tools/test_check_register_ledger.py` | PASS, six models and 31 regression cases. |
| `git diff --check` and `git diff --cached --check` | PASS. The wrapper owns staging, commit and push. |

Continue phase b with case 2, timed preflight and invalidation immediately
before the reset write, including ambiguous and late completion. The full
public operation, post-reset observations and explicit recovery tests remain open.

## Phase b — Timed reset preflight and invalidation, 2026-11-27

Case 2 passed on clean-entry `main` at `88f05ebdc082466f2187c20da1e070b1c35741bf`.
Its parent, subject and five paths matched the preceding wrapper request.
The wrapper recorded upstream equality at 01:59:50 Prague time.

The [private reset operation](../../src/Reset.cpp) encodes the verified target,
obtains a fresh locked-idle guard, and issues one addressed CONFIG write.
It shares timing admission and paced guard handling with initialization.
Immediately before the write it discards startup, initialization, current
profile and conversion evidence. Pending alerts retain their original
generation and gain the unknown-provenance and possible-loss flags. The
command result covers transport and deadline only. The public entry point
remains pending until post-command observations are implemented.

The [reset suite](../../test/test_reset.cpp) exercises four cases.

| Case | Observed result |
| --- | --- |
| `preflightRefusals` | Unbound, uninitialized, dirty-profile and TMP116/N requests issued zero callbacks. |
| `lockedIdlePreflight` | EUN alone and EUN+BUSY each latched unsafe after one guard. Short, continually busy and late guards issued zero writes and kept initialization. |
| `commandAfterBusy` | TMP117/M/N and TMP119 each retried BUSY after the paced interval, then issued exactly one canonical continuous or shutdown frame. Every transfer went to the selected address `0x4B`. |
| `invalidationAtCommand` | An injected write effect with a data NACK and a successful late write both left the driver uninitialized. Pending HIGH was already historical inside the write callback. Both cases retained desired settings, verification history and the last sample. A second command request made zero transfers. Explicit recovery established a new ID/startup interval. |

| Local check | Result |
| --- | --- |
| `cmake -S . -B build/host -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release` and `cmake --build build/host --config Release --parallel 6` | PASS, CMake 4.0.1 and GCC 15.1.0. Rebuilt the final test with `--target tmp11x_reset_test --parallel 6`. |
| `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 24 registered suites. |
| `python scripts/check_references.py` and `python scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages, Python 3.12.10. |
| `python tools/check_register_ledger.py` and `python tools/test_check_register_ledger.py` | PASS, six models and 31 regression cases. |
| `git diff --check` and `git diff --cached --check` | PASS. Staging, commit and push belong to the wrapper. |

Continue phase b with case 3. Carry the same operation budget from command
completion through the rounded 2 ms reset wait, compatible ID/FIRST_ACK,
separate startup wait, renewed guard and full profile observation. Case 4
retains the changed/unchanged reset fixture and explicit replay coverage.
Physical reset behavior remains unverified. Remote CI belongs to stage 43.

## Phase b — Public reset and explicit recovery

Cases 3 and 4 completed phase b on 2026-11-28. The clean input was
`ca1d91c35b4202284d331744289ed3f09ba8419e`. Its parent, subject and 14
paths matched the case 2 wrapper request. The wrapper logged upstream equality
at 02:16:14 Prague time on November 27. The pinned field tables cited above
still define the same model split and 2 ms command interval. The read-only
TMP1x2 reference remained clean at `0791429f`.

The public `reset()` uses one operation deadline. On an idle, matching
device its eight callbacks are one EEPROM preflight read, one addressed
command write, then reads of DEVICE_ID, EEPROM_UL, CONFIG, TLOW, THIGH and OFFSET in that order. Command completion starts the reset wait. Compatible ID
completion starts the separate startup wait. A CONFIG reporting BUSY retains
its flags and requires a paced guard renewal and another CONFIG observation.
A complete mismatch latches DIRTY. Matching observations leave trust UNKNOWN.
Neither outcome advances the verified generation or initializes the device.
[Public contract](../api-contracts.md#startup-apply-and-reset),
[reset engine](../../src/Reset.cpp).

If the reset interval is interrupted, the retained binding keeps the command
completion tick, including on an ambiguous transfer. A subsequent explicit
recovery waits the outstanding interval before taking a new ID/startup origin.
Bound-only diagnostics do not establish this proof. [Replay engine](../../src/Apply.cpp).

| Host cases | Observed result |
| --- | --- |
| `postCommandTiming`, `waitAtDeadline`, `invalidationAtCommand` | At 1 ms resolution ID began 3 ms after command completion and the guard began 4 ms after ID completion. At 2 ms resolution the waits were 4 and 6 ms. Equality at expiry stopped the next read. A stalled recovery returned WORK_LIMIT before any callback and preserved the pending reset interval. |
| `postCommandSafety`, `partialPostCommandReads`, `resetDispatchLimit` | Incompatible ID, EUN and persistent BUSY stopped the sequence. A short transfer at each of the six read positions retained the completed prefix. A late complete word was observed and counted before timeout. The 256th dispatch could finish at OFFSET. With the command in that slot, the operation returned WORK_LIMIT before ID. [Public reset tests](../../test/test_reset.cpp). |
| `addressedOutcomeAndRecovery`, `independentCommandAndObservationOutcomes`, `driftBeforeReadFailure` | All four supported selections exercised unchanged, changed-profile and offset-only images with managed and unmanaged calibration. Only explicit recovery wrote the nonfactory limits and any managed offset. Separate cases combined changed or unchanged limits with a command NACK, ID NACK or unlocked/busy guard. A short THIGH read after a mismatching TLOW returned the transport fault and retained the low-limit drift. |
| `reportedProgrammedReload` | Across all six selections the external POR/general-call fixture loaded only `0x07FC` of programmed CONFIG. External invalidation made zero callbacks. Recovery preserved the seeded shadows and unmanaged calibration. Addressed-reset images remain independent fixture choices. [Outcome tests](../../test/test_reset_outcomes.cpp), [register fixture](../../test/support/FakeTmp11xDevice.cpp). |

| Local check on the phase-b working tree | Result |
| --- | --- |
| `cmake -S . -B build/host -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build/host --config Release --parallel 6`, `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, 25/25 suites with CMake/CTest 4.0.1 and GCC 15.1.0. |
| `cmake -S examples/native_consumer -B build/p25b-consumer -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`, `cmake --build build/p25b-consumer --config Release --parallel 6`, `build/p25b-consumer/tmp11x_native_consumer.exe` | PASS, tests-off consumer linked and printed `0.1.0`. |
| `doxygen -` on `include/TMP11x` with XML and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. Parsed the generated public `reset()` entry in `build/p25b-docs/xml`. |
| `python scripts/check_references.py`, `python scripts/check_promptset.py`, `python tools/check_register_ledger.py`, `python tools/test_check_register_ledger.py` | PASS, 14 pinned PDFs, 55 stages, six model rows and 31 ledger regressions with Python 3.12.10. |
| `git diff --check` and `git diff --cached --check` | PASS. The wrapper owns staging, commit and push for this tree. |

Phase c owns fresh-session review of the command, evidence, fixture assertions
and source claims. Phase d owns acceptance and the completion marker.
The physical reset image, EUN outcome and bus availability remain unverified.

## Phase c — Fresh review, 2026-11-28

Reviewed the stage diff from `d80da0faf4ffeb39a65ca2092026b8b964ebe2f8`
through `521acd3aea85c6054440bcb0c281bfd1da7c3afc` on clean-entry
`main`. The phase-b commit matched the requested subject, parent and 19 paths.
The wrapper recorded upstream equality at 00:21:02 Prague time.

Two parallel read-only reviews covered the driver and lifecycle, and the TI
sources, fixtures and trace assertions. The TMP116 p.26, TMP117 pp.27–28 and
TMP119 pp.34–35 field tables support the existing command gate and reset
duration. The command uses one bound address. The post-command ID starts a
separate startup interval, and the readback records complete words through
the first fault. An earlier observed mismatch remains in the settings snapshot
when a later transfer fails. The programmed-startup fixture masks CONFIG with
`0x07FC`; addressed-reset tests choose their own register images. The sibling
external-invalidation reference remained clean at `0791429f`.

| Finding | Correction and evidence |
| --- | --- |
| Teardown discarded the command completion tick. After a command NACK, `end(); recover()` issued an ID read while the clock was frozen inside the reset interval. The new trace assertion failed against the phase-b driver. | The pending wait now belongs to the command address. `end()` preserves its clock origin. Rebinding to the same address starts a full wait in the new epoch. A failed identity read at a different address preserves the outstanding wait. `invalidationAtCommand` and `resetWaitAfterRebind` verify zero early callbacks, the separate startup delay and successful recovery. [Apply](../../src/Apply.cpp), [public contract](../api-contracts.md#startup-apply-and-reset). |
| Public traces previously used default CONFIG subfields for both modes. A fixed default word could pass those tests. | After public setters change the profile, `resetUsesVerifiedSettings` observes `0xABFC` and asserts the literal command `01 03 FE`. `resetAfterOwnedOneShot` observes MOD=11 after a real driver-triggered shot, asserts the shutdown command `01 06 22`, and gets a CONFIG mismatch when the fixture retains MOD=11 after reset. [Public regressions](../../test/test_reset.cpp). |

| Phase-c check | Result |
| --- | --- |
| `cmake -S . -B build/host -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build/host --config Release --parallel 6`, `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 25 suites with CMake/CTest 4.0.1 and GCC 15.1.0. |
| `cmake -S examples/native_consumer -B build/p25b-consumer -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`, `cmake --build build/p25b-consumer --config Release --parallel 4`, `build/p25b-consumer/tmp11x_native_consumer.exe` | PASS, tests-off consumer linked and printed `0.1.0`. |
| `doxygen -` over `include/TMP11x` with `GENERATE_XML=YES` and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. Parsed the bind, end and reset contracts from `build/p25c-docs/xml`. |
| `python scripts/check_references.py`, `python scripts/check_promptset.py`, `python tools/check_register_ledger.py`, `python tools/test_check_register_ledger.py` | PASS, 14 pinned PDFs, 55 stages, six model rows and 31 ledger regressions on Python 3.12.10. |
| `git diff --check`, `git diff --cached --check` | PASS. The wrapper owns staging, commit and upstream verification. |

Phase d owns final acceptance, the report index and the progress row.
Physical addressed-reset effects remain open. Remote CI belongs to stage 43.
