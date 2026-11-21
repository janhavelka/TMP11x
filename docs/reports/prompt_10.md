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

## Phase b implementation and focused checks, 2026-11-21

**PARTIAL.** Implemented and checked the library foundation. Phase c owns the
fresh review. Phase d owns acceptance, progress and report indexing.

Phase-b entry was clean `main` at
`43ffbbb0f5710614265395bd8328bb64066dc12b`, with 0/0 divergence from
`origin/main`. The parent, subject and sole report path agree with
`commit_20261121-221203.json`. The wrapper logged the commit and verified
upstream equality at 22:15:09 Prague time. There were no unexpected worktree
changes. No local CI workflow exists. Git metadata and sync are wrapper-owned.

| Produced artifact | Implementation and handoff |
| --- | --- |
| `build-foundation` | [CMakeLists.txt](../../CMakeLists.txt) builds a static `TMP11x` target for native consumers and registers the same `src/Version.cpp` with ESP-IDF. Both expose the public include directory and C++17. [Version.h](../../include/TMP11x/Version.h) provides the header constants and the declaration of the out-of-line `TMP11x::version()`. The return value is the version compiled into the library. [Native consumer](../../examples/native_consumer/CMakeLists.txt) uses the library as a separate subdirectory with tests off. [PlatformIO](../../library.json) and [ESP-IDF](../../idf_component.yml) source exports carry the runtime and native example files, and the [changelog](../../CHANGELOG.md) identifies 0.1.0 as development. Stage 11 owns sensor value types. |
| `version-tools` | [Generator](../../scripts/generate_version.py) provides explicit `sync` and read-only `check`. The [thin checker](../../tools/check_version_sync.py) delegates to the same check via the current Python interpreter. `library.json` is authoritative. It supplies the three numeric constants, header string and manifest mirror. The generated header contains no Git or clock data. CMake reads the checked-in header through the source and has no separate version field. |
| `host-test-entrypoint` | `TMP11X_BUILD_TESTS` defaults OFF. When ON, CMake discovers Python 3.10+ and registers `version_cpp`, `version_generator_check`, `version_sync`, `register_ledger` and `register_ledger_regressions`. The [C++ test](../../test/test_version.cpp) checks the compiled-in and header values with explicit exit codes in Release. [Testing guide](../testing.md) documents both build paths and the version and archive checks. Stages 37–39 add CLI and runner cases to the same entrypoint. |

The adopted TMP1x2 files at `afd50889312f6e218a6a34eca687e3e8547d8b40`
are `CMakeLists.txt`, `library.json`, `idf_component.yml`,
`scripts/generate_version.py` and `include/TMP1x2/Version.h`. The tests-off
consumer follows the renamed source pattern in `tools/check_package.py`.
`test/core_no_arduino/compile_main.cpp` supplies the framework-free test
precedent. The local TMP1x2 and OPT4001 HEADs still matched the clean
phase-a observations. Both pinned MIT notices matched the preserved root
license. OPT4001's `tools/check_clean_consumer_package.py` at
`441ef579d7fa3f7bca7665f52632fdaddd306300` was a packaging precedent.
[Source adoption and exact pins](../source-adoption.md#library-foundation).

### Phase-b checks

- **PASS, native Release.** CMake and CTest 4.0.1, GCC 15.1.0, GNU Make
  4.4.1 and Python 3.12.10. Configured
  `cmake -S . -B build-tests-phase-b -G "MinGW Makefiles" -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=11 -DCMAKE_CXX_EXTENSIONS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`,
  built with `cmake --build build-tests-phase-b --parallel`, and ran
  `ctest --test-dir build-tests-phase-b --output-on-failure`. All five cases
  passed, including validation of six BOM models and the 31 ledger
  regressions. Compile commands showed `-DNDEBUG -std=c++17` for the library
  and test. A scratch `g++ -std=c++17 -O3 -DNDEBUG -Wall -Wextra -Werror`
  build of each negative case returned failure for a wrong linked version
  and for a numeric constant inconsistent with the header string.
- **PASS, isolated native source consumer and header.** Copied only the root
  CMake file, `include/` and `src/` to
  `build-consumer-phase-b/renamed library` and copied the two example files
  to `build-consumer-phase-b/isolated application`. Configured that
  application in `build-consumer-phase-b/linked` with MinGW Makefiles,
  `TMP11X_SOURCE_DIR` pointing to the renamed copy, `TMP11X_BUILD_TESTS=OFF`,
  `CMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE` and
  `CMAKE_DISABLE_FIND_PACKAGE_Git=TRUE`. Release build and execution passed
  and printed `0.1.0`. CMake left the disable flags unused, CTest listed zero
  tests and compile commands showed both copied sources using `-std=c++17`.
  Piped a translation unit containing only `#include <TMP11x/Version.h>`
  to `g++ -std=c++17 -pedantic-errors -Wall -Wextra -Werror -I <copied-include> -x c++ -fsyntax-only -`.
- **PASS, version synchronization.** Ran the three commands from the testing
  guide and `python -B build-version-phase-b/verify_generator.py` using the
  ignored scratch harness. Missing-header sync reproduced the checked-in LF
  bytes. Both check commands rejected independent header and IDF drift
  without changing bytes, timestamps or inventory. Sync repaired both,
  preserved the IDF comment and CRLF in the scratch file, and skipped writes
  when current. Changing the sole authority to `2.65535.3` updated both
  mirrors. Six invalid authority values and missing, duplicate or malformed
  IDF version fields failed without writes.
- **PASS with offline SPDX data, PlatformIO archive.** The direct command
  `scripts/pio.cmd package pack . --output build-package-phase-b` selected
  Core 6.2.0 at `C:\pio\penv\Scripts\pio.exe`. Its stock HTTPS fetch of
  SPDX v3.28.0 failed with socket WinError 10013. The same launcher and pack
  command passed using a process-scoped `PYTHONPATH` pointing at the local
  scratch `sitecustomize.py`. That hook supplied the official SPDX reply to
  the one pinned URL and left Core's manifest and license validation active.
  The payload from the official GitHub repository matched Git blob
  `4b7c3d5537191f08dc9771cb0ec5f59fe9c3f9ff` and SHA-256
  `f728c534d8bd1044fc515a2ddb2292be99559021d830bfa3281be0bcd36302ee`.
  A deliberately invalid license failed under the same replay and produced
  no archive. [Pinned SPDX payload](https://raw.githubusercontent.com/spdx/license-list-data/v3.28.0/json/licenses.json).
- **PASS, archive listing and IDF metadata.** `tar -tf build-package-phase-b/TMP11x-0.1.0.tar.gz`
  listed exactly ten regular files. These were the root CMake file, the two
  manifests, LICENSE, README, CHANGELOG, the two native consumer files,
  Version.h and Version.cpp. All entries matched the working-tree bytes.
  The archive SHA-256 was
  `f0b2a758c36d46fc6896a23bca1359bd59633494804b8e222db166eacaddd657`.
  ESP-IDF 5.5.5's Component Manager 2.5.2 parsed the 0.1.0 manifest and
  selected the same ten files. The archive, replay hook and native build
  outputs are in ignored build folders.
- **PASS, structural and source checks.** `python -B scripts/check_promptset.py`
  passed all 55 stages after adjusting the evidence row to the checker’s
  existing form. `python -B scripts/check_references.py` passed all 14 PDF
  hashes. Working and staged Git whitespace checks and local links in the
  edited guides and report passed. The staged diff was empty in-session.

MCU links, the installed CMake package and physical fixture qualification
were **NOT RUN**. Stages 34–36 own the link cases, stage 42 owns the installed
and extracted package tests, and stages 40–41 own fixture evidence. Remote CI
was **NOT RUN** per the stage 43 checkpoint policy. The phase-b result is
pending the wrapper commit and upstream check.
