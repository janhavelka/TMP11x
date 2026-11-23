# Prompt 17 — Type-safe CONFIG codecs and validation

**COMPLETE, 2026-11-23.** The pure C++17 `config-codec` passed native acceptance for all six BOM selections. All 3,072 steady profiles encoded to their expected register words. Readback comparisons accepted the continuous alias and required a confirmed owned trigger to accept one-shot for desired shutdown. [Phase d acceptance](#phase-d-acceptance), [prompt 17](../prompts/17_config-codecs.md).

The entry worktree was clean. Both `main` and its local tracking ref `origin/main` pointed to `6d9259519b9add1473647f4d4bad2380f0dab3f4`. Its parent was `763339c1b156d3da0cde76717ae1f7ca5fe976a3`. The subject and three documentation paths matched the stage 16 closure request. The wrapper recorded upstream equality at 21:16:49 Prague time on November 22.

### Verified inputs

| Producer and artifact | Evidence for phase b |
| --- | --- |
| 5 · `register-ledger` | Each model's CONFIG entry at pointer `0x01` lists field encodings, field-level EEPROM eligibility, status effects and release write policy. [Register ledger](../registers/tmp11x.json). |
| 6 · `behavior-contract` and `behavior-vectors` | The eight `pin_routes` rows cover routing and polarity. The `alert-window-ack-and-rearm`, `therm-hysteresis-and-shutdown` and `one-shot-refusals-and-provenance` traces distinguish observations, flag acknowledgement and trigger ownership. [Behavior](../device-behavior.md), [vectors](../registers/behavior-vectors.json). |
| 7 · `safety-policy` and 8 · API prerequisite | CONFIG BUSY is an observation. Lock proof requires reading EUN in `0x04`. The API distinguishes the desired profile, a complete CONFIG observation and later verified trust. [Safety policy](../eeprom-and-reset-policy.md), [API contract](../api-contracts.md), [architecture](../architecture.md). |
| 11 · `value-types` | Reuse `SensorSettings`, its six enum validators, signed `Limits` and the existing model lookup. [Config.h](../../include/TMP11x/Config.h), [model descriptors](../../src/ModelDescriptor.h). |
| 12 · `command-table` | Positioned field masks, `stableConfigMask` and per-model reserved bits already exist. The transport rejects a stable write containing any bit outside `0x07FC`. [CommandTable.h](../../include/TMP11x/CommandTable.h), [transport guard](../../src/RegisterTransport.cpp). |
| 16 · `temperature-codec` | THIGH and TLOW use exact signed counts in units of 1/128 °C. Reuse the signed codec for register interpretation and compare each limit across all 16 bits. [Temperature.h](../../include/TMP11x/Temperature.h), [signed-word tests](../../test/test_temperature.cpp). |

### TI CONFIG tables checked

| BOM selection | One-based PDF pages | CONFIG bits 1:0 |
| --- | --- | --- |
| TMP116, TMP116N | Figure 32 and Tables 6–7, pp. 26–27. [SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26). | Both reserved. |
| TMP117, TMP117M, TMP117N | Figure 7-14 and Tables 7-6/7-7, pp. 27–28. [SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27). | Bit 1 triggers addressed reset on write-one and reads zero. Bit 0 is reserved. |
| TMP119 | Figure 8-7 and Tables 8-5/8-6, pp. 34–35. [SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34). | Same reset and reserved-bit meanings as TMP117. |

Across the three tables, HIGH, LOW, DATA_READY and EEPROM_BUSY occupy `0x8000`, `0x4000`, `0x2000` and `0x1000`. The masks for MOD, CONV, AVG, T/nA, POL and DR/Alert are `0x0C00`, `0x0380`, `0x0060`, `0x0010`, `0x0008` and `0x0004`. POL=0 selects active low and POL=1 active high. Build ordinary words from validated fields with MOD=00 for continuous or 01 for shutdown. This leaves `0xF803` clear. For reset fixtures, MOD0 and bits 9:2 can load from EEPROM. MOD1 has no stored value. The factory example `0x0220` supplies one test vector. Live settings and calibration require separate evidence.

### Phase b acceptance cases

| Case | Expected evidence |
| --- | --- |
| Encode and decode | Exercise 512 steady profiles per BOM selection (2 modes × 8 cycles × 4 averages × 8 alert/polarity/routing combinations). Check independent single-bit literals and a nonfactory reset fixture, e.g. `0x05DC` = shutdown, cycle 3, average 32, therm, active high, DATA_READY pin. Reject invalid typed selectors, including desired MOD=2 or 3, before publishing an encoded word. Include synthetic forbidden low bits and verify that no status, reserved or reset bit enters an ordinary write. |
| Observe read effects | Preserve the complete raw word, raw MOD and all four status bits. Interpret flags using the observed T/nA. In alert mode a CONFIG read acknowledges HIGH/LOW. In therm mode HIGH is a hysteresis level and LOW reads zero. Both modes consume DATA_READY. For `0xB620`, retain HIGH, DATA_READY and BUSY even though the stable fields match `0x0620`. |
| Compare MOD before masking | `0x0A20` (raw MOD=10) matches desired continuous `0x0220`. For desired shutdown `0x0620`, observed `0x0E20` (MOD=11) matches only with retained owned-trigger evidence. `0x0640` still fails on AVG. A simple mask of both words by `0x07FC` would wrongly accept an unowned one-shot. Check the other stable fields in every mode. |
| Track the provenance boundary in comparison inputs | Physical trigger success, even after deadline, supplies provenance. MOD=01/11 then remains compatible with desired shutdown through polls, completion, cancellation and timeout. Successful explicit SD or invalidation ends the exception. Failed or ambiguous triggers supply no ownership. Ambiguous SD invalidates trust. MOD alone proves neither completion nor a fresh baseline. The acquisition owner must establish guarded SD and a baseline for each new managed shot. [Engineering contract](../prompts/ENGINEERING.md). |
| Validate limits | Exercise signed endpoints, negatives, equal limits and inverted pairs. THERM requires low ≤ high, including when switching modes. ALERT allows independent thresholds. THIGH is `0x02` and TLOW is `0x03`. Changes to either belong to their own full-word comparison. |

Place the pure `encodeConfiguration`, `decodeConfiguration`, comparison and validation beside the existing types and register masks. Register focused literal-vector tests in the [host suite](../../CMakeLists.txt) and update public Doxygen, [test documentation](../testing.md) and [feature coverage](../feature-coverage.md) with the implemented behavior. Stage 19 owns live observation retention and trust, stage 21 owns trigger sequencing, and stage 23 adds opt-in managed offset.

### Sibling comparison

Both read-only worktrees were clean. The cited source files at TMP1x2 local head `0791429` and ADS1115 local head `1aab07b` matched their pinned versions. TMP1x2's `encodeConfiguration` validates before publishing, and `packConfiguration` assembles typed fields. Its extended-mode and resolution layout is chip-specific. ADS1115's `_buildConfigRegisterFor` starts at zero and `kConfigReadbackMask` selects comparison fields. TMP11x additionally needs the raw MOD and owned-trigger input during comparison. [TMP1x2 pinned source](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2Configuration.cpp), [ADS1115 pinned source](https://github.com/janhavelka/ADS1115/blob/e07622a99fe9f13c5d6a5a2497a8112a14185b9d/src/ADS1115.cpp).

### Phase a checks

| Windows command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4`, then `ctest --test-dir build/host -C Release --output-on-failure` | PASS, 13/13 prerequisite tests in the existing Release build, GNU C++ 15.1.0 and CMake/CTest 4.0.1. |
| `python scripts/check_references.py` and `python tools/check_register_ledger.py` | PASS, 14 pinned PDFs and all six model maps. Python 3.12.10. |
| `python scripts/check_promptset.py` | PASS, 55 stages and their artifact dependencies. |

Phase b has all required inputs. TI leaves MOD readback during and after one-shot, the precise flag-clear edge, and the effect of a same-value CONFIG write on an active conversion unresolved. The comparison cases above exercise the software policy. Stage 17 implementation and acceptance remain open.

## Phase b implementation

Work started on clean `main` at `16f600c48028b0891a5f868ae694b7102dd5a087`.
Git history and the wrapper log agree that this commit contained the phase-a
report and matched `origin/main` at 09:03:04 Prague time on November 23.
The three pinned CONFIG tables, field-level persistence and sibling source
files were rechecked at the revisions above.

`validateSettings` checks the exact model, every steady selector and the THERM
limit order. `encodeConfiguration` writes its output only after validation.
`decodeConfiguration` preserves the whole word, raw MOD, all four status flags
and the read-clear mask selected by the observed T/nA bit. The pure decoder
leaves the completion timestamp invalid. [Public interface](../../include/TMP11x/Configuration.h),
[implementation](../../src/Configuration.cpp).

`compareExpectedRegister` checks MOD=11 ownership before applying the stable
mask. For desired shutdown, a confirmed trigger permits MOD=01 or 11 while
bits 9:2 still have to match. For continuous, MOD=00 or 10 can match.
THIGH and TLOW use the stage 16 signed count decoder across all 16 bits.
Stage 18 supplies physical apply, stage 19 retains observations and trust,
and stage 21 owns trigger provenance and fresh-conversion sequencing.
[API guide](../api-contracts.md#config-word-conversion).

| Native case | Evidence |
| --- | --- |
| `allSteadySettings` | All 512 stable field combinations on each of six exact models encoded to independent literals, decoded and compared. Every ordinary word left `0xF803` clear. |
| `statusAndReadEffects`, `nonfactoryResetProfile` | Alert and therm reads retained the raw flags with read-clear masks `0xE000` and `0x2000`. Nonfactory `0x05DC` differed from the default target. Synthetic reserved and command bits survived in the raw observation and stayed clear in the encoded target. |
| `normalizedModeComparison` | All four MOD encodings, both steady targets and both ownership states. A single changed bit at each position 9:2 still reported drift, including during a confirmed one-shot. |
| `rejectSelector`, `signedLimitComparisons` | Every out-of-domain byte value for the six selectors preserved the encoder output. Both limit pointers detected individual changes to all 16 bits. Signed endpoints, equal limits and reversed ALERT/THERM limits exercised profile validation. |

The cases run in Release with independent assertions. [Test source](../../test/test_configuration.cpp),
[CTest registration](../../CMakeLists.txt), [testing guide](../testing.md).

| Windows command | Phase b result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` | PASS, strict C++17, GNU C++ 15.1.0 and CMake 4.0.1. |
| `ctest --test-dir build/host -C Release -R "^configuration_cpp$" --output-on-failure`, then the same command without the filter | PASS, 1/1 focused and 14/14 overall, including register-ledger validation and regressions. CTest 4.0.1. |
| `cmake --build build/consumer --config Release --parallel 4` and `build/consumer/tmp11x_native_consumer.exe` | PASS, tests-off native source consumer printed `0.1.0`. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and clean diff whitespace. Python 3.12.10. |

MCU links, remote CI and physical testing were not run. Phase c owns the fresh
review and any confirmed corrections. Phase d owns final acceptance.

## Phase c review

The clean entry revision was `ae5894a20b6493078749281bca243880e3bfaa1e`.
Its 11 changed paths matched the phase-b request. The wrapper recorded
upstream equality at 09:19:16 Prague time on November 23. Two parallel
read-only reviews covered the TI tables and the public API and tests.
The lead checked both findings against the source and reproduced the test gap.

- **Status flags during profile comparison.** The existing tests decoded LOW
  but every matching CONFIG comparison had LOW clear. A temporary copy of
  the comparator that included LOW in its stable mask passed the old CONFIG test.
  `statusAndReadEffects` now compares the desired profile against each of
  HIGH, LOW, DATA_READY and BUSY asserted individually, in ALERT and THERM,
  for all six BOM selections. The added assertion rejected the faulty copy
  on the ALERT word `0x4620`. The production comparator already accepted
  that word for a matching shutdown profile. [Focused test](../../test/test_configuration.cpp).
- **EEPROM reload scope.** The API guide now specifies power-on and
  general-call reset. The general-call descriptions restore power-up values
  (TMP116 p.21, TMP117 p.22, TMP119 p.24). The addressed Soft_Reset entries
  leave EEPROM reload unspecified (TMP117 p.28, TMP119 p.35). The nonfactory
  fixture exercises power-on reset. [API guide](../api-contracts.md#config-word-conversion),
  [reset policy](../eeprom-and-reset-policy.md#addressed-reset-and-recovery).

The original CONFIG tables confirmed polarity, read-clear masks and the
ordinary write mask. The MOD comparison checks trigger ownership before
dropping MOD1. Both limit comparisons cover all 16 bits through the signed
codec. The 3,072 steady profiles, invalid selectors and transport refusal
vectors exercise the field boundaries. The reviewed sibling files still
matched the pins recorded above. Physical apply, trust retention and trigger
sequencing remain with stages 18, 19 and 21 respectively.

| Windows check | Phase c result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` and `ctest --test-dir build/host -C Release -R "^configuration_cpp$" --output-on-failure`, then CTest without the filter | PASS, 1/1 focused and 14/14 overall, including the register ledger and its regressions. GNU C++ 15.1.0, CMake/CTest 4.0.1. |
| `g++ -std=c++17 -O2 -DNDEBUG -Iinclude test/test_configuration.cpp build/p17-review/include-low.cpp src/Config.cpp -o build/p17-review/include-low.exe`, then run the executable before and after the test edit | The faulty comparison passed the old test (exit 0) and failed the new assertion (exit 1). The temporary source added `command::lowFlag` to the comparison mask. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and clean diff whitespace. Python 3.12.10. |

## Phase d acceptance

Acceptance ran on clean `main` at `8c39261a388835853176bb52c583c8fa8a363e50`.
The phase-c commit and its three paths matched the wrapper request. The
wrapper recorded synchronization with `origin/main` at 09:32:18 Prague time
on November 23. This revision includes both phase-c corrections.

The three TI CONFIG tables cited above agree on POL=0 active low and
POL=1 active high. Each steady encoding left `0xF803` clear. The transport
test refused each forbidden bit without a bus callback and accepted
`0x07FC`. The comparator ignored each status flag independently in ALERT
and THERM mode and detected drift in bits 9:2 even with an owned trigger.
[Codec cases](../../test/test_configuration.cpp), [transport cases](../../test/test_register_transport.cpp).

| Windows acceptance command | Result |
| --- | --- |
| `cmake -S . -B build/host -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build/host --config Release --parallel 4` | PASS with CMake 4.0.1 and GNU C++ 15.1.0. |
| `ctest --test-dir build/host -C Release -R "^configuration_cpp$" --output-on-failure`, then repeat without the filter | PASS, 1/1 focused and 14/14 overall with CTest 4.0.1, including the register ledger and its regressions. |
| `cmake -S examples/native_consumer -B build/consumer -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`, then `cmake --build build/consumer --config Release --clean-first --parallel 4` | PASS, tests-off source consumer rebuilt and linked. |
| `build/consumer/tmp11x_native_consumer.exe` and `ctest --test-dir build/consumer -C Release -N` | PASS, linked version `0.1.0` and zero registered tests. |
| `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude -fsyntax-only -x c++ -` on each public header and README C++ example | PASS, seven headers in isolation and two examples. |
| `doxygen -` over `include/TMP11x` with XML output and `WARN_AS_ERROR=YES` | PASS with Doxygen 1.13.2. The XML documents the validator, encoder, decoder, comparison and raw MOD accessor. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` and `git diff --cached --check` | PASS, 14 pinned PDFs, 55 stages and clean whitespace. Python 3.12.10 and Git 2.52.0.windows.1. |

Audit 47 consumes the public codec and its native tests. Stage 18 owns
guarded physical apply, stage 19 owns retained observations and trust, and
stage 21 supplies trigger provenance during acquisition. MOD readback during
and after one-shot remains undocumented. MCU links, remote CI and physical
qualification are NOT RUN for stage 17. [Handoffs](../prompts/HANDOFFS.md).
