# 42 — Finish packaged consumers, public Doxygen and user documentation

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Package, CI and final review**. Type: **software**.
Prerequisites: [33](33_native-suite.md), [34](34_arduino-examples.md), [35](35_idf-examples.md), [38](38_cli-device-operations.md), [39](39_python-hil-runner.md).

## Objective and boundaries

Make TMP11x a clean independently consumable MCU/native library and document its actual verified feature set without bundling testing/firmware prerequisites into installed core.

## Source audit

Recheck TMP1x2 `CMakeLists.txt`, `library.json`, `idf_component.yml`, version generation, `docs/testing.md`, `tools/check_doxygen.py`, and DP8382x relocated installation/consumer checks. Inspect actual build outputs, current API and feature coverage.

## Complete implementation work

- Audit and complete `library.json`, CMake config/install/export, native `add_subdirectory`, ESP-IDF component metadata, Arduino `lib_deps` from GitHub and generated committed `Version.h` with single version source.
- Create isolated source-consumer, archive/extracted consumer and relocated CMake-installed `find_package` consumer tests in directories that cannot see sibling repos, hidden generated outputs, Git, Python or SDK unless the SDK path is the explicit test.
- Ensure source/header license/attribution, archive include/exclude, no vendor/GPL code, no reference PDFs, no test fixtures or CLI auto-linked into portable core. `#include <TMP11x/TMP11x.h>` must work without a generator execution.
- Add strict meaningful public Doxygen, user/architecture/variant/config/freshness/EEPROM/CLI/HIL/IDF/Arduino integration, troubleshooting and test documentation. Separate documented/implemented/native-tested/MCU-built/physical-verified capability rows.
- Prove a sample application can simultaneously use TMP1x2 and TMP11x from independent dependencies, and demonstrate each typed model without requiring a sibling at build time.

## Verification and exit criteria

Run clean install/extract and relocation consumers, core tests ON/OFF, Doxygen check, version freshness, package inventory/notice validation and full firmware links where available; record command and exact evidence.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run unrelated sibling suites or remote CI (except stage 43).

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_42.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
