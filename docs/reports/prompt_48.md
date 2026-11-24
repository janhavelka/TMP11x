# Prompt 48 — Audit of guarded initialization and configuration trust

## Phase a — Brief and baseline, 2026-11-24

**PARTIAL.** Both prerequisites are complete and all three consumed artifacts are
available. The audit must establish when a volatile write is allowed, when its
effect has been verified, and what evidence survives a failure between those
points. Its output is `audit-18-19`, required by stage 20 before settings
wrappers can use the engine. [Audit mandate](../prompts/48_audit-18-19.md),
[sequence](../prompts/SEQUENCE.json), [artifact owners](../prompts/HANDOFFS.md).

The clean baseline was `d7cb61c7c8c99665546249c38cef15b942f150eb` on
`main`. The subject, four documentation paths and parent `f531557` match
the stage 19 closure request. The wrapper recorded upstream equality at
01:01:08 Prague time. Local HEAD and its upstream still matched at entry.
Review the 25-path cumulative diff from the stage 47 closure `fc1d1de`
through this baseline. Stage 18 closed at `21eba16` and stage 19 at
`d7cb61c`. Both original prompts and their acceptance reports were read.
[Stage 18 requirements](../prompts/18_safe-initialize.md),
[acceptance](prompt_18.md#phase-d-acceptance),
[stage 19 requirements](../prompts/19_config-trust.md),
[acceptance](prompt_19.md#phase-d-acceptance).

### Inputs at the baseline

| Input | Entry point and evidence for the audit |
| --- | --- |
| `safe-apply-engine` | `Apply::step` dispatches identity, EEPROM guard, shutdown, ordered limit writes and readbacks, baseline and final CONFIG. Trace each dispatch to its addressed callback and timing grant. [Phases](../../src/Apply.h), [engine](../../src/Apply.cpp), [register transport](../../src/RegisterTransport.cpp), [apply tests](../../test/test_apply.cpp). |
| `sync-initialize-recover` | `begin(config)` validates timing and binds the profile. Both begin overloads and `recover()` reach the same step loop. Check rejected replacement, replay after partial mutation, tracked health and admission to latest reads on all six BOM selections. [Public API](../../include/TMP11x/TMP11x.h), [implementation](../../src/TMP11x.cpp), [latest reader](../../src/LatestRead.cpp). |
| `config-trust` | `readConfiguration` reads CONFIG once. `verifyConfiguration` scans CONFIG, TLOW and THIGH. Snapshots and invalidation are bus-silent. The shared evidence code records per-field observations, dirty causes and destructive effects. [Public implementation](../../src/TMP11x.cpp), [read and write evidence](../../src/DeviceEvidence.cpp), [stable comparison](../../src/Configuration.cpp), [trust tests](../../test/test_trust.cpp). |

### Sources and failure cases

| Pinned TI source and BOM selections | ALERT, THERM and startup | Register map, CONFIG, limits and EEPROM_UL |
| --- | --- | --- |
| [TMP116 SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17) · TMP116/N | pp. 15–17 | pp. 24–29, especially tables 6 and 10 |
| [TMP117 SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18) · TMP117/M/N | pp. 16–18 | pp. 25–30, especially tables 7-6 and 7-10 |
| [TMP119 SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20) · TMP119 | pp. 18–20 | pp. 32–37, especially tables 8-5 and 8-9 |

TMP116 p. 17 describes BUSY as clear during reset loading. Its CONFIG and
EEPROM_UL tables describe BUSY as set at power-up. Keep both cases. On
TMP117/TMP119, CONFIG and limit writes can be ignored during loading. The
3 ms FIRST_ACK wait is project policy. TI gives an approximate startup time.
[EEPROM policy](../eeprom-and-reset-policy.md), [timing contract](../owner-operations.md).

| Boundary to trace in phase b | Evidence to capture |
| --- | --- |
| Admission, startup and lock | Compare rejected binding with delayed power after binding. With proof absent, time the wait from compatible ID completion. Exercise coarse, wrapping and frozen clocks. EUN in either `0x8000` or `0xC000` must stop before a write. BUSY alone permits paced reads of `0x04`. A CONFIG read that sees BUSY revokes the guard. Record work limits, last pointer and write count. |
| Volatile writes and readback | Check stable writes against `0x07FC`, full signed limit words, and zero writes to EEPROM_UL, shadows or offset. Verify shutdown in ALERT mode before an inverting limit write and both limits before enabling THERM. Inject an ACK with a suppressed write and an ambiguous first or middle write. Record the actual pair, desired target, first dirty cause and last verified generation. |
| Verification and recovery | Change CONFIG or either limit independently. Fail the third read after an earlier mismatch. Preserve the completed fields, timestamps, target and physical health. Trace matching observations while dirty, then explicit replay through a timely final CONFIG. Test MOD=10 as continuous and MOD=11 against desired shutdown with and without physical trigger provenance. |
| Read effects and invalidation | Compare complete, short and deadline-equality CONFIG/TEMP reads. Track DATA_READY separately from retained ALERT events, mixed-generation attribution and the THERM level. Check output and last sample after a late success. After owner-reported reset, check retained historical evidence and a new FIRST_ACK and guard before replay. |

Use the [literal behavior vectors](../registers/behavior-vectors.json),
[register ledger](../registers/tmp11x.json) and [scripted fixture](../../test/support/FakeTmp11xDevice.cpp)
to derive expected frames and compare them with the registered apply, trust,
configuration and latest cases. The clean TMP1x2 and ADS1115 sibling trees
were at `0791429` and `1aab07b`. Their selected implementation and test
files matched the project pins `afd5088` and `e07622a`. TMP1x2's
`src/TMP1x2Configuration.cpp` supplies the step and partial-verification
precedent. ADS1115's `src/ADS1115.cpp` supplies masked readback and ownership
examples. Derive TMP11x masks, ordering and startup timing from its own
sources. [Pinned sibling comparison](../library-comparison.md).

### Entry checks and remaining work

- `cmake --build build/host --config Release --parallel 4` and
  `cmake -E chdir build/host ctest -C Release --output-on-failure` passed
  all 16 baseline tests, including register-ledger validation and regressions
  (CMake 4.0.1, GCC 15.1.0).
- `python -B scripts/check_references.py` verified all 14 PDF sizes and
  hashes. `python -B scripts/check_promptset.py` passed for 55 stages
  (Python 3.12.10).

Phase b records the source-backed success and failure traces above. Phase c
rereads both original prompts and runs two independent read-only reviewers
in parallel. One traces startup, lock, masks and write ordering through the
transport. The other traces trust, destructive reads and publication through
the public API. Each checks the relevant tests, edge cases and simplicity.
The lead verifies findings against the diff and sources, fixes confirmed
gaps and repeats affected checks.

Phase d repeats both producers' acceptance checks, including a clean Release
build, tests-off consumer, isolated public headers, README examples and strict
Doxygen. Trigger dispatch (21), alert consumption (22), managed offset (23)
and job ownership (28–29) retain their integration owners. MCU links (34–36),
fixture qualification (40–41), package qualification (42) and the first remote
CI checkpoint (43) retain their separate evidence gates.

## Phase b — Admission, startup and EEPROM guard

The clean entry revision was `72c17acdf8b6eb73c04a5997b9eda46047985ea8`.
Its parent, subject and sole report path matched the phase-a request. The
wrapper verified equality with `origin/main` at 01:14:47 Prague time.
The audited implementation remains the one in `fc1d1de..d7cb61c`.

TMP116 p. 17 permits reads and describes ignored writes with BUSY clear
during loading. Its field tables describe BUSY asserted at power-up.
TMP117 p. 18 and TMP119 p. 20 describe asserted BUSY and ignored CONFIG
and limit writes. On all six selections, `Apply::readIdentityStep` anchors
the policy wait at compatible ID completion when startup proof is absent.
After that wait, `readGuard` interprets bits 15 and 14 of EEPROM_UL.
Every replay obtains its own guard. A subsequent CONFIG with bit 12 set
revokes it. [TMP116 pp. 17, 26, 29](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 pp. 18, 27, 30](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18),
[TMP119 pp. 20, 34, 37](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20),
[guard policy](../eeprom-and-reset-policy.md#startup-and-operation-guard), [engine](../../src/Apply.cpp).

The scripted bus recorded the following frames at address `0x49`. Each
`R` sends the indicated pointer and reads two bytes. Each `W` sends the
pointer and one MSB-first word. [Fixture and assertions](../../test/test_apply.cpp).

| Case | Expected and observed result |
| --- | --- |
| Rejected replacement and successful replay | In `admissionPreservesBinding`, invalid selectors, an inverted THERM pair or invalid timing produced zero callbacks and preserved the initialized binding. A valid replacement with incompatible ID ended at `R0F` with `IDENTITY_MISMATCH` and one successful transfer. In `allModelsBeginRecoverLatest`, begin and recovery each produced `R0F R04 R01 R03 R02 W01=0620 R01 W03=0000 R03 W02=0500 R02 R01 W01=0220 R01` on all six models. Recovery advanced the generation. |
| FIRST_ACK and suppressed writes | In `startupFromCompatibleCompletion`, the fixture bound at tick 0, probed at 30 and started apply at 40. ID completed at tick 50. Only `R0F` appeared before tick 54 (1 ms clock) or 70 (10 ms clock). At that tick the next frame was `R04` and replay succeeded. In the TMP116 suppressed-write case of `workLimitsWrapAndLoadSuppression`, clear BUSY passed the guard after the wait. The first shutdown write was ACKed and ignored. Readback ended the operation at `W01=0620 R01` with `CONFIG_MISMATCH`, CONFIG still zero and initialization incomplete. |
| EUN and transport failure | In `busyAndUnlockedGuards`, either `0x8000` or `0xC000` ended begin at `R0F R04` with `EEPROM_UNSAFE` and zero writes. A guard bus failure ended recovery on the same two reads with `I2C_BUS` and adapter detail 17. The new `unlockedGuardAtDeadline` case returned `0xC000` exactly at expiry. Begin returned `OPERATION_TIMEOUT`, remained uninitialized and recorded two successful reads, the last at the deadline. After the fixture cleared EUN, recovery still returned `EEPROM_UNSAFE` with zero callbacks. |
| BUSY, elapsed time and work caps | With only BUSY set, the 45 ms case expired after `R0F` and paced `R04` reads with zero transport failures. With a large deadline, dispatch 256 ended on `R04` with `WORK_LIMIT`. A frozen startup wait allowed 1,000,000 unchanged-tick checks and stopped on the next with only `R0F`. Begin across unsigned wrap completed the normal 14 frames. [Work-limit cases](../../test/test_apply.cpp). |
| BUSY found on CONFIG readback | In `busyShutdownReadbackRequiresNewGuard`, shutdown readback returned HIGH, READY and BUSY. The next and last read was `R04`, returning `0xC000`. Begin returned `EEPROM_UNSAFE` after one write, with HIGH retained, no ready baseline and trust DIRTY. In `busyRecheckAndFinalBusy`, BUSY on final CONFIG instead led to a paced `R04 R01` after the final write. A safe guard and matching reread completed initialization with HIGH retained. |

The Release build and four focused suites passed with CMake/CTest 4.0.1
and GCC 15.1.0. Commands were `cmake --build build/host --config Release --parallel 4`
and `cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(identity|apply|register_transport|trust)_cpp$'`.
`python -B scripts/check_references.py` verified 14 pinned PDFs with Python
3.12.10. This slice added the deadline regression to the existing apply
suite. The production guard handled the case as required.

## Phase b — Ordered limit writes and partial effects

The clean entry revision was `a18edfd9df5f75296679e3d339045493915575e8`.
Its parent, subject and two changed paths matched the previous commit
request. The wrapper verified equality with `origin/main` at 01:29:41
Prague time. The following results come from the scripted host fixture.

TI defines TLOW and THIGH as signed 16-bit words. In THERM mode, HIGH
asserts above THIGH and clears below TLOW. The driver reads both limits,
compares signed counts, and verifies each write before relying on it.
For canonical continuous and shutdown writes it emits only CONFIG bits
10:2 (`0x07FC`). The six-model transport matrix rejects each excluded
CONFIG bit and refuses EEPROM_UL and data-shadow writes before the callback.
The apply fixture permits writes only to CONFIG, THIGH and TLOW. The
private transport separately gates the offset purpose by model.
[TMP116 pp. 15–16, 26–28](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26),
[TMP117 pp. 16–17, 27–29](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27),
[TMP119 pp. 18–19, 34–36](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34),
[transport checks](../../test/test_register_transport.cpp), [apply engine](../../src/Apply.cpp).

Pairs below are hexadecimal register words in (TLOW, THIGH) order. Each
apply trace starts with `R0F R04 R01 R03 R02`. The frame sequences in the
table show the decisions after that preflight. [Apply cases](../../test/test_apply.cpp),
[public trust cases](../../test/test_trust.cpp), [non-atomic THERM limit vector](../registers/behavior-vectors.json).

| Boundary | Frames and retained evidence |
| --- | --- |
| Moving an ordered THERM pair | After verified shutdown in THERM (`W01=0630 R01`), raising (0000,0500) to (0780,0A00) produced `W02=0A00 R02 W03=0780 R03`. Lowering it back produced `W03=0000 R03 W02=0500 R02`. Both passed through (0000,0A00). The new signed-boundary case started at (FF80,FFFF), or (−128,−1) counts. It required `W02=0001 R02 W03=0000 R03` before resuming continuous THERM. Writing TLOW first would transiently invert the pair. |
| Leaving and entering THERM | Leaving THERM for (0A00,0500) required timely ALERT shutdown readback (`W01=0620 R01`) before writing either limit. Short, mismatched and deadline-equality readbacks stopped at that `R01` with TLOW still 0000. In the reverse transition, both limits were written and read back in ALERT shutdown before `W01=0630 R01` enabled and verified THERM. |
| ACK with no change | Suppressing the first THERM limit write in either direction stopped at that limit's readback with `CONFIG_MISMATCH` and the old pair intact. On entry from inverted ALERT (0A00,0500) to THERM (0000,0400), ignoring TLOW left (0A00,0500). Ignoring THIGH after verified TLOW left (0000,0500). Both ended in ALERT shutdown with DIRTY trust. |
| Ambiguous first or middle write | In `ambiguousWritesKeepTheTarget`, a data NACK on the first shutdown write or on `W03=FF00` left the attempted word in the fake. The public snapshot invalidated that field, retained the desired continuous profile (FF00,0500), recorded `AMBIGUOUS_WRITE`, and preserved the previous generation and verification time. This held both before first initialization and after a verified begin. |
| Ambiguous TLOW after verified THIGH | Staging (0780,0A00) and expiring after preflight preserved the previous verified target (0000,0500). On the subsequent attempt, `W02=0A00 R02 W03=0780` ended in a data NACK. The fixture retained (0700,0A00). The public snapshot retained THIGH=0A00, marked TLOW unknown, kept the staged target and first cause `AMBIGUOUS_WRITE`, and preserved the previous verification time and generation. Explicit recovery read the guard again, verified TLOW then THIGH, and advanced the generation exactly once after final CONFIG. |

The signed ordering and partial-write snapshot assertions passed against
the existing engine. Verification used CMake/CTest 4.0.1, GCC 15.1.0 and
Python 3.12.10. `cmake --build build/host --config Release --parallel 4`
passed, followed by five passing suites with
`cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(register_transport|temperature|configuration|apply|trust)_cpp$'`.
`python -B scripts/check_references.py` verified 14 pinned PDFs and
`python -B scripts/check_promptset.py` passed for 55 stages.

## Phase b — Verification after drift

The clean baseline was `6aec9adbbb8417d8301e23776c9a423589d4e652`.
Its parent, subject and two paths matched the previous request. The
wrapper verified equality with `origin/main` at 01:44:33 Prague time.

Both public reads feed each physical outcome to `DeviceEvidence::observeRegister`.
On all six selections, `readConfiguration` issued one `R01` and returned
the complete word. Reading `FA20` preserved verified trust with the
continuous alias and BUSY set. Reading `1A28` recorded polarity drift
against desired active-low. Neither observation advanced the verification
time. Explicit verification reads `R01 R03 R02`. A clean scan updates the
time to THIGH completion and retains the generation. [Public operations](../../src/TMP11x.cpp),
[field observations](../../src/DeviceEvidence.cpp), [trust tests](../../test/test_trust.cpp).

| Scripted verification at address `0x48` | Return and state after the callback sequence |
| --- | --- |
| Only TLOW or THIGH drifted by one count | `CONFIG_MISMATCH` after all three reads. Only the changed limit was marked affected. The snapshot showed −255 or 1281 counts and kept the desired pair (−256, 1280). Explicit recovery restored that pair. |
| CONFIG returned `0224`, TLOW returned `FF01`, THIGH timed out | `I2C_TIMEOUT`. CONFIG and TLOW gained new completion times in read order. THIGH retained its earlier `0500` and time. The scan mask and affected mask were both `0x03`. The first dirty cause remained `READBACK_MISMATCH`. Health counted two successes and one failure, with last success at TLOW and last error at THIGH. The desired pin function, both desired limits, verification time and generation survived. |
| Owner restored all three words, then verification ran again | `CONFIG_DIRTY`. The full scan recorded `0220/FF00/0500` and mask `0x07`. The affected mask stayed `0x03` and the prior verified time and generation survived. A subsequent CONFIG NACK left mask zero and all three observation times intact. Explicit recovery then made 14 transfers, starting with identity and EEPROM_UL and ending with CONFIG. The final read established the next verified generation and cleared both masks. |

In `replayAndStagedChangeOnDrift`, staged settings stopped at the
mismatched CONFIG or TLOW preflight read, preserving the old target.
Explicit replay took the full guarded sequence. The extended
`partialVerificationRetainsEvidence` assertions connect the timeout,
outside correction and replay on the same device. [Apply traces](../../test/test_apply.cpp),
[verification contract](../api-contracts.md).

The TI MOD tables specify `10` as a continuous alias that reads back
as `00`, and `11` as the one-shot command. The driver also accepts an
observed `10` for desired continuous. In `physicalTriggerEvidence`, an
injected terminal trigger success allowed `0E20` and `0620` for desired
shutdown. A failed trigger left `0E20` as drift. A physically successful
shutdown write at the deadline ended the exception, so a subsequent
`0E20` again produced `CONFIG_MISMATCH`. This tests provenance at the
write-evidence seam. Stage 21 owns trigger dispatch. [Comparator](../../src/Configuration.cpp),
[TMP116 p. 26](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26),
[TMP117 p. 27](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27),
[TMP119 p. 34](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34).

`cmake --build build/host --config Release --parallel 4` passed with
CMake 4.0.1 and GCC 15.1.0. All four suites passed with
`cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(trust|apply|configuration|health)_cpp$'`.
`python -B scripts/check_references.py` verified 14 PDFs and
`python -B scripts/check_promptset.py` passed for 55 stages with Python
3.12.10. Phase b continues with destructive CONFIG/TEMP reads and
invalidation after owner-reported reset.

## Phase b — Destructive CONFIG and TEMP reads

The clean baseline was `46586af12d1fbf33c3fd69cc346fc29b5466b11e`. Its
parent, subject and two paths matched the prior request. The wrapper
verified upstream equality at 01:59:35 Prague time.

TI specifies that reading either register clears DATA_READY. In ALERT mode
a CONFIG read also acknowledges HIGH and LOW. In THERM mode HIGH follows
the hysteresis thresholds and LOW reads zero. The byte at which a partial
read clears DATA_READY remains unknown. The host fixture exercises both
outcomes. [TMP116 pp. 15–16, 26](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26),
[TMP117 pp. 16–17, 27](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27),
[TMP119 pp. 18–19, 34](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34).

| Host trace | Observed evidence |
| --- | --- |
| CONFIG, TEMP, CONFIG on initialized TMP117 | The first CONFIG returned `A220` (HIGH and READY). The fixture then raised LOW and READY. A complete TEMP left the next CONFIG at `4220`. A short TEMP could leave either `4220` or `6220`, both before and exactly at expiry. In all four short cases the driver discarded pending readiness. The following CONFIG established a baseline at tick 1006 and retained LOW alongside HIGH with its original time and generation. Admission at expiry issued no TEMP callback and preserved pending readiness. |
| TEMP publication at expiry | In `wrappedDeadline`, a complete TEMP ending at expiry returned a physical word and counted as a transport success. A short read at the same tick counted as a transport failure. Both returned `OPERATION_TIMEOUT` and preserved the caller's sample and every field of the previous cached sample. |
| CONFIG across a profile change | In `retainedAlertsAcrossGenerations`, HIGH survived a zero CONFIG and recovery. LOW in the next verified generation set the mixed marker and preserved the first event's time and generation. In `finalThermReadAfterVerifiedAlert`, a complete final CONFIG at expiry captured the THERM HIGH level alongside a retained ALERT LOW. A short final CONFIG left the level unknown and marked possible alert loss. Both left trust dirty and preserved the previous sample. |

The only change to the tests extends `tempConsumesRetainedReadiness` through
the next public CONFIG read. The existing implementation passed all seven
rows. [Latest-read tests](../../test/test_latest.cpp), [apply tests](../../test/test_apply.cpp),
[evidence updates](../../src/DeviceEvidence.cpp), [sample publication](../../src/LatestRead.cpp).

`cmake --build build/host --config Release --parallel 4` passed with GCC
15.1.0 and CMake 4.0.1. All five suites passed with
`cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(latest|apply|trust|register_transport|health)_cpp$'`.
Python 3.12.10 verified 14 PDFs with `python -B scripts/check_references.py`
and 55 stages with `python -B scripts/check_promptset.py`. The remaining
phase-b trace covers owner-reported reset and renewal of FIRST_ACK.

## Phase b — Recovery after an owner-reported reset

The clean entry revision was `018fa7d0732040fb7832cd0eb5b1dddb3743889f`.
Its parent, subject and two paths matched the previous request. The wrapper
verified equality with `origin/main` at 02:15:01 Prague time.

`invalidateDeviceState()` calls `clearLiveState()` and clears the latched
EEPROM refusal. It drops startup proof, initialized state, observed fields,
readiness, owned-trigger provenance and the live THERM level. It keeps the
desired profile, last verified generation and time, sample, pending ALERT
events and transport history. The application restores stable power and
exclusive access before replay. [Invalidation](../../src/TMP11x.cpp),
[state fields](../../include/TMP11x/TMP11x.h), [interference policy](../eeprom-and-reset-policy.md#external-interference).

| Host case | Evidence |
| --- | --- |
| `recoveryAfterReportedReset` on TMP117 | With DATA_READY pending after a successful begin, invalidation cleared readiness and its baseline. A latest read returned `NOT_INITIALIZED`. Neither call issued a frame. After a 100-tick idle interval the new ID callback took 10 ticks. At ID completion +3 the trace was still `R0F`. At +4 the first `R04` returned BUSY (`4000`). At +5 no further frame had run. At +6 a second `R04` returned locked and idle. Only then did replay read CONFIG and both limits, write shutdown, and finish the verified profile. The final CONFIG advanced the generation once. [Apply test](../../test/test_apply.cpp), [startup and guard steps](../../src/Apply.cpp). |
| `retainedAlertsAcrossGenerations` | Invalidation preserved sample `F380` with its original verification trust, generation and timestamp. The retained HIGH and LOW events kept their mixed-generation marker and original observation time and generation. [History assertions](../../test/test_apply.cpp). |
| `invalidationAfterExternalCorrection` | A timed-out TEMP read retained transport detail 17. After EUN was observed, correcting the fixture alone still produced `EEPROM_UNSAFE` without traffic. Invalidation preserved the error and transfer history, binding, desired limits and last verification time. Live trust became UNKNOWN and all three field observations were empty. Public recovery obtained a new ID and guard and advanced the generation once. [Trust test](../../test/test_trust.cpp). |

The reset trace completes the phase-a evidence matrix. The existing driver
passed. The new assertions cover the interval between ID completion and the
first safe post-reset guard. The 3 ms minimum and clock rounding come from
project policy. TI describes an approximate startup interval.
[Startup policy](../eeprom-and-reset-policy.md#startup-and-operation-guard).

`cmake --build build/host --config Release --parallel 4` and
`cmake -E chdir build/host ctest -C Release --output-on-failure` passed
all 16 tests with CMake/CTest 4.0.1 and GCC 15.1.0.
`python -B scripts/check_references.py` verified 14 PDFs.
`python -B scripts/check_promptset.py` passed for 55 stages with Python
3.12.10. `git diff --check` passed. Phase c owns the two independent
reviews of prompts 18 and 19. Stage 20 still depends on audit closure.

## Phase c — Independent review, 2026-11-24

Two read-only reviewers worked in parallel after rereading prompts 18 and 19.
They examined the original changes in `fc1d1de..d7cb61c` and the subsequent
audit tests through `0fe8ab0656eb8d8a17534d5ef3780fa047015158`. The entry
tree was clean. The prior request matched that commit's parent, subject and
two paths. The wrapper verified upstream equality at 02:29:53 Prague time.

| Review | Finding and disposition |
| --- | --- |
| Guarded apply | The FIRST_ACK wait starts at compatible ID completion. After reported reset, a BUSY guard permits only a paced guard reread. A CONFIG readback with BUSY cancels the earlier guard. Signed limit ordering and verification of each write protect the intermediate THERM pair when an ACKed write is ignored. The lead checked these paths against the pinned TI pages above, the transport mask and the scripted frames. No production correction was needed. [Engine](../../src/Apply.cpp), [transport](../../src/RegisterTransport.cpp), [tests](../../test/test_apply.cpp). |
| Trust and publication | The partial scan, short and late destructive reads, explicit invalidation and latest-sample publication matched the stage 19 contract. One regression test changed both CONFIG read source and profile generation between HIGH and LOW. Either change could set the mixed marker. The revised test captures both events through `readConfiguration` in continuous mode and checks that HIGH alone retains its original attribution across recovery. LOW then sets the mixed marker while preserving the first event's time and generation. [Evidence](../../src/DeviceEvidence.cpp), [regression](../../test/test_apply.cpp), [same-generation control](../../test/test_latest.cpp). |

The lead removed only the generation comparison from a copy of
`DeviceEvidence.cpp` under `build/p48c-alerts` and linked it into the apply
test with GCC 15.1.0. The original test passed with that defect. The revised
test failed at the mixed-marker assertion, as expected.

`cmake --build build/host --config Release --parallel 4` and
`cmake -E chdir build/host ctest -C Release --output-on-failure` passed
all 16 tests against production code (CMake/CTest 4.0.1).
`python -B scripts/check_references.py` verified 14 pinned PDFs and
`python -B scripts/check_promptset.py` passed for 55 stages with Python
3.12.10. `git diff --check` passed. Both reviewers finished. Phase d owns
the remaining clean-build, consumer, header, README and Doxygen acceptance
checks and the audit closure.
