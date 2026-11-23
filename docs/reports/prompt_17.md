# Prompt 17 — Type-safe CONFIG codecs and validation

**PARTIAL — phase a, 2026-11-23.** All seven prerequisites (5, 6, 7, 8, 11, 12 and 16) are complete. Phase b can implement `config-codec` as pure C++17 word conversion, profile validation and expected-register comparison. The next consumer is audit 47, followed by stage 18 for bind validation and guarded physical apply. [Prompt 17](../prompts/17_config-codecs.md), [sequence](../prompts/SEQUENCE.json), [handoffs](../prompts/HANDOFFS.md).

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
