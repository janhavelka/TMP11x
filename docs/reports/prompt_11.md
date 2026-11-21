# Prompt 11 — portable Status, model and value types

## Phase a brief, 2026-11-21

**PARTIAL.** The inputs and implementation boundaries are verified. Stage 11
owns portable C++17 values and pure validation. Phase b delivers the types and
their native tests. [Stage prompt](../prompts/11_status-values.md),
[sequence](../prompts/SEQUENCE.json), [handoffs](../prompts/HANDOFFS.md).

Entry was clean `main` at `1014fed870fd2d8ca8d958790a75547172256138`,
with 0/0 divergence from the local `origin/main` for `janhavelka/TMP11x`.
The commit's parent `a3994f417a0f61c1c5ce689c96767be31ff40df0`, subject and
three changed paths match `commit_20261121-225703.json`. The wrapper's
`runs.log` records that commit and upstream equality at 23:00:43 Prague time.
Stage 10 closes the last prerequisite. Stage 11 is the first unfinished
entry in manifest order. Its report was absent at entry.
[Progress](../prompts/PROGRESS.md), [stage 10 acceptance](prompt_10.md).

| Consumed artifact | Verified input at the baseline |
| --- | --- |
| `api-contract` (8) | Binding/settings, value fields, error semantics and stage ownership are present in `docs/api-contracts.md` and `docs/architecture.md`. Reviewed the audit corrections through `2ba9c775fea013498fd05bdcd6a464b69b47033e`. [API](../api-contracts.md), [architecture](../architecture.md), [stage 8](prompt_8.md), [audit 46](prompt_46.md). |
| `owner-contract` (9) | `docs/owner-operations.md` defines immutable binding fields, optional clock for observations, the timebase and positive interval limits, and the separation between acquisition and profile evidence. Job admission and result lifetime remain with their later owners. [Owner contract](../owner-operations.md), [stage 9](prompt_9.md). |
| `build-foundation` (10) | Root `CMakeLists.txt` builds `src/Version.cpp` with `include/TMP11x/Version.h`, exposes C++17 and shares the runtime source list with the IDF branch. `test/test_version.cpp` and five Python checks are registered with opt-in CTest. There is no checked-in workflow at this baseline. [CMake](../../CMakeLists.txt), [host suite](../testing.md), [stage 10](prompt_10.md), [CI checkpoint policy](../prompts/COMMON.md). |
| `variant-contract` (4) and `register-ledger` (5) | `docs/variants.md` defines the six BOM selections, compatibility, address range and shadow indices. The three profiles in `docs/registers/tmp11x.json` provide the per-model mask, layout and CONFIG fields. The behavior contract supplies context for cycle/AVG and startup TEMP. [Variants](../variants.md), [ledger](../registers/tmp11x.json), [behavior](../device-behavior.md), [safety policy](../eeprom-and-reset-policy.md). |

### Source checks

The lead and two read-only reviewers checked the TMP1x2 precedents and the
exact-part TI data. The clean local sibling was at
`c91bac63313ea725ce174ab98abc50705c365639`. The five selected blobs match
the stage 1 pin `afd50889312f6e218a6a34eca687e3e8547d8b40`.
[Pinned TMP1x2 tree](https://github.com/janhavelka/TMP1x2/tree/afd50889312f6e218a6a34eca687e3e8547d8b40),
[source inventory](../library-comparison.md).

| TMP1x2 path at the pin | Decision for TMP11x |
| --- | --- |
| `include/TMP1x2/Status.h` | Use the scoped error and typed status shape, constexpr predicates, symbolic names and borrowed static text. Preserve signed detail for adapter diagnostics. Define TMP11x's own error set. Peer CLI prints symbolic error names and the signed detail. The shared-bus example maps the controller outcome into each package's status independently. Package-local model ordinals appear in peer examples and tests. There is no shared enum ordinal ABI in the inspected paths. |
| `include/TMP1x2/Config.h` | Use the callback/context and strongly typed setting patterns. TMP11x uses the documented seven-bit range, a positive operation budget, an optional clock with resolution and a `Status`-returning GPIO reader. Its cycle selector represents CONV codes and combines with AVG for timing. Peer address maps, independent conversion rates, extended mode, fault queue, pin ownership and yield hook do not apply. |
| `include/TMP1x2/TMP1x2.h` | Use the passive state vocabulary and separate raw/counts/time/freshness fields. TMP11x uses 1/128 °C units, `double` Celsius and the frozen acquisition, generation, trust and offset evidence. Health transitions and operation token semantics have their own owners. |
| `test/helpers/test_helpers.cpp`, `LICENSE` | Use Release-active checks, invalid selector cases and separate time/freshness assertions. The MIT notice blob `c2240ead2e094fb7de9622cc25c3ba68f4d05acf` matches this repository. Phase b records actual adapted material in the source-adoption guide and preserves its notice. |

| Pinned TI datasheet | ADD0 / map / TEMP / CONFIG and timing / ID pages |
| --- | --- |
| [TMP116/N · SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=20) | 20 / 24 / 25 / 26–27 / 31 |
| [TMP117/M/N · SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=21) | 21 / 25 / 26 / 27–28 / 32 |
| [TMP119 · SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23) | 23 / 32 / 33 / 34–35 / 39 |

The ADD0 mapping is GND, V+, SDA, SCL to unshifted `0x48`, `0x49`, `0x4A`,
`0x4B`. TMP116/N requires full-word `0x1116`, carries no decoded revision,
has shadows at `05/06/07/08` hex and has neither hardware offset nor
supported addressed reset. The three TMP117 grades and TMP119 share
mask `0x0FFF`, value `0x0117` and upper-nibble revision, shadows at
`05/06/08` hex, offset at `0x07` and addressed reset at CONFIG bit 1.
The shared ID leaves TMP117 versus TMP119 and exact grade unresolved.
TMP119's map says `0x0117` and its detailed ID table says `0x2117`.
TMP117's ID figure leaves revision unspecified and its table shows zero.
The reviewed shadow maps establish three words on TMP117/TMP119 despite
the four-word overview. The detailed CONFIG field table establishes
addressed reset for those parts despite the register drawing's `R-0` label.

CONV has eight codes in TI table order and AVG has four codes for 1, 8,
32 and 64 conversions. Code 0 has an AVG-dependent period. The desired
settings carry continuous or shutdown. Raw MOD `10` aliases continuous and
raw MOD `11` is an explicit one-shot command. TEMP is signed 16-bit data
with a scale of 1/128 °C and a register range of −256 to +255.9921875 °C.
`0x8000` is also the startup word, so its value alone supplies no validity
or freshness evidence. Existing read-effect, timing and reset uncertainties
remain in the ledger for the operation owners.
[Variant interpretation](../variants.md), [silicon evidence](../prompts/SILICON.md).

### Phase-b implementation and acceptance brief

| Planned owner and paths | Work and focused acceptance cases |
| --- | --- |
| `include/TMP11x/Status.h` | Define `Err`, `Status`, `errorName` and passive `DriverState`. Distinguish terminal success, operation progress, adapter causes and logical refusals. Test each symbolic name, invalid enum formatting, both predicates, signed detail and preservation of the static message pointer. Include generic and phase-specific NACK, short-transfer reporting and transport timeout distinct from operation deadline expiry. |
| `include/TMP11x/Config.h` and a single shared model table | Define six exact model selections, public capability values and pure model/address/setting checks. Keep family, ID mask/value and revision layout plus shadow indices/count and offset/reset gates in the same table so stages 12, 13 and 24 can consume it. Reject invalid selections explicitly and preserve an output on failure. Test all six models against independent literal expectations and test unknown models and `0x47`, `0x48`–`0x4B`, `0x4C`, `0x90` and `0xFF`. Exercise every CONV and AVG code and out-of-range casts. |
| `include/TMP11x/Config.h` settings and binding values | Match the frozen callback signatures, copied `SensorSettings`, stable mode/cycle/AVG, alert mode/polarity/pin function and both `int16_t` limit counts. Verify defaults for cycle 4, AVG 8, continuous, alert, active-low threshold, limits −32768/24576, null callbacks, 1 ms clock resolution, 50/1000 ms budgets and five failures. The frozen contract calls for explicit model/address selection. Use unselected defaults so supplying callbacks alone cannot select a device. Document borrowed contexts, terminal bounded callbacks, setting units and invalid-selector behavior. Stage 23 adds offset control and corresponding profile fields. |
| `include/TMP11x/TMP11x.h` value definitions | Use the frozen `Sample` fields, including `double` Celsius, explicit timestamp validity, acquisition kind, generation, configuration trust, freshness and default unknown offset provenance. Define separate `IdentityInfo` with selected BOM/address, complete raw word validity, compatible ID group and revision validity. Check default unknown/invalid evidence, type precision, exact count units and the independence of a sample's numeric value from timestamp, trust and freshness. Stage 13 populates identity and stage 16 applies the signed codec. |
| `test/`, `CMakeLists.txt`, `docs/testing.md` and affected guides | Register focused tests under `TMP11X_BUILD_TESTS`. Compile each public header in its own translation unit under strict C++17 warnings with no SDK or sibling dependency. Exercise the value operations with no heap allocation and always-active Release checks. Update public Doxygen and the guides affected by the new surface. Add any out-of-line source to both CMake build paths through their common source list. |

Stage 12 brings transport and framing, stage 13 decodes and matches ID from
the shared table, stage 14 binds and exposes identification, and stage 15
adds tracked health. Stages 16–18 own signed register codecs, CONFIG
encoding and complete managed-profile validation and application. Stage 23
populates offset provenance. Phase c reviews the implemented artifacts.
Phase d owns final acceptance and the completion ledger and index entries.

### Phase-a checks

- PASS `cmake -S . -B build-tests-prompt11-a -G Ninja -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`,
  `cmake --build build-tests-prompt11-a --config Release --parallel` and
  `ctest --test-dir build-tests-prompt11-a -C Release --output-on-failure`.
  CMake 4.0.1, Ninja 1.12.1, GNU C++ 15.1.0 and Python 3.12.10 passed all
  six baseline checks, including the six-model ledger and its 31 regressions.
- PASS `python scripts/check_references.py` for 14 pinned PDFs and direct
  review of the relevant TI pages. PASS `python scripts/check_promptset.py`
  for 55 stages and contract links.
- New value-type tests, MCU links, remote CI and physical qualification
  were NOT RUN. No blocker prevents phase b. The wrapper owns the report
  commit and upstream synchronization.

## Phase b implementation, 2026-11-21

Phase b started from clean `main` at
`ba2e51e4d9c73e9aa3292e709997422c909a3c9f`, matching the local `origin/main`
at 0/0 divergence. The parent, subject and one changed file match the
phase-a commit request. `runs.log` records upstream equality at 23:15:42
Prague time. The phase-b changes await the wrapper commit and push.

| Owned surface | Delivered behavior |
| --- | --- |
| [Status and passive state](../../include/TMP11x/Status.h) | `Status` preserves signed diagnostic detail and borrows static text. Symbolic names distinguish pending work, driver refusals, generic and phase-specific NACK, short transfer, bus fault and transport timeout. Whole-operation deadline expiry has a separate code. |
| [Binding and stable settings](../../include/TMP11x/Config.h) | `Config{}` starts with `Model::UNSELECTED` and address zero. `SensorSettings` copies the stable selectors and two signed limit words. Pure predicates validate all selector values and seven-bit addresses. Callback signatures preserve borrowed buffers, contexts, terminal status and explicit budgets. Full profile validation belongs to stages 17–18. |
| [Model descriptor](../../src/ModelDescriptor.h), [lookup](../../src/Config.cpp) | One private table maps all six BOM choices to two access and identity profiles. Each row carries the ID group/mask/value, revision mask/shift, shadow pointers and count, and the offset/reset gates. `getCapabilities` preserves output and returns `INVALID_PARAM` for invalid model values. Stage 12 uses the selected descriptor for access policy. Stage 13 can scan the same table to recognize the raw word independently of the BOM selection before evaluating compatibility. Stage 24 uses the shadow map. |
| [Identity and sample evidence](../../include/TMP11x/TMP11x.h) | `IdentityInfo` has independent raw and revision validity. A zero revision can be valid. `Sample` preserves `double` Celsius, exact signed counts and independent time, acquisition, profile, freshness and offset evidence. The default timestamp is invalid and offset evidence is unknown. Stage 16 populates the numeric values, and stage 23 supplies offset evidence and the managed offset setting. |
| [Build and tests](../../CMakeLists.txt), [test guide](../testing.md) | Both CMake paths include `src/Config.cpp`. The host suite registers `value_types_cpp` with strict C++17 and independent header compilations. PlatformIO advertises the main header alongside the version header. [Source adaptations and MIT attribution](../source-adoption.md#portable-value-types). |

One agent implemented the three test translation units. A separate read-only
agent rechecked the exact pinned TI pages for addresses, maps, TEMP, CONFIG,
limits and ID. The code uses the phase-a ID, EEPROM and reset decisions.
CONV codes 0–3 have AVG-dependent nominal periods. The related code-0 example
in phase a remains valid. The CONFIG software defaults encode `0x0220` and
the limit defaults correspond to `0x8000` and `0x6000`. A default value or a
compatible ID alone supplies no live configuration or exact grade evidence.
The TMP119 ID table discrepancy and reset uncertainties remain recorded in
the silicon evidence. Both agents finished before handoff.

### Phase-b checks

| Command or case | Evidence |
| --- | --- |
| `cmake -S . -B build-tests-prompt11-b -G Ninja -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, `cmake --build build-tests-prompt11-b --config Release --parallel`, `ctest --test-dir build-tests-prompt11-b -C Release --output-on-failure` | PASS, 7/7 registrations with CMake 4.0.1, Ninja 1.12.1, GNU C++ 15.1.0 and Python 3.12.10. The value test and new core source compiled with `-std=c++17 -Wall -Wextra -Wpedantic -Werror` and `-DNDEBUG`. The ledger and its 31 regression cases passed. |
| `statusNamesAndPredicates`, `modelCapabilitiesAndIdentityPolicy`, `addressesAndSelectors`, `defaultAndCopiedEvidence` in [test_value_types.cpp](../../test/test_value_types.cpp) | PASS. Cases cover all symbolic errors, invalid enum formatting, signed detail and static pointer preservation, all 256 model/address/selector encodings, the two literal ID/shadow profiles, unchanged output on capability failure, default binding/settings and independent sample evidence. The runtime guard traps ordinary C++ `new` and `new[]` during checks. `nm -u` on `Config.cpp.obj` reported no undefined symbols. [Config header check](../../test/config_header.cpp), [Status header check](../../test/status_header.cpp). |
| `doxygen build-tests-prompt11-b/Doxyfile.values` | PASS with Doxygen 1.13.2. The local XML build of the three new public headers passed with warnings treated as errors. |
| `scripts/pio.cmd package pack --output build-package-prompt11-b/TMP11x-final.tar.gz` | PASS with managed Core 6.2.0 and the Git-blob-verified offline SPDX response recorded in [stage 10](prompt_10.md#phase-b-checks). All 15 exported files matched the checked source bytes. Archive SHA-256 `ad302e8660e92eb14b7138f69f5feeed21276bed9ab9c012cb259a3d076d7862`. |
| Tests-off consumer from the extracted export under `build-package-prompt11-b/linked` | PASS. The separate app included `<TMP11x/TMP11x.h>`, linked `getCapabilities` and verified TMP119 capability values. Python and Git package discovery were disabled. CMake reported the discovery flags unused and CTest listed zero tests. |
| `python -B scripts/check_promptset.py`, `python -B scripts/check_references.py`, `git diff --check` | PASS for 55 stages, 14 pinned PDF hashes and whitespace. The initial coverage evidence row was relabelled after the checker interpreted its compound first cell as a requirement ID. |

MCU links, deliberate remote CI and physical qualification were NOT RUN.
Stages 34–36 own the first adapter links, 40–41 own fixture evidence, and
43 owns the first deliberate remote checkpoint. Phase c reviews the source,
tests and checked artifact revision. Phase d retains final acceptance and
completion ownership.
