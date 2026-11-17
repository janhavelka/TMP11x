# 10 — Create the independent buildable library skeleton

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Architecture and package foundation**. Type: **software**.
Prerequisites: [2](2_tooling-peer-audit.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md).
Produces: `build-foundation`, `version-tools`, `host-test-entrypoint`.
Consumes: `tooling-plan`, `api-contract`, `owner-contract`.

## Objective and boundaries

Build the first working library foundation without importing device behavior prematurely. Keep consumers independent of sibling checkouts.

## Source audit

Inspect TMP1x2 and OPT4001 CMake, metadata, tests-OFF consumer, generated Version.h, MIT license, `scripts/pio.cmd`, ESP-IDF component naming and template layout. Confirm source license before adaptation.

## Complete implementation work

- Create `include/TMP11x/`, `src/`, `test/`, `examples/`, `tools/`, `scripts/`, `docs/reports/`, `CMakeLists.txt`, `library.json`, `idf_component.yml` and minimal `CHANGELOG.md`. Reuse reviewed preparation files (`.gitignore`, `.gitattributes`, MIT LICENSE, `scripts/pio.cmd`) without replacing provenance. Use `0.1.0` as the initial development version; this does not create or claim a release/tag.
- Adapt selected MIT version generator, checked-in `Version.h`, CMake target and framework-free test registration; pin and document exact source attribution. Do not bring old sensor implementation or TI reference binaries into `src/`.
- Provide `scripts/generate_version.py` with a nonmutating check mode and `tools/check_version_sync.py` as a thin delegation to that check, invoked as `python <path>`, with `library.json` as the sole version authority and nonzero exit on drift. Do not duplicate version parsing/generation rules. Register these checks, the stage 5 register-validator regression runner and minimal real C++ tests in the standard host entrypoint. Stage 42 owns `tools/check_package.py` and `tools/check_doxygen.py`; do not call them before creation.
- Own `docs/testing.md`: document standard host commands, dependency scope and the exact `TMP11X_BUILD_TESTS` CMake option used by tests-OFF consumers. Stages 37–39 append CLI/runner suites to this entrypoint; stage 33 results do not cover tests introduced later.
- Provide a native tests-OFF compile/link consumer using an actual generated version API or a small real pure type already frozen in prompt 8. Record any minimal type implemented here so prompt 11 extends it instead of duplicating it. Do not invent a no-op sensor API merely to produce a linked library. Keep later ESP-IDF platform files optional, not empty fake drivers.
- Set up local test/build scaffolding for subsequent stages. Stage 43 creates the initial remote workflow if none exists; inspect and preserve any existing automatic triggers instead of disabling them to avoid runs. Do not add or dispatch a remote workflow here merely as scaffolding.

## Verification and exit criteria

Stages 1–42 add no initial `.github/workflows/*`; stage 43 owns their creation.
Preserve any workflows/triggers/protection already present rather than deleting
them to enforce this sequencing rule. Local test scaffolding is required now.

Run clean native CMake configure/build/CTest against the minimal real types, an isolated public-header compile, version reproducibility and package listing. No declared chip function succeeds without implementation.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_10.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
