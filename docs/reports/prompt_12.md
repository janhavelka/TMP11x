# Prompt 12 — bounded 16-bit I2C register transport

## Phase a, 2026-11-22

**PARTIAL.** All five inputs are available. Phase b can implement the private
word transport, command table and reusable register fake. This run produced
the brief and verified the existing host baseline. [Stage prompt](../prompts/12_i2c-register-transport.md),
[artifact owners](../prompts/HANDOFFS.md).

Entry was clean `main` at `d96d54652e023807153f7740c9dee58bd7bada63`,
level with the local `origin/main` for `janhavelka/TMP11x`. The stage 11
closure commit `d585d4a09f215e3c5edfc8df8b9988aba8b79427` has the parent,
subject and four paths expected from the prior run. Its initial push failed
on divergence. It is the first parent of the entry merge, which added the
upstream deployment notes. The wrapper verified synchronization at 14:26:22
Prague time. Stage 12 is the first unfinished entry in manifest order.
[Sequence](../prompts/SEQUENCE.json), [progress ledger](../prompts/PROGRESS.md).

### Inputs at the entry revision

| Artifact | Evidence inspected and use in phase b |
| --- | --- |
| `register-ledger` (5) | Three TI profiles cover the six BOM models. The ledger distinguishes hardware access, release write policy, field roles and read effects. It supplies the reviewed literals for the command table. [Ledger](../registers/tmp11x.json), [safety policy](../eeprom-and-reset-policy.md), [stage 5 report](prompt_5.md). |
| `api-contract` (8) | Both callbacks return `Status`. The adapter validates transfer length. The core owns word publication and checks register access. [API](../api-contracts.md), [architecture](../architecture.md), [stage 8 report](prompt_8.md). |
| `owner-contract` (9) | A callback consumes one transfer allowance. On a timed path the grant is the smaller of the callback cap and remaining budget. The adapter covers lock, route selection and controller work within that grant. [Owner contract](../owner-operations.md), [stage 9 report](prompt_9.md). |
| `value-types` (11) | Reuse `Status`, signed `detail`, `I2cWriteFn`, `I2cWriteReadFn`, `isValidAddress` and the descriptor lookup. The two descriptor rows already select offset/reset support and EEPROM pointers. [Status.h](../../include/TMP11x/Status.h), [Config.h](../../include/TMP11x/Config.h), [ModelDescriptor.h](../../src/ModelDescriptor.h), [Config.cpp](../../src/Config.cpp), [stage 11 report](prompt_11.md). |
| `build-foundation` (10) | Root CMake shares the runtime source list with the IDF component and registers seven tests under `TMP11X_BUILD_TESTS`. The tests-off native consumer links the public C++17 library. No workflow is checked in at this baseline. [CMake](../../CMakeLists.txt), [consumer](../../examples/native_consumer/CMakeLists.txt), [host commands](../testing.md), [stage 10 report](prompt_10.md). |

### Source decisions

| Pinned TI source | Timing, framing and register evidence (PDF pages) |
| --- | --- |
| [TMP116/N, SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf) | p. 6 timing, p. 20 ADD0 and retained pointer, p. 21 timeout recovery, p. 22 Figs. 27–28, pp. 24–31 register map and fields. |
| [TMP117/M/N, SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf) | p. 7 timing, pp. 20–22 STOP, address and timeout, pp. 22–23 Figs. 7-9 and 7-10, pp. 25–32 register map and fields. |
| [TMP119, SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf) | pp. 8–9 timing, pp. 22–24 serial interface, STOP, address and timeout, p. 25 Figs. 7-10 and 7-11, pp. 32–39 register map and fields. |

For F02–F04, ADD0 at GND, V+, SDA and SCL selects seven-bit `0x48`,
`0x49`, `0x4A` and `0x4B` respectively. Every read sends START,
address+W, pointer, repeated START and address+R. The controller ACKs the
MSB, NACKs the LSB and sends STOP. Every write sends pointer, MSB and LSB
in one transaction. Each new access supplies its own pointer, including
consecutive EEPROM words. All three parts specify 1–400 kHz, never drive
SCL and have a held-line/no-clock interface timeout of 20–40 ms (35 ms
typical). TMP117 p. 20 and TMP119 p. 23 describe interface reset on STOP
and ignoring bus activity until START. TMP116 p. 21 supports timeout
recovery only for this comparison. The pointer is retained. Electrical
recovery time and the software timeout grant are independent.
[Coverage F02–F04 and R18](../prompts/FULL_COVERAGE.md).

The read-only sibling checks used pinned source objects. TMP1x2 local HEAD
was `c91bac63313ea725ce174ab98abc50705c365639`, ADS1115 local HEAD was
`1aab07bb84d7494422632eda500913ebf3bee660`, and both worktrees were clean.

- At the TMP1x2 pin, `include/TMP1x2/Config.h` and `src/TMP1x2.cpp`
  show timeout/context forwarding, scratch reads and three-byte writes.
  Use `test/test_core.cpp` for callback counts, clobbered scratch and errors
  after possible acceptance. Its four-register map and byte-latching
  assumptions do not apply. The bus-wide commands in `test/bus/test_bus.cpp`
  are excluded. [TMP1x2 source revision](https://github.com/janhavelka/TMP1x2/tree/afd50889312f6e218a6a34eca687e3e8547d8b40).
- At the ADS1115 pin, `src/ADS1115.cpp` checks callbacks, buffers and lengths
  in `_i2cWriteRaw` and `_i2cWriteReadRaw`. Adapt those guards to the exact
  TMP11x word frames. Count actual invocations independently of the returned
  code. Record any adapted MIT source in the adoption ledger and retain its
  license notice. [ADS1115 source revision](https://github.com/janhavelka/ADS1115/tree/e07622a99fe9f13c5d6a5a2497a8112a14185b9d),
  [source adoption](../source-adoption.md).

### Phase b deliverables and focused acceptance cases

| Owner | Acceptance case |
| --- | --- |
| `command-table` in `include/TMP11x/CommandTable.h` | **Map and access.** For every BOM model compare addresses, field masks, access classes and descriptor capabilities with independent ledger-derived literals. Accept reads of `0x00–0x08` and `0x0F`. Reject `0x09–0x0E` and every nonzero upper nibble before I/O (R18). Reject writes to TEMP, ID, EEPROM_UL and all EEPROM shadows. For TMP116/N, `0x07` is EEPROM3. For TMP117/M/N and TMP119 it is a guarded, opt-in offset. Keep status, reserved bits, one-shot and addressed reset out of ordinary CONFIG writes. |
| `register-transport` under `src/` | **Binding and budget.** Exercise the private seam on a validated borrowed view. Use its address for every call. Cover all address byte values, including shifted `0x90–0x97`, invalid model, either callback missing, null context on stateless callbacks, and invalid lengths. A local refusal costs zero callbacks. With a 50 ms cap and 3 ms left, pass exactly 3 ms. With more time left, pass 50 ms. At expiry, issue no callback. |
| `register-fake` in `test/support/FakeTmp11xDevice.{h,cpp}` | **Frame and ownership.** Capture address, direction, buffer lengths, bytes, grant, result and count. Read `[0x0F]` with scripted RX `[0xA1,0x17]` and expect `0xA117`. For a permitted THIGH word, expect TX `[0x02,0x12,0xA5]`. Alternate bound targets sharing an adapter, then reread the first. Each call supplies its pointer, invokes one callback and leaves routing and locking to the adapter. Drive expected words and access facts from independent literals. |
| Private seam and fake | **Failure and side effects.** Script generic, address and data NACK, short transfer, timeout and bus fault. Normalize `IN_PROGRESS`, a driver-only code or an unknown callback code to `I2C_ERROR`, preserving signed detail. Fail after overwriting one RX byte and verify the caller retains its old word. Fail a write after possible device acceptance and exercise alternative device outcomes. Each invoked callback counts once, with no retry. A failed CONFIG read can lose alert/ready evidence and a failed TEMP read can lose readiness. A status-only callback requires adapter byte-count checks. Buffer sentinels cannot detect false success. |

Keep the checked write policy separate from serialization. Pointer eligibility
alone cannot authorize a mutation. The typed operation owner must establish
startup readiness and a locked, idle `0x04` guard before a volatile write.
Stage 13 consumes the private seam for identity, 14 owns public binding, 15
owns health, 17 owns CONFIG codecs and 18 owns guarded apply. Stage 32 extends
the same fake. Stages 34–35 own adapter deadline enforcement and physical
framing evidence belongs to the adapter and fixture campaigns.

Register the transport suite in the existing CTest entrypoint, compile the
public command table independently, and build and run the framework-free
source consumer. Update public Doxygen, testing instructions and affected
guides with the implementation. Phase c reviews the artifacts and phase d
owns final acceptance, progress and index updates.

### Phase-a verification

- PASS — `python scripts/check_promptset.py` (55 stages),
  `python scripts/check_references.py` (14 pinned PDF hashes),
  `python tools/check_register_ledger.py` (six BOM models) and
  `python tools/test_check_register_ledger.py` (31 regressions).
- PASS — `cmake -S . -B build-tests-prompt12-a -G Ninja -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`,
  `cmake --build build-tests-prompt12-a --config Release --parallel` and
  `ctest --test-dir build-tests-prompt12-a -C Release --output-on-failure`
  passed all seven baseline registrations. CMake 4.0.1, Ninja 1.12.1,
  GNU C++ 15.1.0 and Python 3.12.10.
- NOT RUN — stage 12 transport cases and consumer acceptance await implementation.
  MCU links, deliberate remote CI and physical qualification have later owners.
  No blocker prevents phase b.

## Phase b implementation and checks, 2026-11-22

**PARTIAL.** Entry was clean `main` at
`4b8cde9fb16a5411462499c594c648918501b15b`. Its one report path and
subject match the phase-a request. The wrapper recorded equality with
`origin/main` at 14:30:15 Prague time. The five input artifacts and cited
TI and sibling revisions above still apply. The three output artifacts are
implemented and host-tested. Fresh review and final acceptance remain open.

| Artifact | Delivered behavior and integration owner |
| --- | --- |
| `command-table` | [CommandTable.h](../../include/TMP11x/CommandTable.h) publishes register addresses, field masks and release access classes. [CommandTable.cpp](../../src/CommandTable.cpp) uses the existing model descriptor's capabilities and sparse shadow map. Stage 17 consumes these literals for CONFIG codecs. |
| `register-transport` | [Private interface](../../src/RegisterTransport.h) and [implementation](../../src/RegisterTransport.cpp) check the borrowed binding and frame before each callback. They distinguish a local refusal from an attempted transfer and publish a word only on adapter success. Typed writes require startup and locked-idle evidence. Stage 13 can read ID, stage 14 owns public binding, stage 15 consumes attempt evidence, and stage 18 establishes the operation-local guard. |
| `register-fake` | [Fixture declaration](../../test/support/FakeTmp11xDevice.h) and [implementation](../../test/support/FakeTmp11xDevice.cpp) route transfers by address, copy callback traces and script errors, short reads, flag consumption and alternate post-write words. The tests seed register contents. Stage 32 extends this fixture. |

The registered Release suite runs [metadata checks](../../test/test_command_table.cpp)
and [transport cases](../../test/test_register_transport.cpp) against independent
literal expectations. The public header compiles as the first include of its
own test translation unit. The [native consumer](../../examples/native_consumer/main.cpp)
links the public access lookup from a tests-off build.

| Acceptance case | Observed result |
| --- | --- |
| Map and access | `testCommandTable` and `registerAccessByModel` check every pointer byte for each of six models, including writes with each purpose. TMP116/N refuse writes at `0x07`. The other models require the managed-offset purpose. CONFIG rejects status, reserved, reset, continuous-alias and one-shot encodings before I2C. |
| Binding and budget | `bindingAndBudget` and `invalidFrames` exercise all address bytes, null context with stateless callbacks, missing callbacks, invalid models, buffer shapes and expired grants. Reads and writes both receive 3 ms when 3 ms remains under a 50 ms cap, and 50 ms when more time is available. |
| Frame and ownership | `framesAndSharedAdapter` alternates targets at `0x48` and `0x4B`. A combined ID read sends `[0F]`, receives `[A1 17]` and publishes `0xA117`. THIGH transmits `[02 12 A5]` once. Consecutive shadow reads each send their own pointer. |
| Failure and side effects | `terminalStatusesAndReadPublication` preserves generic and phase-specific NACKs, shorts, timeout and bus errors. Incoherent callback codes become `I2C_ERROR` with the original signed detail. A one-byte read preserves the old word. `ambiguousEffects` tests alternate flag-consumption outcomes and unchanged, partially changed and fully changed words after the same write error. |

### Executed checks

GCC 15.1.0, CMake 4.0.1, Ninja 1.12.1 and Python 3.12.10 on Windows.

- PASS — `cmake -S . -B build-tests-prompt12-b -G Ninja -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`
  and `cmake --build build-tests-prompt12-b --config Release --parallel`.
  A strict build flagged a possibly uninitialized local word buffer. The
  buffer now starts initialized and publication still depends on callback status.
- PASS — `ctest --test-dir build-tests-prompt12-b -C Release -R "^register_transport_cpp$" --output-on-failure`
  (1/1) and `ctest --test-dir build-tests-prompt12-b -C Release --output-on-failure`
  (8/8). Both test executables that cover register metadata and value types
  use active Release checks.
- PASS — `cmake -S examples/native_consumer -B build-consumer-prompt12-b -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`,
  `cmake --build build-consumer-prompt12-b --config Release --parallel` and
  `./build-consumer-prompt12-b/tmp11x_native_consumer.exe` (printed `0.1.0`).
- PASS — `python tools/check_register_ledger.py` (six models),
  `python tools/test_check_register_ledger.py` (31 regressions),
  `python scripts/check_references.py` (14 PDF hashes),
  `python scripts/check_promptset.py` (55 stages) and `git diff --check`.
- NOT RUN — MCU links, adapter lock/controller deadline enforcement, physical
  framing and silicon qualification. Remote CI follows the stage 43 checkpoint.
  No blocker for phase c.
