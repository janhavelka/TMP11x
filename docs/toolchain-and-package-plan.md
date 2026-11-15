# Toolchain, package and prompt workflow plan

Audited 2026-11-15 during repository preparation. These are selected build
patterns and an installed-tool inventory, not TMP11x firmware or hardware
qualification. The repository had no driver, build definition or CI workflow
at the start of this audit (`6c5e4496d5c896af689228d4bee4382a53c20626`).

## Source revisions and access

The following public remote HEADs were checked with `git ls-remote`. Local
repositories were read only; no sibling fetch, build, checkout or edit ran.
The local HEAD matched the public HEAD for every locally available row.

| Source | Audited commit | Relevant evidence |
|---|---|---|
| [TMP1x2](https://github.com/janhavelka/TMP1x2/tree/afd50889312f6e218a6a34eca687e3e8547d8b40) | `afd50889312f6e218a6a34eca687e3e8547d8b40` | `AGENTS.md`, `CLAUDE.md`, `CMakeLists.txt`, `library.json`, `idf_component.yml`, `platformio.ini`, `scripts/pio.cmd`, `scripts/generate_version.py`, `.github/workflows/ci.yml`, examples, registered tests and package checks |
| [OPT4001](https://github.com/janhavelka/OPT4001/tree/441ef579d7fa3f7bca7665f52632fdaddd306300) | `441ef579d7fa3f7bca7665f52632fdaddd306300` | Agent entry, IDF-only root CMake and `tools/check_clean_consumer_package.py` |
| [24Cxx](https://github.com/janhavelka/24Cxx/tree/1bd9f969d1df55ca5819b0d290ff331869e7d5bc) | `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | Agent entry, dual native/IDF CMake, CLI host fixture and Python transcript tests |
| [MotorControl-CAN](https://github.com/janhavelka/MotorControl-CAN/tree/60bcefe5f2bd29429b8019361374e1776a2b5972) | `60bcefe5f2bd29429b8019361374e1776a2b5972` | `docs/prompts/{START_HERE,COMMON,PROGRESS}.md`, `scripts/check_promptset.py` and `SEQUENCE.json` structure |
| [DP8382x](https://github.com/janhavelka/DP8382x/tree/87aafc01c959d5f811fb7342a037e26c105acac4) | `87aafc01c959d5f811fb7342a037e26c105acac4` | `AGENTS.md`, `CLAUDE.md`, `docs/prompts/{COMMON,EVIDENCE,VERIFICATION}.md` |
| [esp32-platformio-library-template](https://github.com/janhavelka/esp32-platformio-library-template/tree/b574041c686f44821aa8828675f480576e5eff55) | `b574041c686f44821aa8828675f480576e5eff55` | GitHub connector: `AGENTS.md`, `library.json`, workflow, version generator, `include/YourLibrary/YourLib.h` and `LICENSE`; root `CMakeLists.txt` returns 404 |

TMP1x2, OPT4001 and 24Cxx were clean. MotorControl-CAN contained concurrent
bench edits, including prompt 47; DP8382x contained concurrent implementation
edits. The selected shared prompt contracts were unchanged in those dirty
trees. Their live changes and historical bench authorizations are not TMP11x
inputs. The older hashes in the initial SOURCE_MAP are historical observations,
not the revisions audited here.

TMP1x2, OPT4001 and 24Cxx carry MIT with the 2026 THYMOS Solution s.r.o./Jan
Havelka notice. Preserve that notice when adapting substantial source. The
template has an MIT notice with placeholder author text; prefer the maintained
sensor library as the source for actual ports. Vendor PDFs retain their own
notices and are separate from MIT library code.

## Minimal build and package layout

Stage 10 should establish `include/TMP11x/`, `src/`, `test/`, `examples/`,
`scripts/`, `tools/`, `library.json`, `idf_component.yml`, `CMakeLists.txt`,
`LICENSE` and a generated checked-in `Version.h`. Stage 8 owns exact public
declarations. Preparation does not create pretend source files or build passes.

| Area | Selected behavior | Precedent and required adaptation |
|---|---|---|
| Native core | Standard C++17 library, public headers independently usable, tests OFF for consumers; CMake/CTest registers actual assertions | TMP1x2 root CMake; use TMP11x sources, no `BusOperations` general-call helper; keep assertions active for Release tests |
| PlatformIO native | Additional native host route with explicit source filters | TMP1x2 `env:native` and `native_core_no_arduino`; CTest remains the complete suite because the sibling native environment ignores several suites |
| Arduino ESP32-S2/S3 | Full linked examples with application-owned I2C and console | TMP1x2 `platformio.ini` and `examples/01_basic_bringup_cli`; recheck platform pin and board settings when introduced, keep board/pin policy out of core |
| Native ESP-IDF S2/S3 | Root CMake supports `idf_component_register`; examples declare actual I2C, GPIO, timer and console dependencies | TMP1x2 `examples/esp_idf/basic/{CMakeLists.txt,main/CMakeLists.txt}`; retain IDF 6 console-component distinction only for tested versions |
| ESP32-P4 | Optional additional full link and consumer gate, stage 36 | Installed SDK supports the target; adapter/board configuration and actual TMP11x build remain unverified |
| Shared CLI | One framework-neutral parser/output engine with thin Arduino/IDF transports and host fixture | TMP1x2 `examples/common/Tmp1x2Cli*`; 24Cxx CLI host/transcript pattern; rewrite device commands and read-to-clear semantics |
| Python HIL | Parser and finite campaign tests before physical runs; firmware/runner/source identities retained | TMP1x2 runner and CTest registration; no inherited fixtures, success logs or EEPROM-write commands |
| Version | `library.json` authority; generated checked-in header, deterministic check mode; clean consumers do not run Python or Git | Adapt TMP1x2 generator; its CMake project regex hardcodes `TMP1x2` and must change |
| Documentation | Public Doxygen and relevant guides accompany each public behavior | TMP1x2 Doxygen checker/test pattern, DP8382x evidence-layer and meaningful-coverage rules |
| Source package | Explicit inclusion list, safe extraction, renamed clean consumer | TMP1x2 `library.json`/`tools/check_package.py`, OPT4001 clean PlatformIO consumer |
| Installed CMake package | Relocatable exported target and `find_package` consumer, with build/install include interfaces | Required TMP11x work: audited TMP1x2 CMake has no install/export implementation to copy |
| CI | Native warnings/sanitizers, real documentation, package and linked SDK gates | Adapt commands to implemented targets; keep automatic triggers; stage 43 owns the first comprehensive remote checkpoint |

No sibling checkout, vendor PDF, reference-code archive, hardware transcript or
toolchain installation may be needed by an extracted or installed consumer.
Runtime exports must exclude `.git`, `.pio`, `build*`, managed components,
generated SDK configuration, logs, prompts, reports and reference payloads.
Metadata/version and required notices must survive extraction under another
directory name. Do not advertise broader framework support from metadata alone.

## File adoption decisions

| Source file or group | Decision | Reason / owning stage |
|---|---|---|
| TMP1x2 `scripts/pio.cmd` | Reuse unchanged when introduced | Resolves one existing managed Core and fails clearly when missing; usable in preparation |
| TMP1x2 `LICENSE` | Preserve on adopted source | The same maintained MIT source notice applies to copied substantial material |
| TMP1x2 `scripts/generate_version.py` | Adapt at stage 10 | CMake regex contains `project(TMP1x2 VERSION ...)`; regenerate for TMP11x metadata and test nonmutating check behavior |
| TMP1x2 root CMake, manifests, PlatformIO config | Adapt at stage 10 | Project names, exported sources, targets, version and board/package claims differ; add missing install/export behavior |
| TMP1x2 package and Doxygen checks | Adapt with concrete implemented files | Required filename lists and symbols are device-specific; extend package checks to relocated install consumers |
| TMP1x2 `.github/workflows/ci.yml` | Adapt at the prescribed CI stage | Commands reference sibling targets and SDK pins; workflow presence does not prove builds |
| TMP1x2 example CLI, IDF CMake and identity helpers | Adapt at stages 34–39 | Framework separation and archive directory independence are useful; device behavior and names must change |
| OPT4001 `tools/check_clean_consumer_package.py` | Adapt package-consumer design | Its package/filter/version assumptions are OPT4001-specific |
| MotorControl-CAN prompt checker | Adopt narrow structural checks only | Index dependencies and prompt prerequisite links must agree with JSON; avoid its unrelated CAN evidence framework |
| Template `AGENTS.md`, workflow, source skeleton | Reject wholesale copy | Mandatory `tick`, optional tasks/heap and retry guidance conflict with TMP11x; placeholder metadata and unpinned tooling are not a usable final scaffold |
| Sibling reference folders, reports, old HIL logs | Do not copy wholesale | Acquire exact TMP11x-relevant sources with provenance; old evidence qualifies neither this device nor this code |

## Installed tools on the preparation host

These observations come from commands and files inspected on 2026-11-15.
No installation or firmware build ran.

| Tool | Observation |
|---|---|
| Python | `python --version`: 3.12.10 |
| CMake | `cmake --version`: 4.0.1 |
| Native compiler | `g++ --version`: MinGW-W64 UCRT MCF GCC 15.1.0; available on PATH |
| Managed PlatformIO | `%USERPROFILE%\.platformio\penv\Scripts\pio.exe --version`: Core 6.1.19 |
| PATH PlatformIO | A different `Python312\Scripts\pio.exe` is on PATH; use the repository wrapper to select the managed Core |
| Arduino package cache | `framework-arduinoespressif32/package.json`: 3.2.0; cached package metadata does not prove the future pinned platform build |
| PlatformIO compiler cache | Xtensa and RISC-V package metadata: `14.2.0+20241119`; other versioned compiler directories also exist |
| IDF environment | `IDF_PATH` unset and `idf.py` absent from PATH in this shell; installed SDKs exist below and need their matching environment activated |
| Additional host compiler | `clang++` not found on PATH in this audit; second-compiler evidence NOT RUN |

| Installed SDK root | Exact source HEAD | Installed compiler directory |
|---|---|---|
| `C:/esp/v5.4.4/esp-idf` | `296b6eab9445fd720e71aecab961e2d3fbca9944` | Xtensa/RISC-V `esp-14.2.0_20260121` |
| `C:/esp/v5.5.5/esp-idf` | `b774170ff46c393eeb5e495ea37936038d3f4f4f` | Xtensa/RISC-V `esp-14.2.0_20260121` |
| `C:/esp/v6.0.1/esp-idf` | `8c19b156084a0753687347cca1f5355782893533` | Xtensa/RISC-V `esp-15.2.0_20251204` |
| `C:/esp/v6.1/esp-idf` | `fff9895c82d744c7237be8847347bdd1b07c6643` | Xtensa/RISC-V `esp-15.2.0_20251204` |

SDK versions were read from `tools/cmake/version.cmake`; each installation's
`tools/idf-env.json` lists ESP32-S2, ESP32-S3 and ESP32-P4. These are installed
target records, not completed link tests. No target is known missing from this
inventory; dependency completeness and example compatibility remain to be
verified. Prefer the installed current SDKs when selecting a test matrix, then
record the actual chosen versions. Do not treat TMP1x2's historical CI matrix
(`5.3.2`, `5.5.1`, `6.0.1`) as an automatically verified TMP11x support range.

## Promptset and agent-entry audit

The initial 44-stage JSON sequence, numbered files and explicit links passed
`python scripts/check_promptset.py`; the original five checker regression cases
passed `python scripts/test_check_promptset.py`. An independent comparison of
all 44 prerequisite lines and README dependency rows with `SEQUENCE.json` found
no mismatch. `git diff --check` passed before the preparation changes. These
checks establish planning consistency, not register accuracy or implementation.

Selected improvements for the lead's integration:

1. Keep AGENTS as the single instruction source and make CLAUDE import it with
   `@AGENTS.md`, as TMP1x2, OPT4001, 24Cxx and DP8382x do. Put the task-entry route
   in AGENTS so it cannot diverge between agents.
2. Explicitly distinguish preparation/prompt maintenance, one-stage execution
   and whole-set execution. Reading START_HERE is not an instruction to start
   the 44-stage driver implementation. Resume from concrete existing artifacts.
3. Preserve the current main-branch and single-lead Git rules. Evidence reports
   should identify their input/base SHA; record the resulting commit in a later
   progress update or handoff, avoiding a self-referential commit-hash edit loop.
4. Validate prerequisite links and README index rows against SEQUENCE, not just
   that linked files exist. Validate JSON top-level/item types and required
   metadata before dereferencing; return useful errors for malformed input.
5. Keep checker fixtures narrow. The original `shutil.copytree(ROOT, ...)` copied
   `.git` and every reference payload once per test; reference intake makes that
   wasteful. Copy only documentation/checker inputs and represent non-text link
   targets cheaply where the test only needs existence.
6. Make report status artifact-based and distinguish inherited evidence gates
   from owned work. Keep software, SDK, CI and physical evidence separate.
7. Refresh only relevant changed sibling seams after this audit; record unchanged
   revisions instead of re-reading entire libraries every prompt. Preserve
   immutable vendor payloads and pin any updated sources explicitly.

The lead owns changes to AGENTS, CLAUDE, shared contracts, checker, progress and
reports. The recommendations above describe the audit findings; final integrated
check results belong in the preparation report. Native TMP11x tests, MCU links,
package consumers, remote CI and physical HIL are **NOT RUN** at this preparation
checkpoint because their implementations or fixtures are not established.
