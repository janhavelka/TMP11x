# 9 — Freeze cooperative ownership, timing and result contract

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Architecture and package foundation**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md).

## Objective and boundaries

Select exact semantics for bounded multi-turn operations, freshness, cancellation and ambiguous effects before implementation.

## Source audit

Inspect TMP1x2 `src/TMP1x2Operations.cpp`, `docs/owner-operations.md`, timeout tests; OPT4001 and ADS1115 owner interfaces. Study actual callback timeout/error mapping from peers.

## Complete implementation work

- Add `docs/owner-operations.md`: `startInitialize`, `startConfigure`, `startRead`, `startRecover`, `poll(nowMs,maxTransfers)`, `cancel`, `takeResult` with single active slot and retained terminal identity.
- Freeze callback timeout budget versus whole-job deadline, 32-bit wrap-safe timestamps, zero-transfer polls, unsupported operations and exactly-once result retention.
- Distinguish conversion trigger, ready observation, cached latest and confirmed fresh result; retain read-to-clear status in an owner-visible observation rather than throwing it away.
- Define cancellation/timeout failure trust: no automatic hardware rollback, no duplicate job completion, no overwriting pending result, no reentrancy and no callback buffered past return.
- Specify rules for external multiplexer route selection between polls and temporary I2C owner lock (one callback/transaction, never across sensor conversion wait).

## Verification and exit criteria

Use small native pseudocode/fake transfer traces to demonstrate one-read/one-write budget and no I2C on admission/cancel. Independent review checks ambiguous partial writes and operation-result lifetime.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_9.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
