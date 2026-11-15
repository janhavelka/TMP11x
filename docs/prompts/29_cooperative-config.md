# 29 — Implement cooperative initialization, settings and recovery

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [18](18_safe-initialize.md), [19](19_config-trust.md), [25](25_per-device-reset.md), [28](28_cooperative-admission.md).

## Objective and boundaries

Use one shared safe settings apply/reconcile path for synchronous and owner-driven operations, including EEPROM lock/busy waiting.

## Source audit

Inspect TMP1x2 `stepApply`, `poll`, `begin` synchronous state engine and tests. Follow TMP11x owner contract, variant register ledger and safe EUN/busy gates.

## Complete implementation work

- Connect owner `INITIALIZE`, `CONFIGURE` and `RECOVER` phases to identity, lock/busy, checked writes and stable readback; do not duplicate an independent algorithm that can drift from blocking semantics.
- Track first mutating write attempt and commit staged desired settings then; cancellation before a write must preserve previous settings while cancellation after may require recovery.
- Every `poll(nowMs,maxTransfers)` limits I2C physical callbacks, releases the bus on return and yields during EEPROM/startup waits without blocking.
- Enforce callback timeouts against remaining whole-operation budget; record which steps may have taken effect when a deadline expires after callback.
- Keep reset and EEPROM writes excluded from all normal job paths. A partially applied desired profile remains explicitly dirty until verified replay.

## Verification and exit criteria

Fault-injection tests cover every phase, late/failed writes, unchanged clock, `maxTransfers=0/1/large`, cancellation timing, CPU-only step transitions and exact callback upper bounds.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_29.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
