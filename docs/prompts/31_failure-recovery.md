# 31 — Harden cancellation, ambiguous effects, timeout and health

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [15](15_passive-health.md), [19](19_config-trust.md), [25](25_per-device-reset.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md), [30](30_cooperative-measurement.md).

## Objective and boundaries

Close adversarial failure paths so unattended devices can rejoin after I2C errors or power cycles without silent incorrect samples.

## Source audit

Revisit TMP1x2 owner/dirty/health/timeout fault tests, ADS1115 indeterminate write cases and exact IO callback implementation. Check error fallback behavior of Arduino and IDF adapters only as planned, not yet production.

## Complete implementation work

- Inject transport I2C_NACK, TIMEOUT, BUS, malformed IN_PROGRESS and short transactions during every READ/CONFIG/RECOVER phase.
- Distinguish tracked bus health, whole-operation timeout, failed precondition, unsupported variant, EEPROM unsafe, and configuration dirty status. Counter saturation and last error preservation must be deterministic.
- Implement intentional recovery after outside reset and power interruption: require identity/locked-state proof before writes; preserve desired settings; do not automatically reset entire I2C bus.
- Cancellation retains terminal result and uncertain mutating effect; no automatic replay of ambiguous writes or fake rollback.
- Verify polling/reading does not silently consume an already retained terminal result; overflowed time domain and bad deadline are explicitly rejected.

## Verification and exit criteria

Use independent reviewer/fault matrix; run all registered host fault suites with strict warnings, active asserts and relevant sanitizers if available. Capture all open gaps.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_31.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
