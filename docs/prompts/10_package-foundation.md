# 10 — Create the independent buildable library skeleton

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Architecture and package foundation**. Type: **software**.
Prerequisites: [2](2_tooling-peer-audit.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md).

## Objective and boundaries

Build the first working library foundation without importing device behavior prematurely. Keep consumers independent of sibling checkouts.

## Source audit

Inspect TMP1x2 and OPT4001 CMake, metadata, tests-OFF consumer, generated Version.h, MIT license, `scripts/pio.cmd`, ESP-IDF component naming and template layout. Confirm source license before adaptation.

## Complete implementation work

- Create `include/TMP11x/`, `src/`, `test/`, `examples/`, `tools/`, `scripts/`, `docs/reports/`, `CMakeLists.txt`, `library.json`, `idf_component.yml` and minimal `CHANGELOG.md`. Reuse reviewed preparation files (`.gitignore`, `.gitattributes`, MIT LICENSE, `scripts/pio.cmd`) without replacing provenance. Use `0.1.0` prerelease development metadata; do not claim an actual GitHub release.
- Adapt selected MIT version generator, checked-in `Version.h`, CMake target and framework-free test registration; pin and document exact source attribution. Do not bring old sensor implementation or TI reference binaries into `src/`.
- Provide a native tests-OFF compile/link consumer using one actual public type. Keep later ESP-IDF platform files optional, not empty fake drivers.
- Set up test/CI scaffolding necessary for locally executing each subsequent stage, without remote-CI dispatch until stage 43.

## Verification and exit criteria

Run clean native CMake configure/build/CTest against the minimal real types, an isolated public-header compile, version reproducibility and package listing. No declared chip function succeeds without implementation.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_10.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
