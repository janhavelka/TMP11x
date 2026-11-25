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
