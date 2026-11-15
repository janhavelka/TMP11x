# 28 — Implement bus-silent jobs and exactly-once results

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [9](9_owner-deadline-contract.md), [14](14_binding-lifecycle.md), [15](15_passive-health.md), [27](27_sync-helpers-cache.md).
Produces: `job-lifecycle`.
Consumes: `owner-contract`, `binding-api`, `transport-health`, `config-trust`, `sync-api`.

## Objective and boundaries

Build the selected cooperative owner operation entry/exit mechanism, not another bus manager or opaque retry queue.

## Source audit

Adapt audited TMP1x2 `src/TMP1x2Operations.cpp`, `OperationResult`, token API and tests; examine MB85RC owner retention as a secondary example.

## Complete implementation work

- Implement the shared admission/result machinery, one active slot, typed `OperationKind`, stable `OperationToken` and copied staged target. Keep an operation's public start route unavailable until its real phase engine is connected in stages 29/30; a scaffold must not accept a job that can only hang or return fabricated success.
- Admission performs **zero I2C** and does not overwrite output token on rejection. While active or terminal result pending, conflicting mutating operations/rebinds reject predictably.
- Implement and unit-test private poll/result transitions with `maxTransfers` budget (including zero), bounded CPU-only transitions, no yield/sleep and `takeResult` exactly once with token match. Public admitted operations must have real execution, cancellation and timeout terminal paths.
- Apply stage 9's post-callback clock policy. Entry `nowMs` alone cannot measure time spent inside a callback or prove a timely completion. Do not present the selected peer's external-time approximation as an exact callback-completion timestamp.
- Define and test cancel, end/unbind behavior for active operations/result slots; no operation state may become silently unreachable.
- Connect stage 14 lifecycle and stage 19 invalidation guards to actual active/retained slots. Test rebind/end/unbind/invalidate with active jobs and pending results under the frozen policy, initially using the private lifecycle harness and again with real engines in stages 29/30.
- Validate immutable binding fields before token/slot commitment. A configure/initialize/recover request cannot change model, address, callbacks/contexts, clock epoch or transport policy. Reject mismatch with zero I2C and unchanged caller token; deliberate binding changes use the lifecycle API.
- Owner budget counts physical transfer callbacks, not number of register bytes. No global transport queue or TCA mux state.

## Verification and exit criteria

Native tests trace admission without I2C, zero-budget checks, foreign/stale
tokens, retained outcome, rejected admission invariants and no heap allocation.
An unconnected operation kind must reject before I2C without occupying the slot.

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
