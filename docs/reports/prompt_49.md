# Prompt 49 — Audit of conversion settings and fresh acquisition

## Phase a — Brief and baseline, 2026-11-25

**PARTIAL.** Both inputs are available. The audit follows a managed settings
change through the first result that the driver may publish as fresh. It also
checks how an owned one-shot establishes that boundary when TEMP or DATA_READY
still describes earlier work. Stages 22 and 23 consume the resulting
`audit-20-21` report. [Audit requirements](../prompts/49_audit-20-21.md),
[sequence](../prompts/SEQUENCE.json), [handoffs](../prompts/HANDOFFS.md).

Clean `main` and its local upstream ref both pointed to
`931ee693f7ace4722d5c42109e4ef6c8b0f3093d`. The stage 21 closure request
matched the subject, parent `b241ea7` and four changed paths. The wrapper
recorded upstream equality at 02:29:49 Prague time. The 34-path audit diff
starts at the stage 48 closure `aff8ef39570553cbce1a9eac72c265faf27fb4c1`.
Stage 20 closed at `1b1c8ff` and stage 21 at the audit baseline.

| Input | Entry points and evidence checked |
| --- | --- |
| `conversion-settings` | The three setters copy the desired profile and call `applySettings`. The timing query computes nominal period and standby separately from count-scaled active time. The fixture checks the shutdown, field writes, CONFIG baseline, desired mode and final readback for limit-only and routing-only changes as well as cycle and averaging. [Requirements](../prompts/20_conversion-settings.md), [producer acceptance](prompt_20.md#phase-d-acceptance), [setters](../../src/TMP11x.cpp), [apply](../../src/Apply.cpp), [timing](../../src/Timing.cpp), [wire assertions](../../test/test_conversion_settings.cpp). |
| `sync-acquisition` | The three fresh-read calls share a step engine. The one-shot wire test checks the EEPROM guard, CONFIG comparison, explicit shutdown, baseline read, one trigger, ready poll and combined TEMP read. Review each destructive read through evidence bookkeeping, deadline handling and sample publication. [Requirements](../prompts/21_fresh-one-shot.md), [producer acceptance](prompt_21.md#phase-d-acceptance), [acquisition](../../src/Acquisition.cpp), [evidence](../../src/DeviceEvidence.cpp), [budget](../../src/OperationBudget.h), [publication](../../src/LatestRead.cpp), [wire assertions](../../test/test_acquisition.cpp). |

### Audit evidence to collect

1. Compare the 32 CONV × AVG table entries for each family with the literal test expectations. Check all six BOM selections, zero standby when averaging fills the cycle, and one-shot duration across CONV codes. Keep TMP117's one-shot electrical test condition attached to continuous estimates. Trace a settings change during an incomplete block through final CONFIG and the first eligible ready event, including an ambiguous shutdown write. [Timing tests](../../test/test_timing.cpp), [block tests](../../test/test_averaging_blocks.cpp), [behavior vectors](../registers/behavior-vectors.json).
2. Use the same retained TEMP value across two generations. Trace stale READY at the shutdown baseline, both allowed owned MOD readbacks, an unowned MOD=11, BUSY renewal, EUN refusal and a diagnostic that consumes READY between steps. Record writes, generation, trigger provenance, sticky alerts and eligibility to publish. A lost event must end without a second trigger. [Device behavior](../device-behavior.md), [guard policy](../eeprom-and-reset-policy.md), [fixture](../../test/support/FakeTmp11xDevice.cpp).
3. At a wrapping clock tick and at exact expiry, compare the callback allowance, terminal physical result, caller output and cached sample. Cover a late trigger, late TEMP, repeated clock ticks and finite work exhaustion. For continuous conversion, a combined TEMP read still leaves silicon byte latching open. [Owner deadlines](../owner-operations.md), [engineering contract](../prompts/ENGINEERING.md).

| Exact BOM selections | Pinned electrical, mode, CONFIG timing and EEPROM guard pages |
| --- | --- |
| TMP116, TMP116N | pp. 5, 12–14, 25–27, 29. Page 14 specifies one-shot entry from shutdown. [SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=14). |
| TMP117, TMP117M, TMP117N | pp. 6, 14–15, 26–28, 30. Table 7-7 gives the nominal continuous periods. [SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28). |
| TMP119 | pp. 7, 16–17, 23, 33–35, 37. Include the precision guidance on bus activity when assessing the default poll schedule. [SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23). |

The clean OPT4001 and TMP1x2 sibling checkouts were at `f17d494` and
`0791429`. OPT4001's `src/OPT4001.cpp` matched pin `441ef57`. Its latest
and try-fresh calls illustrate the API distinction. TMP1x2's
`src/TMP1x2Configuration.cpp` and `src/TMP1x2Measurement.cpp` matched
`afd5088`. Its rate setter illustrates copying the desired configuration.
Its OS-bit completion rule is specific to TMP1x2.

### Baseline checks and stopping point

- `cmake --build build/host --config Release --parallel 4` and `cmake -E chdir build/host ctest -C Release --output-on-failure -R "^(conversion_settings|timing|averaging_blocks|acquisition)_cpp$"` passed all four selected suites with CMake/CTest 4.0.1.
- `python -B scripts/check_references.py` verified the paths, sizes and hashes of all 14 PDFs. `python -B scripts/check_promptset.py` passed for 55 stages with Python 3.12.10.

Phase b starts with the timing tables and the interrupted settings-change
trace in item 1. Phase c requires two parallel read-only reviews, one covering
settings, timing and averaging, the other covering acquisition, read effects
and deadlines. Full producer verification remains due at acceptance. Physical
MOD readback, the read-clear race and continuous TEMP byte latching remain
open for stages 40–41. Stage 43 owns the first remote CI checkpoint.

## Phase b — Timing and the first fresh result after recovery

The slice started on clean `main` at `b9029de60b44c92e16fbf55c7037fcf50ac51cda`.
The phase-a commit matched its requested subject, parent and single report path.
The wrapper recorded equality with `origin/main` at 02:44:30 Prague time.

All 96 published continuous-period cells matched the stage 6 vectors and the
literal C++ test matrix. Each family has 32 CONV × AVG selections. Subtracting
the table's rounded active budget from each cell reproduced all 32 standby
expectations, including every zero marked by TMP116 footnote 1. At CONV 100,
AVG 01 gives 875 ms standby and AVG 11 gives zero. [TMP116 Table 7](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=27),
[TMP117 Table 7-7](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28),
[TMP119 Table 8-6](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=35).

Multiplying each electrical row by 1, 8, 32 and 64 reproduced all four
min/typ/max triples per family. For 64 conversions TMP116/N gives
864/992/1088 ms, and TMP117/M/N and TMP119 give 832/992/1120 ms, assuming
no extra block overhead. TMP117 specifies one-shot mode, so continuous use
of its row is an extrapolation. TMP119 specifies no internal averaging.
Across all six BOM selections the one-shot budget stayed at the derived
active maximum plus 10 ms for every CONV code. The margin is library policy.
[Electrical conditions](../device-behavior.md), [vectors](../registers/behavior-vectors.json),
[timing tests](../../test/test_timing.cpp).

The interrupted-block test previously ended at the fake device's READY bit.
It now follows the same raw TEMP value through the public API on both sides
of a failed cycle change. The TMP117 fixture published 8 counts, then took
one input of 80 counts toward the next eight-input block. A scripted timeout
on the shutdown write left the device in continuous mode with CONFIG
`0x0120`. The driver retained the requested cycle, marked the profile dirty
and refused `tryReadFresh` without a bus callback. The fixture leaves the
partially accumulated result unknown. [Regression test](../../test/test_conversion_settings.cpp).

Explicit recovery performed the 14 expected transfers, including the EEPROM
guard, shutdown, verified limit writes, CONFIG baseline, continuous-mode
write and final CONFIG read. That last read captured HIGH and READY with
stable fields `0x0320`. It retained HIGH and advanced the verified generation
once. The public fresh read returned `MEASUREMENT_NOT_READY` immediately
after recovery and again after seven inputs. The eighth input published the
same 8 counts under the new generation. A second read found no fresh result.
[Apply sequence](../../src/Apply.cpp), [read effects](../../src/DeviceEvidence.cpp),
[acquisition](../../src/Acquisition.cpp).

Python 3.12.10 with `pypdf` compared the three pinned tables and electrical
rows directly with the vectors and C++ literals. `python -B scripts/check_references.py`
verified all 14 PDF hashes. `cmake --build build/host --config Release --parallel 4`
and `cmake -E chdir build/host ctest -C Release --output-on-failure -R "^(conversion_settings|timing|averaging_blocks|acquisition)_cpp$"`
passed all four suites with CMake/CTest 4.0.1.

Item 1 is checked. Phase b resumes at item 2 with owned one-shot provenance,
stale or lost READY and guard renewal. Item 3 covers deadlines and clock wrap.

## Phase b — One-shot ownership and guard renewal

Item 2 started on clean `main` at `86753aff4f7b056cf40ef9a286f330b262800255`.
The preceding commit matched its request in subject, parent and both paths.
The wrapper verified upstream equality at 02:59:43 Prague time.

| TMP117 host fixture | Observed transfers and result |
| --- | --- |
| Stale READY and HIGH at the shutdown baseline | After a safe read of `0x04` and a matching CONFIG preflight, the driver wrote `0x0620` to CONFIG. The baseline read returned `0xA620`. HIGH stayed pending. The driver wrote `0x0E20` once. Without another READY it polled CONFIG until timeout. A completed eight-input block allowed one fresh TEMP read even when TEMP still equaled the reset word `0x8000`. |
| Owned and unowned MOD | The wire tests accepted MOD=01 and 11 after a successful trigger. Fresh publication required READY and carried the verified settings generation. At preflight, unowned `0x0E20` returned `CONFIG_MISMATCH` and marked the profile dirty after just two reads. After an abandoned shot or a physically successful late trigger, the next explicit shot accepted the residual `0x0E20`, wrote shutdown, read its baseline and issued one new trigger. |
| BUSY and EUN | A BUSY preflight delayed the shutdown write until another safe `0x04` read and CONFIG comparison. EUN on the initial guard refused all writes. The new regression makes BUSY appear later, at the shutdown baseline (`0xB620`). The renewed guard returned `0xC000` (EUN and BUSY). Shutdown was the only write. The driver returned `EEPROM_UNSAFE`, retained HIGH with its original generation and published no sample. A subsequent shot was refused without I/O. |
| Diagnostic reads during a shot | A CONFIG diagnostic captured READY and HIGH in `0xAE20`. The subsequent poll returned `0x0E20` and the pending event allowed one fresh TEMP read. HIGH survived that read. In a separate trace a latest-TEMP diagnostic consumed the polled event before publication. The shot returned `MEASUREMENT_NOT_READY` with caller output intact. Ambiguous diagnostic reads returned the transport failure and recorded uncertain readiness. None of these losses issued another trigger. |

The pinned CONFIG, one-shot and EEPROM sections support the register fields
and read effects. The two MOD readbacks remain fixture alternatives pending
physical observation. [TMP116 pp. 14, 26, 29](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=14),
[TMP117 pp. 15, 27, 30](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=15),
[TMP119 pp. 17, 34, 37](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=17),
[acquisition tests](../../test/test_acquisition.cpp), [evidence bookkeeping](../../src/DeviceEvidence.cpp).

`cmake --build build/host --config Release --parallel 4` and
`cmake -E chdir build/host ctest -C Release --output-on-failure -R "^(acquisition|conversion_settings|configuration|trust)_cpp$"`
passed all four suites with CMake/CTest 4.0.1. Python 3.12.10 passed
`python -B scripts/check_references.py` (14 PDFs) and
`python -B scripts/check_promptset.py` (55 stages). Item 3 remains in phase b.

## Phase b — Acquisition deadlines and work limits

Item 3 started on clean `main` at `f5b2823644cd02c0b8434a7dc8c24b330a3d1bc4`.
The item 2 commit matched its request in subject, parent and both paths.
The wrapper verified equality with `origin/main` at 03:14:23 Prague time.

| Host test | Deadline or work boundary observed |
| --- | --- |
| `residualOwnedOneShot` | The extended test starts a 12 ms shot at tick `0xFFFFFFF8`. The shutdown baseline receives 12 ms and finishes at tick 1. The trigger receives the remaining 3 ms and succeeds physically at tick 4. The caller receives `OPERATION_TIMEOUT`. Health records five successes and no new failure. The sentinel output and the prior cached TEMP value and timestamp survive. No poll follows. The fixture then accepts a partial conversion, and the next explicit shot clears it with shutdown and publishes its own completed block. |
| `lateContinuousTransfers` | With 4 ms at entry, the guard spends 2 ms. A CONFIG or combined TEMP read completing exactly at tick 2004 counts as a physical success. HIGH remains pending, READY is consumed, and the prior cached value and timestamp survive. The caller receives timeout with its sentinel intact. |
| `continuousPollingAcrossWrap` | At 2 ms resolution the owner credits an overdue poll, then waits 18 ms from its completion for the 15.5 ms nominal period. Eight clock reads per tick still allow a fresh result after wrap. The combined TEMP read proves the transfer shape. Physical latching during continuous updates remains open for stages 40–41. |
| `guardDenialsAndLimits` | With BUSY held and 5,000 ms available, an advancing clock permits 256 guard reads before `WORK_LIMIT`. A frozen clock permits one guard read, then returns `WORK_LIMIT` during the wait. The shared budget permits 1,000,000 unsuccessful same-tick checks and refuses the next. A separate 6 ms one-shot budget ends with `OPERATION_TIMEOUT` while the guard is still BUSY. All three traces contain only guard reads. |

The physical callback updates health and records its completion tick. Acquisition
records a successful trigger before checking expiry. TEMP consumes readiness
before checking expiry and publishes only on timely success.
[Acquisition tests](../../test/test_acquisition.cpp), [transport](../../src/RegisterTransport.cpp),
[acquisition](../../src/Acquisition.cpp), [TEMP publication](../../src/LatestRead.cpp),
[work limits](../../src/OperationBudget.h), [deadline contract](../owner-operations.md#deadline-arithmetic).

`cmake --build build/host --config Release --parallel 4` passed.
`cmake -E chdir build/host ctest -C Release --output-on-failure -R "^(acquisition|latest|health|register_transport)_cpp$"`
passed all four suites with CMake/CTest 4.0.1. With Python 3.12.10,
`python -B scripts/check_references.py` verified 14 PDFs and
`python -B scripts/check_promptset.py` passed for 55 stages.
All three phase-b items are checked. Phase c starts with the two independent
reviews of prompts 20–21.

## Phase c — Independent review, 2026-11-26

Two read-only reviewers reread prompts 20 and 21 and inspected the changes
from `aff8ef39570553cbce1a9eac72c265faf27fb4c1` through
`2a0104139b12bbeeaee4b67458684a80c02eed5e` in parallel. The latter
commit matched the phase-b request in subject, parent `f5b2823` and both
paths. The wrapper verified upstream equality at 03:29:23 Prague time on
November 25. The checkout was clean at review entry.

- **Settings and timing.** The reviewer checked all 96 nominal cycle-table
  cells, the TMP116 standby footnotes and the three electrical rows against
  the independent timing expectations. The 384 public settings cases cover
  both steady modes. The recovery trace retains HIGH and consumes stale
  READY at final CONFIG. With AVG=8 the eighth new input then publishes a
  result under the recovered generation, even when its temperature equals
  the previous sample. [Timing tests](../../test/test_timing.cpp),
  [settings tests](../../test/test_conversion_settings.cpp).
- **Fresh acquisition and deadlines.** The reviewer traced both allowed
  owned MOD readbacks through explicit shutdown, the stale-ready baseline
  and one trigger. A destructive TEMP read can consume the only ready
  event. The shot then ends without a second trigger. At clock wrap the
  last 3 ms reach the trigger callback, whose physical success at expiry
  preserves ownership and leaves the prior sample and timestamp intact.
  The lead checked the wire assertions against the budget, transport and
  publication paths. [Acquisition tests](../../test/test_acquisition.cpp),
  [operation budget](../../src/OperationBudget.h), [TEMP publication](../../src/LatestRead.cpp).

**Finding and correction.** The public fresh-read guide and Doxygen
described CONFIG polling without qualifying the comparator mode. The lead
checked the alert and therm sections in each pinned datasheet. In alert
mode CONFIG acknowledges HIGH and LOW and can deassert a comparator-routed
ALERT pin. In therm mode HIGH persists across the read. The guide now also
shows how to identify acknowledged flags from a CONFIG observation.
`decodeConfiguration`, the register fake and `missingReadyAndThermLevel`
already implement and exercise this distinction. [README](../../README.md#read-a-new-conversion),
[public header](../../include/TMP11x/TMP11x.h),
[TMP116 pp. 15–16](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=16),
[TMP117 pp. 16–17](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=17),
[TMP119 pp. 18–19](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=19).

`cmake --build build/host --config Release --parallel 4` and
`cmake -E chdir build/host ctest -C Release --output-on-failure` passed
all 20 suites with GCC 15.1.0 and CMake/CTest 4.0.1. Doxygen 1.13.2 ran
on the public headers with `WARN_AS_ERROR=YES`. Its XML entry for
`tryReadFresh` contains the corrected therm-mode behavior. Python 3.12.10
verified all 14 PDFs and the 55-stage promptset. Phase d owns final
acceptance on the wrapper commit. Physical MOD readback, flag-clear races
and continuous TEMP byte latching remain open for stages 40–41.
