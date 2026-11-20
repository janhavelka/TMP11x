# 43 — Run one dedicated remote CI integration checkpoint

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Package, CI and final review**. Type: **ci**.
Prerequisites: [33](33_native-suite.md), [35](35_idf-examples.md), [42](42_package-docs.md), [55](55_audit-42.md).
Produces: `ci-record`.
Consumes: `native-qualification`, `idf-adapter-source`, `register-validator`, `version-tools`, `package-doc-tools`, `package-qualification`, `cli-complete`, `hil-runner`, `host-test-entrypoint`, `arduino-adapter-source`, `firmware-cli-handoff`, `audit-42`.

## Objective and boundaries

Create and verify the first complete GitHub Actions software CI checkpoint, with exact commit/job provenance and no repeated remote dispatch on every earlier prompt.

## Source audit

Audit TMP1x2 and TCA9548A pinned current workflow actions/dependency rules, CMake test registration, generation checks, DP8382x `docs/prompts/71-ci-checkpoint.md` at its recorded revision and actual available toolchain versions. `scripts/pio.cmd` selects the Windows managed Core; Linux CI uses its own explicitly pinned installation. Record both versions and explain any difference. Do not run sibling remote CI.

## Complete implementation work

- Create `.github/workflows/ci.yml` using the local entrypoints from stage 10 and later stages. If a workflow was independently added in the meantime, inspect and complete it while preserving existing push/pull_request/optional workflow_dispatch triggers and required checks. Use minimal permissions, pinned actions/tool versions, finite timeouts and cache keys matching actual dependencies; do not replace a working workflow merely because this stage says to create one.
- Register native GCC/Clang or installed toolchain checks, active Release assertions, registered tests, ledger/promptset/checker integrity, sanitizer if reliable, clean source/package/install consumer, documentation, plus feasible Arduino S2/S3 and native ESP-IDF full-link jobs.
- Explicitly execute the compiled CLI/parser/golden tests added in 37/38 and Python runner plus real compiled-fixture integration tests added in 39. Use the updated standard host entrypoint and named version/register/package/Doxygen tools from 5/10/42; checking only the stage 33 test inventory is insufficient. Full firmware jobs use the final CLI-integrated sources and refresh the stage 38 handoff evidence.
- Keep an ESP32-P4 job when stage 36's source and a supported SDK/toolchain are available; no physical board is needed for linking. Report any missing target gate explicitly, without turning a skipped job into support evidence. Hardware HIL remains a separate opt-in fixture job and must never open a physical port in default hosted CI.
- Run corresponding local checks, commit and safely push inputs first; identify exact GitHub workflow run associated with the tested commit, inspect completed required jobs, fix actual failures and rerun this single checkpoint as necessary to establish a green final tested SHA.
- A subsequent report/README commit does not retroactively become the tested SHA. Keep original hooks/permissions intact, never use skip-ci to avoid failures and never label queued/pending jobs PASS.

## Verification and exit criteria

Record commit SHA, run/job links, versions, exact suite counts and terminal status. If remote access absent, keep CI NOT RUN/BLOCKED with local evidence while moving on to independent final audit.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_43.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
