# 1 — Audit current sensor-driver ownership and APIs

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Ecosystem inspection**. Type: **software**.
Prerequisites: None — first evidence stage.
Produces: `ecosystem-audit`.
Consumes: None.

## Objective and boundaries

Establish a source-verified map of the I2C driver conventions in my GitHub repositories before making new public declarations. This stage is read-only research except its navigation/report files. Reuse preparation audits after verifying their revisions and remaining obligations.

## Source audit

Inspect `janhavelka/TMP1x2` public `Status.h`, `Config.h`, `TMP1x2.h`, `CommandTable.h`, all four `src/TMP1x2*.cpp`, registered host tests, and AGENTS; compare `OPT4001`, `ADS1115`, `TCA9548A` and `MB85RC` for differing state and bus ownership. Check the actual selected code and Git revisions, not just README descriptions.

## Complete implementation work

- Identify source declaration, implementation, real caller/test, and TMP11x destination for `Status`, I2C callbacks, health, transport retries, `bind/begin/end`, config trust, `start*/poll()/takeResult()`, sample freshness and raw diagnostics. Create `docs/library-comparison.md` and `docs/source-adoption.md` with adopted/adapted/rejected judgments.
- Check current checkout branch, upstream and origin. If the fresh checkout has no remote, record this and continue local work; do not accidentally push to a sibling. Preserve other user changes and document initial HEAD.
- Identify whether existing `janhavelka/TMP11x` already exists or is an empty intended remote; do not fabricate a GitHub repository URL or create a different remote.
- Record precise source paths and revision hashes. Identify any code actually copied later and its license; do not copy code during this audit.

## Verification and exit criteria

Verify each material API claim against implementation and a registered test/caller. Run current promptset checker and `git diff --check`; report unavailable GitHub private paths instead of guessing.

## Additional review

One lead owns the comparison; assign a read-only independent health/owner review if possible.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_1.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
