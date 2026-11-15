# 43 — Run one dedicated remote CI integration checkpoint

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Package, CI and final review**. Type: **ci**.
Prerequisites: [33](33_native-suite.md), [35](35_idf-examples.md), [42](42_package-docs.md).

## Objective and boundaries

Create and verify the first complete GitHub Actions software CI checkpoint, with exact commit/job provenance and no repeated remote dispatch on every earlier prompt.

## Source audit

Audit TMP1x2 and TCA9548A pinned current workflow actions/dependency rules, `scripts/pio.cmd`, CMake test registration, generation checks, DP8382x stage71 CI contract and actual available toolchain versions. Do not run sibling remote CI.

## Complete implementation work

- Create `.github/workflows/ci.yml` with appropriate push/pull_request/optional workflow_dispatch triggers, minimal permissions, pinned actions, finite timeouts and cache keys matching actual dependencies.
- Register native GCC/Clang or installed toolchain checks, active Release assertions, registered tests, ledger/promptset/checker integrity, sanitizer if reliable, clean source/package/install consumer, documentation, plus feasible Arduino S2/S3 and native ESP-IDF full-link jobs.
- Keep ESP32-P4 job only if exact supported SDK target is proven and buildable; report unsupported/unavailable target rather than fake skip-success. Hardware HIL remains a separate opt-in fixture job.
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
Do not run unrelated sibling suites or remote CI (except stage 43).

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_43.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
