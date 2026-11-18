# 29 — Implement cooperative initialization, settings and recovery

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [18](18_safe-initialize.md), [19](19_config-trust.md), [25](25_per-device-reset.md), [28](28_cooperative-admission.md).
Produces: `configuration-jobs`.
Consumes: `safe-apply-engine`, `sync-initialize-recover`, `config-trust`, `offset-feature`, `job-lifecycle`, `register-fake`.

## Objective and boundaries

Use one shared safe settings apply/reconcile path for synchronous and owner-driven operations, including EEPROM lock/busy waiting.

## Source audit

Inspect TMP1x2 `stepApply`, `poll`, `begin` synchronous state engine and tests. Follow TMP11x owner contract, variant register ledger and safe EUN/busy gates.

## Complete implementation work

- Connect owner `INITIALIZE`, `CONFIGURE` and `RECOVER` phases to identity, lock/busy, checked writes and stable readback; do not duplicate an independent algorithm that can drift from blocking semantics.
- Reuse stage 18's resumable single-callback step engine with stage 19 trust and stage 23 opt-in offset extension. INITIALIZE uses the existing binding; CONFIGURE stages sensor settings only. The shared 0x04 guard, timing origin, shutdown/application/baseline sequence and alert retention apply unchanged. Execute stage 28's lifecycle/invalid-binding cases against these real engines and keep rejected binding changes bus-silent.
- Startup evidence is internal, with no Config seed to compare. INITIALIZE/RECOVER establish missing proof using FIRST_ACK in the current operation. Reusing a valid Config for settings after invalidation cannot fail because of a stale startup input; normal immutable-binding, ownership and settings validation still apply.
- Track first mutating write attempt and commit staged desired settings then; cancellation before a write must preserve previous settings while cancellation after may require recovery.
- Every `poll(nowMs,maxTransfers)` limits I2C callbacks and returns pending during EEPROM/startup waits; it does not call yield/sleep hooks. The application schedules the next turn. Reuse the TMP116 startup-conflict policy from stage 18 instead of equating busy-clear with reset-load completion.
- Enforce callback timeouts against remaining whole-operation budget and reobserve authoritative time after callbacks. Retain physical health, observed values, consumed flag evidence and possible effects after late success, but never clear dirty, mark initialized or publish/cache a sample on that timeout. Caller-supplied nowMs does not override the clock hook.
- Keep reset and EEPROM writes excluded from all normal job paths. A partially applied desired profile remains explicitly dirty until verified replay.

- Apply and verify the full owned profile: stable CONFIG, THIGH, TLOW and explicitly managed supported OFFSET. Preserve unmanaged hardware offset without writing it; observe its provenance. Preserve TMP116's 0x07 refusal. Inject cancellation/error after each write/readback with nondefault limits/offset; recovery must not silently restore only CONFIG or forget the staged target. Apply ordering and possible intermediate comparator states match synchronous behavior.

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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_29.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
