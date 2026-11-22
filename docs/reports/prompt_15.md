# Prompt 15 — Passive transport health

## Phase a brief, 2026-11-22

**PARTIAL.** All four prerequisites are complete. Prompt 15 is the next
eligible stage after 14. The clean entry revision on `main` was
`143406078fa52da37e6bd2cb2627b873de0abe37`. Its parent was `338bfe6`.
The subject and four documentation paths match `commit_20261122-171203.json`.
The wrapper recorded upstream equality at 17:15:03 Prague time and the local
tracking ref matched at entry. [Stage requirements](../prompts/15_passive-health.md),
[execution order](../prompts/SEQUENCE.json), [artifact handoffs](../prompts/HANDOFFS.md).

### Inputs at the baseline

| Producer and input | Evidence available to phase b |
| --- | --- |
| 8 · `api-contract` | The value-copy snapshot, online predicate, callback accounting and lifecycle reset boundaries are specified in the [API contract](../api-contracts.md) and [engineering contract](../prompts/ENGINEERING.md#lifecycle-health-trust). |
| 11 · `value-types` | `DriverState` and `Status` already exist. The binding supplies an optional wrapping millisecond clock and a positive eight-bit failure threshold, default 5. [Status.h](../../include/TMP11x/Status.h), [Config.h](../../include/TMP11x/Config.h). |
| 12 · `register-transport` | Each word helper returns a `TransferResult` with the normalized callback status and an explicit `attempted` flag. A local refusal has `attempted == false`. A combined word read invokes one callback. [Transport interface](../../src/RegisterTransport.h), [implementation](../../src/RegisterTransport.cpp). |
| 12 · `register-fake` | The fake records one trace entry per callback, including faults. Its script can deliver a partial word or an ambiguous write. Use that trace to check both the physical count and the status presented to the owner. [Fake interface](../../test/support/FakeTmp11xDevice.h), [transport cases](../../test/test_register_transport.cpp). |
| 14 · `binding-api` | Accepted bind, end and unbind call `clearObservations()`. Rejected bind returns before replacement. Public probe and identity reads share the ID engine, which retains the successful transfer alongside a logical mismatch. A probe mismatch clears live trust. Keep transport history outside that trust reset. [Device class](../../include/TMP11x/TMP11x.h), [lifecycle implementation](../../src/TMP11x.cpp), [ID engine](../../src/Identity.cpp). |

### Sibling source decisions

The three sibling worktrees were clean. The inspected source and test files
matched the stage 1 pins at their respective local heads, TMP1x2 `c91bac6`,
OPT4001 `f17d494` and ADS1115 `1aab07b`. The comparison used read-only Git
objects. The six TMP11x models share this host-side accounting policy. Stage
12 supplies their checked wire framing. [Revision inventory](../library-comparison.md),
[register transfers](../board-integration.md#register-transfers).

- TMP1x2 retains the last fault across success, saturates its counters and
  records success and fault times with separate validity flags. Its test
  distinguishes a clockless observation from a valid time of zero. Use these
  conventions. TMP11x updates state on every tracked callback, including
  before initialization, and counts every normalized physical failure. The
  peer gates transitions on initialization and filters error codes.
  [Status](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/include/TMP1x2/Status.h),
  [snapshot](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/include/TMP1x2/TMP1x2.h),
  [track](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2.cpp#L120),
  [healthAndInvalidation](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/test/helpers/test_helpers.cpp#L197).
- OPT4001 blocks ordinary reads once OFFLINE. TMP11x leaves each otherwise
  authorized attempt available to the application, which owns backoff and
  bus recovery. [OPT4001 implementation](https://github.com/janhavelka/OPT4001/blob/441ef579d7fa3f7bca7665f52632fdaddd306300/src/OPT4001.cpp),
  [offline test](https://github.com/janhavelka/OPT4001/blob/441ef579d7fa3f7bca7665f52632fdaddd306300/test/test_basic.cpp#L3100).
- ADS1115 separates raw callback completion from local admission and tests an
  ordinary read from OFFLINE. TMP11x already has an explicit attempt flag
  and normalizes incoherent callback returns to `I2C_ERROR` with the original
  signed detail. Use that result once at the tracked boundary.
  [ADS1115 transport](https://github.com/janhavelka/ADS1115/blob/e07622a99fe9f13c5d6a5a2497a8112a14185b9d/src/ADS1115.cpp#L2518),
  [passive offline test](https://github.com/janhavelka/ADS1115/blob/e07622a99fe9f13c5d6a5a2497a8112a14185b9d/test/test_basic.cpp#L1361).

### Phase b deliverable and acceptance cases

Deliver `transport-health` through the public `HealthSnapshot`,
`healthSnapshot()` and `isOnline()`, backed by per-instance accounting at
the private checked transport boundary. Both getters read memory only.
Use the binding's clock at callback completion when available. Register
the focused native cases with the existing CMake suite, document the fields
in public Doxygen and exercise the public getters from the external consumer.
Show how to inspect health in the README and update the host test inventory.
[CTest registration](../../CMakeLists.txt), [consumer](../../examples/native_consumer/main.cpp),
[test guide](../testing.md), [F18 coverage](../feature-coverage.md).

| Case | Required observation |
| --- | --- |
| First callback and disappearance | Start uninitialized at UNINIT. A first tracked success gives READY. At threshold 2, two address NACKs give DEGRADED then OFFLINE. A further failure keeps OFFLINE and records the latest fault. One ordinary tracked success from OFFLINE reaches READY, clears the streak and preserves fault detail and time. Check thresholds 1 and 255 as well. |
| Mixed callbacks in one operation | Check each authorized read and guarded write against the trace. Two successes followed by a short transfer yield two successes, one failure and a streak of one. Prefix bytes and ambiguous write effects never add another outcome. An incoherent callback return counts as one normalized physical error. |
| Preconditions and logical results | An invalid pointer, prohibited write, invalid binding or expired budget before the callback adds zero. A successful tracked ID read followed by a family mismatch adds one success. A later settings mismatch, not-ready decision or owner timeout adds zero more. A physical `I2C_TIMEOUT` counts once. |
| Neutral diagnostics | With populated OFFLINE history, use public `probe()` and `readIdentity()` for a matching word, an incompatible word and a transfer fault. Verify direct status, the single callback per bound call and preservation of counters, state, fault and times. |
| Clock and counter boundaries | Distinguish absent time from a valid zero. Cross the 32-bit clock wrap, including repeated errors, and preserve the most recent fault's full `Status`. Seed counters near their maxima at the private seam and check saturation of both totals and the failure streak. |
| Lifecycle and predicate | Accepted bind, same-target rebind, end and unbind reset history locally. Invalid rebind preserves it. Exercise all eight combinations of initialized and state in the pure online predicate. Only initialized READY or DEGRADED reports online. A tracked success alone establishes neither profile trust nor fresh conversion evidence. |

Stage 16 consumes the tracked path for latest reads. Stage 18 exercises
startup, apply and recovery before and after initialization. Stage 19 tests
explicit trust invalidation with transport history retained. Stage 26 extends
neutrality checks to raw diagnostics. Stage 31 covers callbacks that complete
after an operation deadline.

### Phase a checks

The following checks passed on the entry tree using GNU C++ 15.1.0,
CMake/CTest 4.0.1, Python 3.12.10 and Git 2.52.0.windows.1.

| Command | Baseline result |
| --- | --- |
| `cmake -S . -B build-tests-prompt15-a -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build-tests-prompt15-a --config Release --parallel 4` | Fresh Release configure and build passed. |
| `ctest --test-dir build-tests-prompt15-a -C Release --output-on-failure` | All 10 existing tests passed, including value types, transport, identity and binding. |
| `python -B scripts/check_promptset.py` and `python -B scripts/check_references.py` | 55 manifest stages and all 14 pinned PDF hashes passed. |

The phase-b cases above remain open. MCU links, physical qualification and
the stage 43 remote CI checkpoint remain with their assigned owners.

## Phase b implementation, 2026-11-22

**PARTIAL.** Implemented `transport-health` on clean `main` at
`5e197995815b1bf9cd71617af82be361b12aa505`. Its parent, subject and sole
report path match the phase-a handoff and commit request. The wrapper
verified upstream equality at 17:30:09 Prague time. The three clean
sibling checkouts still matched the phase-a revisions. The wire unit
remains a single word transfer as shown in TMP116 p. 22, TMP117 pp. 22–23
and TMP119 p. 25. [Transfer figures](../board-integration.md#register-transfers).

`healthSnapshot()` copies the instance's tracked history and takes its
initialization flag from the current lifecycle state. `isOnline()` reads
that same lifecycle state and the tracked transport state. Accepted bind,
end and unbind clear history. Clearing live trust after an incompatible ID
keeps the history available. [Public declarations](../../include/TMP11x/TMP11x.h),
[device lifecycle](../../src/TMP11x.cpp), [API contract](../api-contracts.md).

Managed callers pass `RegisterTransport::tracked(device)` to a register or
identity read, or to a guarded write. The frame helper normalizes each
terminal callback result and records it once. The optional clock is sampled
there, after callback return. The ID engine passes the same view through
to the read before classifying the word. Public diagnostics use the
health-neutral binding. Neither frame admission nor callback execution
checks the OFFLINE state. [Transport view](../../src/RegisterTransport.h),
[accounting](../../src/RegisterTransport.cpp), [identity engine](../../src/Identity.cpp),
[architecture](../architecture.md).

### Native cases

The tests compare per-instance snapshots against the stage 12 fake's
callback trace. The clocked fixture derives its tick from the completed
trace length, so sampling before the callback would change the asserted
time. [Test source](../../test/test_health.cpp), [CTest inventory](../testing.md).

| Case | Observation |
| --- | --- |
| `disappearanceAndClockWrap` | For each of the six BOM selections, a successful read, two address NACKs and a bus fault take an uninitialized instance through READY, DEGRADED and OFFLINE. The next ordinary read succeeds. It retains the last fault and its valid wrapped timestamp of zero. |
| `mixedTransfersAndRefusals` | A guard read, allowed limit write and short readback yield two successes and one failure. An ambiguous write timeout counts once. An incoherent callback retains its signed detail as an I2C error. Invalid binding, invalid pointer, protected write and exhausted budget add zero. A complete incompatible ID adds one tracked success. |
| `neutralDiagnosticsAndLifecycle` | With populated OFFLINE history, probe and explicit ID read each return a match, mismatch or physical timeout directly. Every call adds one bus trace entry and leaves clock and history intact. A rejected bind preserves history. Same-target bind, end and unbind clear it locally. |
| `saturatingCounters` | With the threshold at 255, seeded counts cross from 254 failures to OFFLINE and receive another error. Both totals and the streak stop at their maxima. A subsequent read clears the streak. |
| `onlinePredicate` | All eight initialization/state combinations agree with the public rule. Only initialized READY or DEGRADED returns true. |

### Phase b checks

Windows, GNU C++ 15.1.0, CMake/CTest 4.0.1, Python 3.12.10 and Doxygen 1.13.2.

| Command or check | Result |
| --- | --- |
| `cmake -S . -B build-tests-prompt15-b -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build-tests-prompt15-b --config Release --parallel 4` | PASS, strict C++17 Release build. |
| `ctest --test-dir build-tests-prompt15-b -C Release -R "^health_cpp$" --output-on-failure`, then repeat without the filter | PASS, 1/1 focused and 11/11 overall. |
| `cmake -S examples/native_consumer -B build-consumer-prompt15-b -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`, then `cmake --build build-consumer-prompt15-b --config Release --parallel 2` and `./build-consumer-prompt15-b/tmp11x_native_consumer.exe` | PASS, public getters linked and returned the initial state. The consumer printed `0.1.0`. CTest's `-N` listed zero tests in the consumer build. |
| `doxygen -` over the public headers with XML and `WARN_AS_ERROR=YES`. Compile the README C++ blocks with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -` | PASS, all ten snapshot fields and both getters had descriptions in the XML. Both examples compiled. |
| `python -B scripts/check_promptset.py`, `python -B scripts/check_references.py` and `git diff --check` | PASS, 55 manifest stages, 14 pinned PDF hashes and clean whitespace. |

Phase c owns fresh review of these artifacts. Managed latest reads (16),
initialization and settings comparisons (18–19), acquisition readiness (21),
broader raw diagnostics (26) and callback completion past the owner deadline
(31) retain their integration checks. MCU links, physical tests and remote
CI were not run in phase b.

## Phase c review, 2026-11-22

**PARTIAL.** The review started on clean `main` at
`5eb52c18185d2f2360259d49a718e2778ab70190`. The parent, subject and all
16 paths matched the phase-b commit request. The wrapper recorded upstream
equality at 17:48:56 Prague time. Two read-only reviewers examined the
physical accounting and lifecycle code, and the test and documentation
coverage, respectively.

Both read and write callbacks record one normalized outcome on return.
Admission checks precede both callbacks. The ID engine classifies a complete
word after the transport has recorded the outcome. Public identity diagnostics
use a neutral binding, and an incompatible ID clears live trust while
preserving history. The existing tests exercise these boundaries, saturation,
clock wrap, teardown and the online predicate. [Transport](../../src/RegisterTransport.cpp),
[identity](../../src/Identity.cpp), [lifecycle](../../src/TMP11x.cpp).

The review found two gaps in write coverage. All tracked writes in the
health suite used a clockless binding. The missing startup and locked-idle
proofs were tested only through the neutral transport. The new
`guardedWriteTimestamps` case checks a successful write at tick 1, a data
NACK at tick 2 and an ordinary successful write from OFFLINE at tick 3.
Each accepted attempt samples the clock once. Between the NACK and recovery,
the test withholds each proof in turn and checks the refusal, attempt flag,
trace length, clock count and complete saved history. Recovery retains the
fault and its timestamp. [Health cases](../../test/test_health.cpp),
[write admission cases](../../test/test_register_transport.cpp).

| Local check | Review result |
| --- | --- |
| Fresh MinGW Release configure and build with the phase-b commands, using `build-tests-prompt15-c` and `build-consumer-prompt15-c` | PASS with GNU C++ 15.1.0 and CMake 4.0.1. The standalone tests-off consumer linked and printed `0.1.0`. |
| `ctest --test-dir build-tests-prompt15-c -C Release -R "^health_cpp$" --output-on-failure`, then the full suite without the filter | PASS after the test change, 1/1 focused and 11/11 overall. |
| Link the health test with two separately modified temporary copies of `RegisterTransport.cpp` | The new case detected both injected faults at the expected assertions. One omitted the write clock and the other sampled the clock on a refused write. |
| `python -B scripts/check_promptset.py`, `python -B scripts/check_references.py`, `git diff --check` and `git diff --cached --check` | PASS with Python 3.12.10 and Git 2.52.0.windows.1. All 55 manifest stages and 14 pinned PDFs validated. |

Phase d owns final acceptance and the report index and progress entries.
Managed-operation integration, MCU links, hardware and remote CI retain the
owners recorded above.
