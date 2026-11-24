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
