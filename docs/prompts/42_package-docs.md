# 42 — Finish packaged consumers, public Doxygen and user documentation

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Package, CI and final review**. Type: **software**.
Prerequisites: [33](33_native-suite.md), [34](34_arduino-examples.md), [35](35_idf-examples.md), [38](38_cli-device-operations.md), [39](39_python-hil-runner.md).
Produces: `package-qualification`, `package-doc-tools`.
Consumes: `native-qualification`, `version-tools`, `board-guide`, `arduino-adapter-source`, `idf-adapter-source`, `cli-complete`, `firmware-cli-handoff`, `hil-runner`, `arduino-build-record`, `idf-build-record`, `host-test-entrypoint`.

## Objective and boundaries

Make TMP11x a clean independently consumable MCU/native library and document its actual verified feature set without bundling testing/firmware prerequisites into installed core.

## Source audit

Recheck TMP1x2 `CMakeLists.txt`, `library.json`, `idf_component.yml`, version generation, `docs/testing.md` and `tools/check_doxygen.py`. DP8382x prompt 70 is a planned relocated-consumer contract, not proof of an implemented install/export precedent. Implement and verify TMP11x's own relocated consumer. Inspect actual build outputs, current API and feature coverage.

## Complete implementation work

- Audit and complete `library.json`, CMake config/install/export, native `add_subdirectory`, ESP-IDF component metadata, Arduino `lib_deps` from GitHub and generated committed `Version.h` with single version source.
- Implement `tools/check_package.py` and `tools/check_doxygen.py`, invoked with `python <path>` and nonzero failure exits, plus their meaningful negative tests. `python tools/check_package.py` is the packaging entrypoint; document any required archive preparation command and arguments. Consume stage 10's real version generator/checker instead of inventing another version authority. Publish exact commands in `docs/testing.md` and register checks in the standard host entrypoint used by stage 43.
- Create isolated source-consumer, archive/extracted consumer and relocated CMake-installed `find_package` consumer tests without source-tree/sibling fallbacks or hidden generated outputs. Prove native consumers do not invoke Git/Python or need an SDK; build tools used to prepare the archive/test harness may still use them. SDK consumer tests explicitly declare their toolchain and dependencies.
- Ensure source/header license/attribution, archive include/exclude, no vendor/GPL code, no reference PDFs, no test fixtures or CLI auto-linked into portable core. `#include <TMP11x/TMP11x.h>` must work without a generator execution.
- Add strict meaningful public Doxygen, user/architecture/variant/config/freshness/EEPROM/CLI/HIL/IDF/Arduino integration, troubleshooting and test documentation. Use the canonical FULL_COVERAGE evidence columns without merging host, target, CI or physical proof.
- Produce `Doxyfile` with `INPUT` limited to `include/TMP11x` and selected user guides; exclude promptsets, audits, reports, vendor PDFs and build artifacts. Keep warnings actionable, record local/CI Doxygen versions, and include `PROJECT_NUMBER` in the existing version generator's synchronization inventory.
- Prove a sample application can simultaneously use TMP1x2 and TMP11x from independent pinned packages; it may fetch/install the explicitly declared TMP1x2 dependency but must not require the local sibling checkout. If stage 36 has delivered usable coexistence application source, reuse it and qualify its package consumption here; otherwise create the shared sample here without waiting for stage 36 or a P4 SDK. A later/resumed stage 36 reuses this application source with its P4 glue. Demonstrate each typed model separately from physical qualification. Keep TMP1x2 optional and absent from TMP11x's exported dependency graph.

- Publish the board/precision guide and exact-grade/package matrix required by F01–F04/F16–F17. Include voltage/temperature/accuracy-test conditions, bus timing/timeout/pulls, ADD0, decoupling, thermal response/strain/self-heating and part-specific thermal-pad recommendations. Distinguish typical current curves from guaranteed operating limits.
- Document all included R/F and excluded X rows, reset-content conflicts and source-unknown semantics. Headline sensor accuracy, NIST words and a hardware offset never establish system certification or calibrated application accuracy.
- Finalize the guide created in stage 4 and extended by 34/35; reconcile stage 38 firmware integration metadata after any packaging/example changes. Maintain independent availability of native package checks and target-specific build evidence; do not imply an old image covers changed CLI or adapter source.
- Close two named sub-checkpoints in the stage report: package/consumer qualification, then documentation/Doxygen. Each records its checks and meaningful commit; neither completes the whole stage by itself. Reuse one independent-package coexistence sample across relevant targets rather than cloning it in each example. Keep the pinned GitHub `lib_deps` gate, recording network/authentication limits without replacing it with a local sibling fallback.

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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_42.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
