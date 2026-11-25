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
