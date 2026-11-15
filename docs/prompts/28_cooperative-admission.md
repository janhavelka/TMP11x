# 28 — Implement bus-silent jobs and exactly-once results

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [9](9_owner-deadline-contract.md), [14](14_binding-lifecycle.md), [15](15_passive-health.md), [27](27_sync-helpers-cache.md).

## Objective and boundaries

Build the selected cooperative owner operation entry/exit mechanism, not another bus manager or opaque retry queue.

## Source audit

Adapt audited TMP1x2 `src/TMP1x2Operations.cpp`, `OperationResult`, token API and tests; examine MB85RC owner retention as a secondary example.

## Complete implementation work

- Implement `startInitialize`, `startConfigure`, `startRecover`, `startRead` admission, one active request slot, typed `OperationKind`, stable `OperationToken` and copied staged target.
- Admission performs **zero I2C** and does not overwrite output token on rejection. While active or terminal result pending, conflicting mutating operations/rebinds reject predictably.
- Implement `poll` result skeleton with `maxTransfers` budget (including zero), no yield/sleep or callback beyond admitted budget, and `takeResult` exactly once with token match.
- Define and test cancel, end/unbind behavior for active operations/result slots; no operation state may become silently unreachable.
- Owner budget counts physical transfer callbacks, not number of register bytes. No global transport queue or TCA mux state.

## Verification and exit criteria

Native tests trace admission without I2C, zero-budget checks, foreign/stale tokens, retained outcome, rejected admission invariants and no dynamically growing allocations.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_28.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
