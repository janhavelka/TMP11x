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
