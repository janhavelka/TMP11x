# Prompt 13 — identity validation

## Phase a, 2026-11-22

**PARTIAL.** All inputs for the private `identity-engine` are available.
The stage 12 closure commit, `29a325773f39dc113087cda6d8f6631227ecb0a0`,
was the clean entry revision on `main`. Its parent was `624db94`, its
subject and four documentation paths matched the previous commit request,
and the wrapper recorded equality with `origin/main` at 15:14:45 Prague
time. Stage 13 follows stage 12 in the execution sequence. Phase b owns the
engine and its first executable tests. [Prompt](../prompts/13_identity-selection.md),
[sequence](../prompts/SEQUENCE.json), [ownership](../prompts/HANDOFFS.md).

### Verified inputs

| Producer | Artifact at the entry revision | Use |
| --- | --- | --- |
| 4 | `variant-contract` in [variants.md](../variants.md) | Six explicit BOM selections, two recognizable groups and the limits of a family match. |
| 5 | `register-ledger` in [tmp11x.json](../registers/tmp11x.json) | Three silicon profiles assign read-only DEVICE_ID to `0x0F`. The open `tmp119-id-default` record retains both reset words. |
| 8 | [API contract](../api-contracts.md) and [architecture](../architecture.md) | A complete incompatible ID is the explicit ID-read exception to output preservation. A short or failed read supplies no new observation. |
| 11 | `value-types` in [TMP11x.h](../../include/TMP11x/TMP11x.h), [Config.h](../../include/TMP11x/Config.h), [ModelDescriptor.h](../../src/ModelDescriptor.h) and [Config.cpp](../../src/Config.cpp) | Reuse `IdentityInfo` and the two descriptor rows. Recognize the observed group independently, compare it with the selected model's group, and keep capability lookup tied to that selection. |
| 12 | `register-transport` in [RegisterTransport.cpp](../../src/RegisterTransport.cpp) and `register-fake` in [FakeTmp11xDevice.h](../../test/support/FakeTmp11xDevice.h) and [its implementation](../../test/support/FakeTmp11xDevice.cpp) | `detail::readWord` takes a borrowed `Config`, pointer and remaining budget. It returns the physical status and attempted flag. The fake can seed the ID, deliver a scripted short prefix and capture every callback on a shared bus. |

### Silicon and comparison sources

| Selected BOM model | Complete-word rule and supporting TI tables |
| --- | --- |
| TMP116, TMP116N | All 16 DID bits must equal `0x1116`. Revision is unavailable. [SBOS740A p.24 Table 3](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24), [p.31 Table 15](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=31). |
| TMP117, TMP117M, TMP117N | DID[11:0] equals `0x117`. REV[15:12] is observed separately, including zero in the documented `0x0117`. [SNOSD82D p.25 Table 7-3](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25), [p.32 Table 7-15](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=32). |
| TMP119 | The register map gives `0x0117`. The detailed ID section gives `0x2117`. Both encode DID `0x117`, so either word matches all four TMP117-like selections. Exact part and accuracy grade require BOM evidence. [SNIS236 p.32 Table 8-2](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32), [p.39 Table 8-14](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=39). |

The ordinary TMP114 and TMP118 negative fixtures at `0x0F` use zero,
inferred from each non-indexed-read rule and register map. TMP114 with CRC
disabled is a fixture choice. Its real ID is `0x1114` at `0x0B`. TMP118
places `0x1180` there with revision in the **low** nibble. TMP118 UID access
requires shutdown and data writes. TMP114 describes bad-CRC NACK/FFh
behavior, leaving the exact omitted-CRC response open. Script a transport
failure and a complete `0xFFFF` as separate adverse cases.
[SNIS214E pp.24, 26, 29, 38](../reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=24),
[SNIS237A pp.18–19, 23, 26, 29](../reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=23),
[exclusion audit](../reference/extracted/excluded-families-audit.md).

At OPT4001 revision `f17d4944640145773d3331e155a5bc728fcc4c66`,
`probe` and its native tests distinguish callback errors from a complete
mismatch, perturb individual ID bits and check callback counts. The selected
header, implementation and test file also match the project's `441ef579`
comparison pin. Reuse the test method. OPT4001's pointer, ID layout and
tracked `readDeviceId` have different contracts. [Probe implementation](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/src/OPT4001.cpp#L400),
[native probe tests](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/test/test_basic.cpp#L817).

Linux IIO at `23c62304dde29abe92013d85910d03a77dada35d` lists TMP116,
TMP117 and TMP119 in both device-match tables. Its probe reads `0x0F`,
propagates bus errors, switches on three full-word constants and falls back
to device match data for other words. For TMP11x, a different 117-family
revision is admissible and a wrong DID ends validation, even on a configured
route. The GPL-2.0-only source serves as a factual comparison.
[Pinned driver](https://github.com/torvalds/linux/blob/23c62304dde29abe92013d85910d03a77dada35d/drivers/iio/temperature/tmp117.c),
[blob and license record](../reference/public-code-catalog.md).

### Phase b acceptance cases

Implement the private engine under `src/` with `command::deviceId` and the
checked word read. Register the focused fixture tests in the existing host
suite and document their invocation. Every admitted case should show exactly
one combined callback at the selected address, TX `[0F]`, requested RX
length two and the clipped timeout. A mismatch ends the attempt at this
frame, before any retry, scan, data write or foreign UID procedure.
[Host test entrypoint](../../CMakeLists.txt), [testing guide](../testing.md).

| Case | Expected evidence |
| --- | --- |
| Full-word TMP116 | Both selections accept `0x1116`, group TMP116, revision invalid. `0x0116` and `0x2116` are unknown complete IDs. |
| Shared TMP117-like DID | Each of the four selections accepts `0x0117`, `0x2117` and `0xA117` with valid revisions 0, 2 and 10. Preserve the exact selected model and address in each result. |
| Swapped groups | TMP116 selected with `0x2117` publishes the raw word, TMP117-like group and revision 2. TMP119 selected with `0x1116` publishes TMP116 group and invalid revision. Both report `IDENTITY_MISMATCH`, raw valid and compatible false after a successful physical read. |
| Shared-bus neighbors | Select only the intended address. Named ordinary TMP114 and TMP118 fixtures return a complete zero and yield an unknown-group mismatch. Exercise the two adverse CRC fixtures above. An unrelated responder scripted with `0x2117` yields only family compatibility for a TMP117-like selection. |
| Transfer failure | Address NACK, timeout and adapter-reported short (including one delivered byte) preserve a prefilled `IdentityInfo` and propagate the transport error and detail. A complete `0xFFFF` produces mismatch evidence. |
| Local refusal | Unsupported model or address, including shifted `0x90`, fails with `INVALID_CONFIG`. A valid binding with zero remaining budget returns `OPERATION_TIMEOUT`. Each preserves output and costs zero callbacks. |

Stage 14 exposes bound, clockless `readIdentity` and `probe`. Stage 15
checks that diagnostics leave health counters untouched on all outcomes.
Managed initialization at stage 18 needs the successful physical read
separately from its logical mismatch. Discovery and route selection belong
to the application. In phase c an independent reviewer should challenge the
`0x2117` cases against both TMP119 tables and check that the engine never
promotes an observed revision into an exact part. [Coverage R17, F01, F19 and X07](../prompts/FULL_COVERAGE.md).

### Entry checks

Windows, GNU C++ 15.1.0, CMake/CTest 4.0.1, GNU Make 4.4.1 and Python
3.12.10. The checks below cover the prerequisites at the entry revision.

- PASS — `cmake -S . -B build-tests-prompt13-a -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release` and `cmake --build build-tests-prompt13-a --config Release --parallel`.
- PASS — `ctest --test-dir build-tests-prompt13-a -C Release --output-on-failure`, 8/8 including value types, word transport and both register-ledger checks.
- PASS — `python scripts/check_references.py` verified all 14 pinned PDFs. `python scripts/check_promptset.py` verified the 55-stage sequence and handoffs.
- PASS — All 27 local report links resolve. `git diff --check` and a whitespace check of the new report found no issues.

The identity implementation and its native results remain open for phase b.
Physical qualification and MCU links have later owners. The first remote CI
checkpoint is stage 43.

## Phase b implementation, 2026-11-22

**PARTIAL.** The private `identity-engine` is implemented and host-tested.
The entry revision was `a9f0541823cd240faa2c16ff3020b9bc2b6cf38e` on
clean `main`. Its parent, subject and single report path match the phase-a
commit request. The wrapper recorded equality with `origin/main` at 15:31:12
Prague time.

A single checked read of `command::deviceId` supplies the raw word. The
engine matches it against the two existing descriptor rows and compares the
recognized group with the selected model's group. On a complete mismatch it
publishes the observation and includes the raw word in `Status::detail`.
On a local refusal or failed callback, it preserves the caller's entire
`IdentityInfo`. The returned `TransferResult` retains the physical status
and attempted flag for the operation owner. [Private interface](../../src/Identity.h),
[implementation](../../src/Identity.cpp), [descriptor table](../../src/ModelDescriptor.h).

| Registered native case in `identity_cpp` | Evidence |
| --- | --- |
| `selectedModelsAndIdBits` | All six selections read `0x1116` and all 16 revision values with DID `0x117`. Incompatible recognized words retain their observed group and revision validity. Single-bit changes exercise every significant ID bit. `0x0116` and `0x2116` yield unknown-group mismatches. |
| `sharedBusNeighbors` | Four addresses on one bus return the two ordinary TMP114/TMP118 negative words, a coincidental `0x2117` and an intended `0x0117`. Separate adverse TMP114 cases exercise a CRC-related NACK and a complete `0xFFFF`. Each attempt produces exactly one combined `[0F]` read requesting two bytes at the selected address. |
| `failedReadsAndLocalRefusals` | Address NACK, timeout and shorts delivering zero or one byte preserve prior evidence and adapter detail. Invalid selections, out-of-range addresses including `0x90`, and an expired budget preserve output and produce zero callbacks. Admitted reads exercise both a 7 ms remaining budget and the 19 ms callback cap. |

The [focused tests](../../test/test_identity.cpp) use the stage 12 register
fixture with independent literals and Release-active checks. The pinned TI
tables cited in phase a still give both `0x0117` and `0x2117` for TMP119.
In the shared-bus fixture an unrelated responder returns `0x2117` while
the selected model remains TMP117. This is executable evidence of the
limit of ID-based identification. [Variant guide](../variants.md#identity-decision).

### Executed checks

Windows, GNU C++ 15.1.0, CMake/CTest 4.0.1, GNU Make 4.4.1 and Python 3.12.10.

- PASS — `cmake -S . -B build-tests-prompt13-b -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release` and `cmake --build build-tests-prompt13-b --config Release --parallel`.
- PASS — `ctest --test-dir build-tests-prompt13-b -C Release -R "^identity_cpp$" --output-on-failure` (1/1) and the same command without the name filter (9/9, including the register-ledger validator and its regressions).
- PASS — `cmake -S examples/native_consumer -B build-consumer-prompt13-b -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`, `cmake --build build-consumer-prompt13-b --config Release --parallel` and `./build-consumer-prompt13-b/tmp11x_native_consumer.exe` (printed `0.1.0`).
- PASS — `python scripts/check_references.py` verified 14 pinned PDFs. `python scripts/check_promptset.py` checked 55 stages. `git diff --check` and whitespace checks of the new files passed.

Phase c owns fresh review of the actual engine and tests, including the
independent challenge of both TMP119 source tables. Stage 14 connects the
private engine to bound diagnostics. Stage 15 verifies health accounting.
MCU links, remote CI and physical qualification remain with their later stages.
