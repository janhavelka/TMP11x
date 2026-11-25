# Prompt 20 — Conversion settings and timing

## Phase a — Requirements and baseline, 2026-11-25

**PARTIAL.** All seven inputs are available for phase b. The clean baseline was
`aff8ef39570553cbce1a9eac72c265faf27fb4c1` on `main`. Its parent
`7b214030064bd0234841d737794d7f707a020892`, subject and four changed paths
match the stage 48 closure request. The wrapper recorded equality with
`origin/main` at 00:14:51 Prague time. The local tracking ref still matched
at entry. The manifest places 20 immediately after 48. The progress ledger
records completion of prerequisites 6, 8, 16, 17, 18, 19 and 48.
[Stage requirements](../prompts/20_conversion-settings.md), [sequence](../prompts/SEQUENCE.json),
[progress](../prompts/PROGRESS.md).

### Inputs verified at the baseline

| Input | Evidence and use in phase b |
| --- | --- |
| `behavior-contract` | Read the mode transitions, timing conditions, polling policy and open silicon questions in [device behavior](../device-behavior.md). |
| `behavior-vectors` | Checked the three nominal timing matrices against the pinned PDFs. Use the independent active-time triples, standby footnotes and named block and generation traces as test oracles. [Literal vectors](../registers/behavior-vectors.json). |
| `config-codec` | The typed selectors already cover two replayable modes, eight CONV codes and four AVG counts. The codec validates candidates, writes canonical continuous mode and accepts its readback alias. [Types](../../include/TMP11x/Config.h), [codec](../../src/Configuration.cpp), [encoding tests](../../test/test_configuration.cpp). |
| `safe-apply-engine` | The candidate overload of `detail::Apply` checks the current profile against the owned target before the first write. The replay overload allows explicit recovery from drift. Reuse the candidate path for settings. [Apply phases](../../src/Apply.h), [implementation](../../src/Apply.cpp). |
| `config-trust` | CONFIG observations retain alerts and account for consumed readiness. A timely final match establishes the generation and baseline. The public snapshot distinguishes desired settings, observations and trust. [Read effects](../../src/DeviceEvidence.cpp), [device API](../../include/TMP11x/TMP11x.h), [trust tests](../../test/test_trust.cpp). |
| `register-fake` | The fake records bus frames and scripts ambiguous writes, ignored writes and destructive reads. Phase b needs a test fixture that advances individual conversions through a complete block and can abort a partial block. [Register fixture](../../test/support/FakeTmp11xDevice.cpp). |
| `audit-18-19` | Audit 48 closed the guard-renewal, partial-verification and event-provenance findings. The baseline suite exercises those cases. [Audit acceptance](prompt_48.md#phase-d-acceptance), [guarded apply tests](../../test/test_apply.cpp). |

### Timing sources and acceptance values

The following pages use one-based PDF numbering. The electrical rows give
min / typ / max milliseconds for a single conversion. Typical values use
25 °C and 3.3 V. Each BOM selection retains its own operating and accuracy ratings.

| Models | Electrical row and conditions | Modes, averaging and nominal CC period |
| --- | --- | --- |
| TMP116, TMP116N | 13.5 / 15.5 / 17, one conversion, −55 to +125 °C at 1.9–5.5 V. [SBOS740A p.5](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5). | pp.12–14, 26–27. Table 7 gives standby footnotes for each cell. [Table 7](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=27). |
| TMP117, TMP117M, TMP117N | 13 / 15.5 / 17.5 in **one-shot** mode. Applying the row to continuous conversion is an estimate. [SNOSD82D p.6](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6). | pp.12, 14–15, 27–28. [Table 7-7](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28). |
| TMP119 | 13 / 15.5 / 17.5 without internal averaging. Both this row and the TMP117 row use −55 to +70 °C at 1.7–5.5 V or −55 to +150 °C at 1.8–5.5 V. [SNIS236 p.7](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7). | pp.14, 16–17, 34–35. TI recommends limiting I²C activity during active conversion for best precision (p.23). [Table 8-6](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=35). |

Verify all 32 CONV × AVG pairs against the literal cells for each selection.
The nominal continuous period includes active time and standby. Its reciprocal
gives an estimated update rate. The device exposes no sample counter or
specified maximum continuous period. Active-time estimates scale the electrical
row by 1, 8, 32 or 64 under the same conditions, assuming successive
conversions with no added block overhead. One-shot has zero standby and its
estimated duration is independent of CONV.

Three cases distinguish the quantities a timing helper must report.

- With CONV 100 and AVG 01 the nominal period is 1000 ms, the table budgets 125 ms active and 875 ms standby, and the derived typical active duration is 124 ms.
- With CONV 100 and AVG 11 standby is zero. The 64-conversion typical duration is 992 ms. Derived upper estimates are 1088 ms for TMP116/N and 1120 ms for the other four selections.
- TMP117 with CONV 010 and AVG 01 has a nominal 250 ms period and a derived active upper estimate of 140 ms. Stage 6's continuous conversion-window policy gives 323 ms at 1 ms resolution, rounding up `max(1.25 × 250, 140) + 10`. Its one-shot window is 150 ms. The owner budgets setup and transfers separately. Elapsed time alone supplies no completion evidence. [Polling policy](../device-behavior.md#polling-and-deadlines).

### Phase-b work and test cases

1. Implement `setMode`, `setCycle` and `setAveraging` on the public device and add pure timing helpers. Exercise both steady modes × eight cycles × four averages on all six selections (384 profiles). Check that changing one selector preserves the others, the binding and the final requested mode. Cover invalid enum casts, an uninitialized binding and known dirty state with zero callbacks. Reads preserve the conversion settings. [Public contract](../api-contracts.md), [current implementation](../../src/TMP11x.cpp).
2. Trace a managed change from a verified profile through the operation-local EEPROM guard, explicit shutdown, field application and verification, CONFIG baseline in shutdown, desired mode and final CONFIG readback. Include limit-only and routing-only changes through today's apply seam. At the first attempted mutation, retain candidate intent and invalidate old readiness. A timely final match establishes a new baseline while preserving returned alert evidence and consuming READY. Check that a previous sample retains its original generation. Preflight drift, an ambiguous write, or a late or mismatched final read cannot establish a new generation. Require explicit recovery after invalidation or unresolved drift. [Apply regressions](../../test/test_apply.cpp), [generation trace](../registers/behavior-vectors.json).
3. Execute `reset-and-two-averaging-blocks` with synthetic counts whose means are exact. Seven inputs leave TEMP at `0x8000` and READY clear. The eighth publishes `0x0007`. After acknowledgement, the next eight inputs publish `0x001B`. Entering shutdown three inputs into the third block leaves that second result intact. Keep the fixture silent about fractional rounding, same-value or in-flight CONFIG writes and flag retention on shutdown where TI leaves them open. Block-end comparator timing is inferred for TMP116/N and explicit for TMP117/M/N and TMP119. [Stage 6 evidence](prompt_6.md).
4. Register the focused native cases in [CTest](../../CMakeLists.txt) and document the setters' I/O and timing units in public Doxygen and the affected guides. Explain how block length and standby affect noise, latency and average current under the exact part's conditions. Stage 21 consumes the verified continuous baseline and timing for fresh acquisition and explicit one-shot. Stage 22 owns the public limits and alert setters. Audit 49, the synchronous API in 27 and cooperative measurement in 30 also consume `conversion-settings`. [Artifact ownership](../prompts/HANDOFFS.md).

### Sibling patterns

The clean OPT4001 and TMP1x2 checkouts were at
`f17d4944640145773d3331e155a5bc728fcc4c66` and
`0791429f8fce1170e2082f1b011456fc461318b8`. The inspected setter
implementations matched the project pins. OPT4001's `setMode` directs
one-shot requests to an explicit operation. Its rollback on apply failure
would lose the target of a possibly successful TMP11x write. TMP1x2's
`setConversionRate` copies the requested field into a candidate. Its four
fixed rates and 35 ms format-transition waits apply to different silicon.
TMP11x uses its own timing tables, fixed temperature format and guarded
generation boundary. [OPT4001 source](https://github.com/janhavelka/OPT4001/blob/441ef579d7fa3f7bca7665f52632fdaddd306300/src/OPT4001.cpp),
[TMP1x2 source](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2Configuration.cpp).

### Baseline checks

| Check | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4`, then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 16 existing tests. CMake/CTest 4.0.1, GCC 15.1.0. |
| `python -B scripts/check_references.py` and a read-only PyMuPDF comparison of the three timing tables with the stage 6 vectors | PASS, 14 pinned PDFs, 96 nominal period cells and 12 derived active-time triples. |
| `python -B scripts/check_promptset.py` and report whitespace and local-link checks | PASS with Python 3.12.10. |

Phase b owns implementation and the new test results. Physical qualification
remains open for stages 40–41. Stage 43 owns the first remote checkpoint.

## Phase b implementation

**PARTIAL, 2026-11-25.** The three public setters and the timing query are
implemented. The clean input was `4490d2d80cb319bf6a13c6dba86cc85ca168e98a`.
Its parent, subject and sole changed path matched the phase-a request. The
first push received a server error. The wrapper recorded synchronization at
00:42:05 Prague time, and both local refs matched at this run's entry.
The seven inputs verified in phase a remain available at this revision.

`setMode`, `setCycle` and `setAveraging` copy the desired settings, change
one selector and run the same synchronous apply loop as initialization and
recovery. Settings changes use the candidate overload, which checks the
previous owned profile before its first write. A mismatch found there keeps
the original target. An attempted mutation commits candidate intent and
invalidates prior readiness. Only timely final verification establishes the
next generation. Stage 21 can use its baseline for continuous acquisition.
[Public setters](../../include/TMP11x/TMP11x.h), [implementation](../../src/TMP11x.cpp),
[apply engine](../../src/Apply.cpp).

`getConversionTiming` reports integer microsecond durations and nominal
continuous results per second. The period and standby follow the published
table's rounded active budget. The block duration scales the separate
single-conversion electrical row. Thus a TMP117 with CONV 010 and AVG 01
has a 250,000 µs nominal cycle, 125,000 µs nominal standby, 124,000 µs
typical active time and a 140,000 µs upper active estimate. Its library
windows are 322,500 µs for continuous acquisition and 150,000 µs for
one-shot, before rounding to the owner's tick. A one-shot has zero standby.
The TMP117/M/N electrical row describes one-shot under the conditions cited
in phase a. Its continuous use remains an estimate. The header is usable
without a device instance. [Timing API](../../include/TMP11x/Timing.h),
[calculation](../../src/Timing.cpp), [operating conditions](../device-behavior.md#conversion-modes-and-intervals).

| Registered suite | Executed evidence |
| --- | --- |
| [timing_cpp](../../test/test_timing.cpp) | `timingTable` compared all 32 period and standby cells per selected model with independent literals. It checked count, update rate, both windows, the single-conversion electrical limits and all four derived block triples. The active estimates and one-shot window were identical across all eight cycle selectors for each AVG. Invalid queries preserved the output. |
| [conversion_settings_cpp](../../test/test_conversion_settings.cpp) | `allPublicProfiles` exercised 384 profiles through public setters and checked CONFIG and TEMP reads, the physical mode, other owned fields and bound identity. `localRefusalsAndPreflightDrift` checked zero clock/bus callbacks on invalid, uninitialized and known-dirty requests, as well as a read-only preflight mismatch. `generationChanges` checked all 14 frames for cycle, averaging, limit-only and route-only changes, historical sample attribution and final alert/READY effects. `incompleteChangesKeepIntent` checked an ambiguous first write and mismatched or late final reads followed by explicit recovery. |
| [averaging_blocks_cpp](../../test/test_averaging_blocks.cpp) | `allBlockLengths` checked publication on inputs 1, 8, 32 and 64 for the corresponding AVG codes. `resetAndTwoAveragingBlocks` held TEMP at `0x8000` for seven inputs, published `0x0007` and then `0x001B` at block ends, and aborted three inputs into the next block via the public shutdown setter. Eight new inputs after resumption published `0xFFF8`. The test checked READY acknowledgement separately from comparator latches. |

The [register fake](../../test/support/FakeTmp11xDevice.cpp) accepts exact
integer means for these continuous ALERT-mode traces. Block-end comparison
is inferred for TMP116/N and explicit for TMP117/M/N and TMP119. Fractional
rounding, same-value and in-flight CONFIG writes, and flag retention on
shutdown remain outside the silicon claims. The routing and limit cases use
the existing candidate seam. Stage 22 owns their public setters.

| Check on the phase-b tree | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 19 tests including all three new suites, with CMake/CTest 4.0.1 and GCC 15.1.0. |
| `cmake --build build/consumer --config Release --parallel 4` and `build/consumer/tmp11x_native_consumer.exe` | PASS, source consumer linked and printed `0.1.0`. |
| `doxygen -` with `INPUT=include/TMP11x`, XML output and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. The XML index under `build/p20b-docs` includes all four public functions. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, local target and whitespace checks | PASS with Python 3.12.10. Verified 14 pinned PDFs, the 55-stage manifest and 161 local Markdown targets. A read-only PyMuPDF comparison matched all 96 source period cells to the stage 6 vectors and native test literals. Rechecked the electrical rows and the unchanged sibling source revisions listed above. |

Phase c reviews these artifacts against the pinned sources and contracts.
Physical qualification and the first remote checkpoint remain with stages
40–41 and 43. This phase has no software blocker.

## Phase c review

**PARTIAL, 2026-11-25.** The review began on clean `main` at
`c41a1c119384e48ba07b6ae128ebb1c52eb6bf1f`. Its parent, subject and all
20 paths matched the phase-b request. The wrapper recorded upstream equality
at 01:04:09 Prague time. Both local refs matched on entry. The review covered
the phase-b diff, the shared apply and observation paths, all three new test
suites and the seven prerequisite artifacts recorded above.

| Finding | Correction and regression evidence |
| --- | --- |
| A setter could observe HIGH and READY in a verified continuous profile, then time out reading TLOW before its first write. CONFIG had acknowledged both flags. The previous profile and its baseline were still valid, but the driver had dropped READY. | `readinessAfterPreflightFailure` reproduced the loss at `R0F R04 R01=A220 R03(timeout)`. Preflight now retains eligible, timely readiness for that generation. A subsequent latest read consumes it. A separate late preflight returning `6220` kept the raw flags and LOW event, with zero writes and zero new ready credit. [Observation logic](../../src/DeviceEvidence.cpp), [setter tests](../../test/test_conversion_settings.cpp). |
| A scripted failure of the first shutdown write during an averaging block could return the fixture's generic error and ignore the specified post-error CONFIG word. | `ambiguousShutdownDuringBlock` reproduced the status mismatch. The fake now preserves the adapter's `I2C_TIMEOUT`, detail −31 and scripted word `0120`. It declines synthetic conversions while partial accumulation is unknown. The driver retains the dirty candidate, and explicit guarded recovery establishes CC at `0320`. The eighth new input publishes the first result. [Register fake](../../test/support/FakeTmp11xDevice.cpp), [setter tests](../../test/test_conversion_settings.cpp). |
| The original abort trace inspected TEMP after the setter returned. A spurious READY from the abort could already have been acknowledged. | On each of the six selections, `resetAndTwoAveragingBlocks` now checks `W01=0620 R01=0620` at entry to shutdown after three partial inputs. READY was clear in the first post-write observation and TEMP retained `001B`. [Averaging tests](../../test/test_averaging_blocks.cpp). |

Two read-only reviews ran in parallel. The timing review compared the 96 TI
period cells to the stage 6 vectors and native literals, all 32 TMP116 standby
footnotes to the standby literals, and the 12 count-scaled triples to the three
electrical rows. The block review checked the two completed means, partial
abort and comparator evidence against TMP116 pp.12–14, TMP117 pp.12, 14–15
and TMP119 pp.14, 17. TMP116/N block-end comparison remains an inference.
The sibling setter implementations still match the phase-a pins.
[Timing sources and conditions](../device-behavior.md#conversion-modes-and-intervals),
[stage 6 vectors](../registers/behavior-vectors.json).

| Final review check | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 19 suites with GCC 15.1.0 and CMake/CTest 4.0.1. The settings suite exercised all 384 steady profiles, and timing covered all 192 model/cycle/averaging selections. |
| `cmake --build build/consumer --config Release --parallel 4` and `build/consumer/tmp11x_native_consumer.exe` | PASS, source consumer printed `0.1.0`. |
| `doxygen -` with public headers, XML and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2. The index contains the three setters and the timing query. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, local Markdown targets and `git diff --check` | PASS with Python 3.12.10. The 14 pinned PDFs and the 55-stage manifest verified. |

Phase d owns final acceptance of the wrapper commit, the report index and
the progress row. Physical qualification belongs to stages 40–41 and the
first remote CI checkpoint belongs to stage 43.
