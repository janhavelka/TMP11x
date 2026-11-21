# Prompt 10 — independent buildable library foundation

## Phase a brief, 2026-11-21

**PARTIAL.** Requirements and inputs verified. Phase b owns implementation.
[Stage 10 prompt](../prompts/10_package-foundation.md),
[handoffs](../prompts/HANDOFFS.md), [verification](../prompts/VERIFICATION.md).

Entry was clean `main` at `cac1b93ae8dbde50afe854e4a04c620bcc16a8ac`,
with 0/0 divergence from `origin/main` in `janhavelka/TMP11x`. The parent
`2ba9c775fea013498fd05bdcd6a464b69b47033e`, subject and three changed
paths match `commit_20261121-215703.json`. The wrapper logged the commit
and verified equality with upstream at 22:01:55 Prague time. Stages 1–9,
45 and 46 are complete in execution order through 46. Stage 10 follows 46
and precedes 11. No prior stage 10 report, build sources, package metadata,
host entrypoint or local workflow exists at this baseline.
[Sequence](../prompts/SEQUENCE.json), [progress](../prompts/PROGRESS.md).

| Consumed artifact | Verified input |
| --- | --- |
| `tooling-plan` (2) | The plan records selected tooling, package boundaries, peer pins and prior PlatformIO launcher correction. [Plan](../toolchain-and-package-plan.md), [report](prompt_2.md). |
| `api-contract` (8) | The published API and architecture reserve `Version.h` for stage 10 and establish the C++17 dependency boundary and later production ownership. [API](../api-contracts.md), [architecture](../architecture.md), [report](prompt_8.md). |
| `owner-contract` (9) | The owner contract defines transport ownership and later admission, timing and result behavior. Stage 10 introduces only a version API, so these rules carry forward to the sensor implementation. [Owner operations](../owner-operations.md), [report](prompt_9.md). |
| `register-validator` (5) | The validator and regression runner exist and run independently with the register ledger. Stage 10 registers the existing scripts with CTest. [Validator](../../tools/check_register_ledger.py), [regressions](../../tools/test_check_register_ledger.py), [ledger](../registers/tmp11x.json), [report](prompt_5.md). |
| `audit-8-9` (46) | The committed acceptance includes the independent reviews, comparator ordering, unmanaged-offset invalidation, passive job inspection and retained late read evidence. Silicon uncertainties remain assigned to their feature owners. [Audit](prompt_46.md#phase-d-acceptance-2026-11-21). |

### Peer sources

Both local sibling checkouts were clean. The selected files match their
pinned blobs at the recorded local HEADs. Both pinned MIT `LICENSE` notices
match the existing TMP11x notice for THYMOS Solution s.r.o. and Jan Havelka.
Record the exact file and pin for each adaptation in the stage report and
source-adoption guide when the implementation is written.

| Source and verified local revision | Adoptable files and limits |
| --- | --- |
| TMP1x2 pin `afd50889312f6e218a6a34eca687e3e8547d8b40`, local `c91bac63313ea725ce174ab98abc50705c365639` | `CMakeLists.txt` supplies the native and IDF branches, public C++17 usage requirement, opt-in CTest and test-only Python discovery. `tools/check_package.py` runs a renamed-source `add_subdirectory` consumer with tests off by default. `test/core_no_arduino/compile_main.cpp` is the separate tests-on smoke. Adapt `scripts/generate_version.py`'s render, sync and check pattern and `include/TMP1x2/Version.h`'s documented version representation. Give TMP11x its own metadata and paths. `examples/esp_idf/basic/{CMakeLists.txt,main/CMakeLists.txt}` derive the checkout component name for `REQUIRES`, useful to stage 35. [Pinned tree](https://github.com/janhavelka/TMP1x2/tree/afd50889312f6e218a6a34eca687e3e8547d8b40). |
| OPT4001 pin `441ef579d7fa3f7bca7665f52632fdaddd306300`, local `f17d4944640145773d3331e155a5bc728fcc4c66` | `tools/check_clean_consumer_package.py` isolates the packed library from the checkout. Root `CMakeLists.txt` is IDF-only, and the IDF example hardcodes `REQUIRES OPT4001`. `library.json`, `idf_component.yml`, `scripts/generate_version.py` and `include/OPT4001/Version.h` are packaging references. Use TMP1x2 for the dual-mode CMake basis and preserve TMP11x's existing override-aware `scripts/pio.cmd`. [Pinned tree](https://github.com/janhavelka/OPT4001/tree/441ef579d7fa3f7bca7665f52632fdaddd306300), [launcher provenance](../source-adoption.md). |
| ESP32 PlatformIO template pin `b574041c686f44821aa8828675f480576e5eff55` | The pinned `AGENTS.md` confirms public headers, implementation and examples layout. The root CMake lookup returned 404. The placeholder manifest and MIT attribution, Arduino lifecycle template and Git/time version generator are unsuitable to port. [Pinned tree](https://github.com/janhavelka/esp32-platformio-library-template/tree/b574041c686f44821aa8828675f480576e5eff55). |

### Implementation and acceptance brief

| Stage 10 output | Phase-b work and acceptance checks |
| --- | --- |
| `build-foundation` | Create the C++17 CMake library with a small out-of-line version function in `src/`, using the checked-in generated public `Version.h`. A separately configured tests-OFF consumer under `examples/` should include the header, link and run the function. Configure with `TMP11X_BUILD_TESTS=OFF` and Python and Git package discovery disabled. Check the public header in isolation and use a clean consumer/source path independent of sibling trees and SDKs. Stage 11 still owns the sensor value types. Add `library.json`, `idf_component.yml` and the development changelog at `0.1.0`, with project provenance and an explicit runtime export list. Inspect an actual `scripts/pio.cmd package pack` archive listing for version, license, manifests, headers and source. Keep test tools, local builds, prompts, reports and vendor reference PDFs out of runtime exports. |
| `version-tools` | Put deterministic render, sync and nonmutating check in `scripts/generate_version.py`. Use `library.json` as authority and validate any checked-in version mirrors, including the IDF manifest. `tools/check_version_sync.py` delegates to the same check and propagates its failure. Verify that sync is reproducible and both checks fail on deliberate drift without changing the scratch copy. Leave Git and wall-clock metadata out of the generated version; later firmware work owns build identity. |
| `host-test-entrypoint` | Gate CTest and Python interpreter discovery behind `TMP11X_BUILD_TESTS`, default OFF. Register the generator check, thin checker, existing ledger validator and regression runner, plus real C++ checks for the generated constants and linked version value. Run a fresh Release configure/build/CTest to show assertions remain effective. Document the actual commands, dependencies and test registration in `docs/testing.md`. Stages 37–39 extend the same entrypoint. |

Phase b should update the affected guides and public Doxygen with the
implemented surface. Stage 35 owns the linked ESP-IDF app. Stage 42 owns
package checker and Doxygen tooling, extracted and relocated install package
qualification. Stage 43 owns the initial workflow and first deliberate CI
checkpoint. Phase c reviews the actual artifacts and fixes confirmed gaps.
Phase d owns final acceptance, indexing and completion.

### Phase-a checks

- Python 3.12.10 passed `python -B scripts/check_promptset.py` (55 stages),
  `python -B scripts/check_references.py` (14 pinned PDFs),
  `python -B tools/check_register_ledger.py` (six BOM models), and
  `python -B tools/test_check_register_ledger.py` (31 regressions).
- Git 2.52.0.windows.1 verified the stage 46 commit against the wrapper
  request and runs log. The working and staged diff whitespace checks and
  the new report's whitespace and local links were checked.
- CMake and CTest 4.0.1, GCC 15.1.0 and GNU Make 4.4.1 are available for
  `MinGW Makefiles`. `scripts/pio.cmd --version` selected Core 6.2.0 at
  `C:\pio\penv\Scripts\pio.exe`. `scripts/pio.cmd package pack --help`
  confirmed the output argument.

There is no phase-b input blocker. Native library builds, tests-OFF consumer,
version checks, package listing, MCU links, remote CI and physical
qualification were NOT RUN. The wrapper owns the phase-a commit and upstream
verification.
