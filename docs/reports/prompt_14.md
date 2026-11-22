# Prompt 14 — Bus-silent binding and lifecycle

## Phase a, 2026-11-22

**PARTIAL.** Prompt 14 is the next eligible stage. Its five prerequisites
are complete and all six inputs are present. The clean entry revision on
`main` was `d7971626fd08ab5f20c0eacef6dee19774984ed6`. Its parent
(`bf5cf9e`), subject and four documentation paths match the prompt 13
closure request. The wrapper recorded equality with `origin/main` at
16:14:25 Prague time. [Stage brief](../prompts/14_binding-lifecycle.md),
[manifest order](../prompts/SEQUENCE.json), [handoffs](../prompts/HANDOFFS.md).

### Inputs verified at entry

| Producer and artifact | What phase b can use |
| --- | --- |
| 8 · `api-contract` | The [operation gates](../api-contracts.md#operation-gates) specify binding by value, teardown on an empty instance and the complete-ID mismatch exception. The [architecture](../architecture.md) assigns one device and its observations to each instance. |
| 9 · `owner-contract` | The [owner contract](../owner-operations.md) fixes callback bounds, stable instance identity, the free-slot rule and token lifetime across teardown. |
| 11 · `value-types` | [Config.h](../../include/TMP11x/Config.h), [Status.h](../../include/TMP11x/Status.h) and [TMP11x.h](../../include/TMP11x/TMP11x.h) supply the binding, settings, status, identity and trust types. [Config.cpp](../../src/Config.cpp) supplies the model lookup. The public header currently exposes value types only. |
| 12 · `register-transport`, `register-fake` | [RegisterTransport.cpp](../../src/RegisterTransport.cpp) validates a borrowed binding for a word transfer. It checks the per-callback cap and handles transport errors. Bind additionally needs the operation timeout, clock contract and offline threshold. The [fake](../../test/support/FakeTmp11xDevice.h) records addresses, request/response bytes, timeouts and scripted failures. |
| 13 · `identity-engine` | [Identity.h](../../src/Identity.h) and [Identity.cpp](../../src/Identity.cpp) return separate family and physical transfer results. The [identity tests](../../test/test_identity.cpp) cover all six selections, mismatch publication and preservation of caller output on a failed transfer. |

### Source decisions

Both sibling checkouts were clean at the revisions below. TMP1x2 copies
the candidate, unbinds before validation and carries dirty/format evidence
across replacement. Its `begin(config)` calls bind then recover. Its
bus-silent `end()` can cancel active work. The `validation` and `lifecycle`
tests and the retained-job cases show which effects the peer exposes.
TMP11x borrows the callback ownership and local teardown conventions. Its
contract requires validation before replacement, fresh target evidence on
acceptance and explicit owner handling of active or retained jobs.
[TMP1x2 implementation](https://github.com/janhavelka/TMP1x2/blob/c91bac63313ea725ce174ab98abc50705c365639/src/TMP1x2.cpp#L19),
[core cases](https://github.com/janhavelka/TMP1x2/blob/c91bac63313ea725ce174ab98abc50705c365639/test/test_core.cpp),
[job cases](https://github.com/janhavelka/TMP1x2/blob/c91bac63313ea725ce174ab98abc50705c365639/test/jobs/test_jobs.cpp#L370).

OPT4001 copies the supplied config and clears old state before validation.
Its `begin` binds, probes and applies settings before marking the device
initialized. Its legacy `end` can write POWER_DOWN. The applicable test
pattern is `test_bind_and_unbind_are_bus_silent`, with callback counts at
each lifecycle boundary. TMP11x requires zero callbacks for `end` too.
[OPT4001 implementation](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/src/OPT4001.cpp#L174),
[native cases](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/test/test_basic.cpp).

All three TI datasheets permit reads during the approximately 1.5 ms
power-up EEPROM load. TMP116 p.17 and its BUSY tables on pp.26 and 29
disagree about the bit during load. A compatible diagnostic read proves
only family compatibility at that instant. Stage 18 establishes missing
startup proof from the completion of its own successful compatible read
under stable power, waits the clock-rounded 3 ms policy interval and checks
the current lock/busy state at `0x04` before a write.
[TMP116 p.17](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 p.18](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18),
[TMP119 p.20](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20),
[stage 7 safety policy](../eeprom-and-reset-policy.md).

### Phase b implementation and acceptance

Deliver `binding-api` and `public-identity-api` on the C++17 device class
in `include/TMP11x/TMP11x.h`, with implementation in `src/`. Keep the
instance at a stable address and delete copy/move construction and
assignment. Keep the desired profile, bound/initialized state, identity
observation, startup evidence and configuration trust distinct. Register
the public lifecycle suite in [CMakeLists.txt](../../CMakeLists.txt) with
the existing fake. Document the available API in its Doxygen and affected
guides, including the [host test inventory](../testing.md).

| Phase b case | Required observation |
| --- | --- |
| Empty instance | `readIdentity(out)` and `probe()` return `NOT_BOUND` with zero I²C. Preserve a prefilled output. Repeated `end()` and `unbind()` succeed locally. |
| Structural admission | Accept all six BOM models and seven-bit addresses `0x48–0x4B`. Require both bus callbacks, positive callback and operation timeouts and clock resolution through `INT32_MAX`, and positive offline threshold. Exercise zero, maximum and out-of-range inputs, including shifted address `0x90`. Clock and GPIO hooks are optional. Borrowed contexts may be null when their hooks support it. Validate locally without invoking hooks. |
| Rejected rebind | Bind A and obtain an ID. Present an invalid B, varying each required field independently. Expect `INVALID_CONFIG`, zero callbacks and preservation of the entire previous binding, desired settings and available evidence. The next explicit probe reaches A with A's timeout. |
| Accepted rebind | Install the candidate directly by value. Changing the caller's copy must leave the installed target intact. Even a same-target rebind after an external reset clears old observations, dirty reasons and startup origin/proof. The new state is bound, uninitialized, with unknown hardware trust. Binding supplies no power-on timestamp. First-mutating-write commitment applies to later settings operations. |
| End, then unbind | End clears observations, startup proof and initialization while retaining the binding and desired settings. A subsequent explicit ID read can still reach the same target. Unbind also releases the binding and both diagnostic calls then refuse locally. Assert zero I²C at each teardown boundary, including zero shutdown writes. |
| Bound diagnostics | With no clock and before initialization, each call uses exactly one combined read at the bound address with its per-callback timeout, TX `[0F]` and two MSB-first response bytes. Exercise both public entry points on compatible and incompatible words. TMP116/N requires `0x1116`. The other four selections accept DID `0x117` with observed revision, including TMP119's published `0x0117` and `0x2117`. Preserve the selected BOM model. |
| Mismatch and transfer failure | A complete incompatible word replaces explicit ID output and returns `IDENTITY_MISMATCH`. Probe returns the same family status. Retain mismatch evidence and invalidate live trust. Only another ID check can re-establish compatibility on that binding. A short transfer or NACK preserves prior complete evidence and caller output. Each attempted diagnostic stops after its one callback. All outcomes leave health accounting untouched and cannot establish initialization or startup proof. |
| Same-address mux routes | Use two instances at `0x48` behind distinct application route contexts. Seed distinguishable IDs. Interleave probes, rebind or tear down A, then read B. Assert exact route, frame count and independent observations. The application owns mux selection and bus serialization. |

Stage 15 asserts neutral diagnostic health counters. Stage 16 integrates
sample caching. Stages 17–18 supply complete profile validation and stage
18 implements verified `begin(config)`/`begin()`. Stage 19 owns explicit
device-state invalidation. Stage 28 implements the active/result slot. Both
occupied states must return `BUSY` before bind, end, unbind or unrelated
I²C. The per-instance token serial survives accepted rebind and teardown.
Phase b can document these future guards without adding job fields.
[Coverage F05](../prompts/FULL_COVERAGE.md), [forward owners](../prompts/HANDOFFS.md).

### Entry checks

Windows, GNU C++ 15.1.0, GNU Make 4.4.1, CMake/CTest 4.0.1 and Python 3.12.10.

| Command | Result |
| --- | --- |
| `cmake -S . -B build-tests-prompt14-a -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release` and `cmake --build build-tests-prompt14-a --config Release --parallel` | PASS, fresh prerequisite build |
| `ctest --test-dir build-tests-prompt14-a -C Release --output-on-failure` | PASS, 9/9 including value types, register transport and private identity |
| `python scripts/check_references.py` and `python scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages |
| Local report links, report whitespace and `git diff --check` | PASS, 23 local links resolve |

The public lifecycle cases above remain open for phase b. MCU links and
physical qualification retain their later gates. Stage 43 owns the first
remote CI checkpoint.
