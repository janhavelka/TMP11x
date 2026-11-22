# Prompt 16 — Signed temperature and threshold codecs

## Phase a brief, 2026-11-22

**PARTIAL.** All seven prerequisites (5, 6, 11–15) are complete. Prompt 16 follows 15 in the execution sequence. Phase b owns two deliverables: the pure `temperature-codec` and the tracked `latest-read-engine` with its per-instance sample cache. [Prompt 16](../prompts/16_temperature-codec.md), [manifest](../prompts/SEQUENCE.json), [handoffs](../prompts/HANDOFFS.md).

The clean entry revision on `main` was `564d30fd99ea1bbbf14f9caaae453bba1607437e`, with zero divergence from local `origin/main`. Its parent was `e50959c02d373398de30d5a6a6bf8acd48d02f42`. The subject and three documentation paths matched `commit_20261122-181203.json`. The wrapper verified upstream equality at 18:14:42 Prague time.

### Inputs verified at entry

| Producer and artifact | Evidence usable by stage 16 |
| --- | --- |
| 5 · `register-ledger`; 6 · `behavior-vectors` | The per-model TEMP, THIGH and TLOW entries identify word format and read effects. The vector sets `temperature_and_limits` and `host_quantization` give independent wire bytes and inverse examples. The reset, repeated-read and late-success traces supply separate publication cases. [Ledger](../registers/tmp11x.json), [vectors](../registers/behavior-vectors.json), [behavior contract](../device-behavior.md). |
| 11 · `value-types` | `Sample` already holds raw, counts, Celsius, timestamp/validity, acquisition kind, freshness, profile generation, trust and offset provenance. `Limits` carries two signed count values. [Sample](../../include/TMP11x/TMP11x.h), [Limits](../../include/TMP11x/Config.h), [status codes](../../include/TMP11x/Status.h). |
| 12 · `register-transport`, `register-fake` | `readWord` returns the attempt flag and normalized status, clips the callback grant, and commits a complete MSB-first word only on success. The fake traces bytes, grants and results, clears DATA_READY on TEMP and can deliver a prefix with optional flag consumption on failure. [Word transport](../../src/RegisterTransport.cpp), [fake](../../test/support/FakeTmp11xDevice.cpp). |
| 13 · identity prerequisite; 14 · `binding-api` | Binding and compatible ID reads leave the device uninitialized. A complete mismatch clears live evidence. Accepted bind, end and unbind clear observations. Add the sample cache to those lifecycle boundaries and preserve it on rejected rebind. [ID engine](../../src/Identity.cpp), [device lifecycle](../../src/TMP11x.cpp). |
| 15 · `transport-health` | `RegisterTransport::tracked(device)` records one physical outcome and optional completion tick for each callback. The latest reader needs that same completion time for its observation. An owner timeout adds zero physical faults. [Transport view](../../src/RegisterTransport.h), [health cases](../../test/test_health.cpp). |

### Exact-part and sibling source checks

| Selected BOM model | Signed examples, TEMP, CONFIG read effects, THIGH/TLOW (one-based PDF pages) |
| --- | --- |
| TMP116, TMP116N | p. 11 Table 1, pp. 25–26 Tables 5–6, p. 28 Tables 8–9. [TI SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf). |
| TMP117, TMP117M, TMP117N | p. 13 Table 7-1, pp. 26–27 Tables 7-5/7-6, p. 29 Tables 7-8/7-9. [TI SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf). |
| TMP119 | p. 15 Table 7-1, pp. 33–34 Tables 8-4/8-5, p. 36 Tables 8-7/8-8. [TI SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf). |

The exact upper endpoint is 32767/128 = 255.9921875 °C. The example tables abbreviate the decimal. The word range also covers factory THIGH at +192 °C and TLOW at −256 °C. Use the selected grade's specified conditions for accuracy claims. At startup TEMP holds `0x8000` until the first completed conversion, including the averaging block when enabled. Offset saturation can also yield that code on supporting models. A TEMP read acknowledges DATA_READY and preserves alert-mode HIGH/LOW. TI leaves the precise clear edge and byte latching during an overlapping update unresolved. [Silicon evidence](../prompts/SILICON.md), [board and grade conditions](../board-integration.md).

Both read-only sibling worktrees were clean. At TMP1x2 `c91bac6`, the selected sample declaration and measurement implementation matched the pin `afd5088`. Reuse the pattern of decoding into a local sample before publishing and keeping timestamp validity independent of freshness. Its shifted 12/13-bit format, 1/16 °C scale and one-shot inference are device-specific. At ADS1115 `1aab07b`, the signed test source matched `e07622a`. Its raw `0xFFFE` → −2 result/cache check and separate endpoint tests are applicable test patterns. Gain scaling and ADC clipping flags belong to that converter. [TMP1x2 sample](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/include/TMP1x2/TMP1x2.h), [TMP1x2 measurement](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2Measurement.cpp), [ADS1115 signed tests](https://github.com/janhavelka/ADS1115/blob/e07622a99fe9f13c5d6a5a2497a8112a14185b9d/test/test_basic.cpp).

### Phase b acceptance cases

| Case | Required result |
| --- | --- |
| Signed TEMP and limits | Share one pure conversion for TEMP (`0x00`), THIGH (`0x02`) and TLOW (`0x03`). Decode via a wide signed intermediate, subtracting 65536 when bit 15 is set, then divide the signed counts by 128.0. Check the literal words `0x0C80` = +25, `0xF380` = −25, `0xFF80` = −1, `0xFFFF` = −0.0078125, `0x8000` = −256 and `0x7FFF` = +255.9921875 °C. Exercise both directions, zero and negative fractions. |
| Celsius to wire | Accept finite inputs in the exact inclusive interval. Round scaled values to nearest count, with ties away from zero. Check the rounded range before integer conversion. Test both sides of ±1/256 °C, the exact ties (−1/+1 count), and −6.50390625 °C → `0xFCBF`. Reject NaN, both infinities and values immediately outside either endpoint with `INVALID_PARAM`, preserving the caller's output. |
| Latest read and provenance | Use the agreed `readLatest(Sample&)` entry point. Route the scalar `readTemperature` form through the same observation and publish Celsius only on success. Public admission requires binding and initialization. Prove local refusal before initialization with zero callbacks, including after a compatible probe. Exercise success at the private engine until stage 18. In either desired mode, including dirty trust, issue just one tracked TEMP word read. Preserve the raw word, trust and generation at observation, unknown offset, `ReadKind::LATEST` and `freshConversion=false`, including on a repeated word or startup sentinel. |
| Time and cache | Untimed latest uses the per-callback cap and supports a missing clock. A valid completion tick of zero, including after wrap, is distinguishable from an absent timestamp. Publish sample and cache copies only after timely success. A short transfer or NACK preserves both and the scalar output. Exercise both possible readiness outcomes of a failed TEMP read. Preserve historical copies and their original provenance across an incompatible ID. |
| Timed owner boundary | At the private seam, seed a prior good sample. With a 12 ms budget from `0xFFFFFFF8` and a pre-read tick of 1, grant at most 3 ms. Completion at 4 or later gives `OPERATION_TIMEOUT`. On physical success retain the complete raw observation, completion time and consumed readiness. On a short transfer retain the physical failure and possible readiness loss. In both cases preserve the public output and cache. Expiry before the call costs zero callbacks. [Owner deadline and wrap trace](../owner-operations.md). |

Register the new native tests in the existing CMake suite, document the public API and update the affected coverage rows R01, R12, R16 and F17. Stage 17 consumes the codec for settings validation. Stage 18 supplies real managed initialization, 21 owns fresh acquisition, 22 the limit operations, 23 hardware offset provenance, 27 the complete cache inspection and age API, and 30 the cooperative read. [API contract](../api-contracts.md), [coverage owners](../prompts/FULL_COVERAGE.md), [test entrypoint](../testing.md).

### Phase a checks

Windows, GNU C++ 15.1.0, CMake/CTest 4.0.1, Python 3.12.10 and Git 2.52.0.windows.1.

| Check | Result |
| --- | --- |
| `cmake -S . -B build-tests-prompt16-a -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build-tests-prompt16-a --config Release --parallel 4` | PASS, fresh Release baseline. |
| `ctest --test-dir build-tests-prompt16-a -C Release --output-on-failure` | PASS, all 11 prerequisite tests, including the register ledger and its regressions. |
| Python cross-check of the two JSON vector sets with signed big-endian `int.from_bytes`, exact decimal division and `ROUND_HALF_UP` | PASS, 11 signed word rows and 14 inverse rows matched the independent calculations. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages. |

Phase b can start on the verified inputs. The codec and latest-read acceptance tests are pending implementation. MCU links, physical evidence and the stage 43 remote CI checkpoint retain their later owners.

## Phase b implementation, 2026-11-22

**PARTIAL.** The signed codec and latest-read engine are implemented. The clean baseline was `ef11a6cc96526bac06295f9fd59020cea791ade1` on `main`, equal to local `origin/main` and the wrapper's logged starting revision. Its parent `74fe24b` matched the phase-a request's subject and sole path. The wrapper verified that commit upstream at 19:15:11 Prague time.

The phase-a TI pages were rechecked against the pinned PDFs for signed units, the startup word, TEMP read effects and the THIGH/TLOW format. The two sibling checkouts were clean. TMP1x2 advanced to `0791429`, with the cited sample header and measurement source unchanged from `afd5088`. The ADS1115 signed-test source at `1aab07b` still matched `e07622a`.

| Produced artifact | Implementation and next consumer |
| --- | --- |
| `temperature-codec` | `decodeCounts` uses a signed 32-bit intermediate. `toCelsius` applies the 1/128 °C scale. `encodeCelsius` checks the input and the rounded count before narrowing. Stage 17 can use the codec for profile validation. [Public header](../../include/TMP11x/Temperature.h), [encoder](../../src/Temperature.cpp). |
| `latest-read-engine` | Both public entry points check the real lifecycle through `readLatest`. The private engine issues one tracked TEMP read, uses its completion tick for expiry and timestamp validity, and publishes only a timely complete sample. Its TEMP-specific result retains the physical outcome and any complete word. Stage 18 supplies verified initialization. Stages 19 and 21 consume the read effects and extend acquisition. Stage 27 owns cache inspection and age. [Public API](../../include/TMP11x/TMP11x.h), [lifecycle](../../src/TMP11x.cpp), [engine](../../src/LatestRead.cpp), [transport](../../src/RegisterTransport.cpp). |

| Native cases | Evidence |
| --- | --- |
| `signedRegisterVectors`, `nearestCountTiesAwayFromZero`, `rejectBeforeRounding`, `everyWordRoundTrips` | All 11 literal signed words and all 65,536 round trips passed. Inverse cases included half-count ties of both signs, negative fractions, NaN, infinities and the nearest doubles outside the endpoints. [Codec tests](../../test/test_temperature.cpp). |
| `publicGuards`, `clocklessLatest` | Public calls preserved sample/scalar outputs before binding and before initialization, including after a compatible probe. The private engine read one TEMP in either desired mode for all six models. Repeated `0x8000` retained its numeric value, unknown offset and unproven freshness. Clockless timed admission preserved the cache. [Latest tests](../../test/test_latest.cpp). |
| `failedReadsAndLifecycle` | NACK and one-byte failures preserved output and cache. The fixture exercised both possible readiness outcomes of a failed TEMP read. Rejected bind and complete ID mismatch retained the historical sample. Accepted bind, end and unbind cleared it. |
| `wrappedDeadline` | A 12 ms budget from `0xFFFFFFF8` granted 3 ms at tick 1. Completion at tick 4 or later returned `OPERATION_TIMEOUT` and preserved output and cache. Late success retained the word and physical success tick, and late short retained the fault without a complete word. Pre-call expiry spent zero callbacks. A timely completion at tick zero produced a valid timestamp. |

Windows verification used GNU C++ 15.1.0, CMake/CTest 4.0.1, Python 3.12.10 and Git 2.52.0.windows.1.

| Local check | Result |
| --- | --- |
| `cmake -S . -B build/host -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build/host --config Release --parallel 4` | PASS, strict C++17 build. |
| `ctest --test-dir build/host -C Release -R "^(latest\|temperature)_cpp$" --output-on-failure`, then repeat without the filter | PASS, 2/2 focused and 13/13 overall, including the register ledger and regressions. |
| `cmake -S examples/native_consumer -B build/consumer -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`, then `cmake --build build/consumer --config Release --parallel 2` and `./build/consumer/tmp11x_native_consumer.exe` | PASS, tests-off consumer linked and printed `0.1.0`. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py` | PASS, 14 PDF pins and 55 stages. |

Successful managed acquisition and scalar transport failures through the public API require stage 18 initialization. Phase c reviews these artifacts against the private-engine and public-admission boundaries. MCU links, remote CI and physical qualification retain their scheduled owners.
