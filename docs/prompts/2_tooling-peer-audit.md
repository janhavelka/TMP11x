# 2 — Audit build, examples, prompt and test infrastructure

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Ecosystem inspection**. Type: **software**.
Prerequisites: [1](1_ecosystem-core-audit.md).
Produces: `tooling-plan`.
Consumes: `ecosystem-audit`.

## Objective and boundaries

Select a minimal reproducible repository layout and test/build gates from existing working projects. Do not inherit every huge feature from an unrelated project.

## Source audit

Read TMP1x2 `CMakeLists.txt`, `library.json`, `idf_component.yml`, `.github/workflows/ci.yml`, `scripts/{pio.cmd,generate_version.py}`, `examples/`, `test/`, and tools; compare `OPT4001`, `esp32-platformio-library-template`, MotorControl-CAN's `docs/prompts/{START_HERE,SEQUENCE,COMMON,PROGRESS}` and DP8382x's `docs/prompts/{COMMON,EVIDENCE,VERIFICATION}`.

## Complete implementation work

- Create `docs/toolchain-and-package-plan.md`: native C++17, PlatformIO, Arduino ESP32-S2/S3, native IDF S2/S3, optional P4, CMake/export, CLI, HIL, docs and version source.
- Document which specific files/scripts can be selectively copied unchanged and which must be rewritten or eliminated; never bundle unrelated `docs/reference/` or old hardware logs.
- Determine the installed VS Code-managed PlatformIO path mechanism (the sibling wrapper is a reference; do not install another PIO Core), available IDF/compiler versions and missing targets.
- Verify this 44-file sequence, dependency metadata and link/checker design against the two existing promptset precedents. Record any mismatch in `docs/reports/prompt_2.md`.

## Verification and exit criteria

Run promptset checker; verify all selected sibling build rules exist; no firmware builds are claimed before production sources exist.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_2.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
