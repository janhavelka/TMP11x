# 30 — Implement owner-driven fresh measurement and one-shot jobs

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [20](20_conversion-settings.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md).

## Objective and boundaries

Provide cooperative sampling with correct readiness evidence across wait turns and interleaved other I2C activity.

## Source audit

Compare TI timing/data-ready/one-shot sections and TMP1x2 `startRead` job state machine, while replacing all TMP1x2 EM/conversion timing and format checks.

## Complete implementation work

- Implement owner `READ` for continuous latest versus shutdown one-shot (fresh), with caller-supplied time and exact transport budgets.
- Stage trigger, wait, CONFIG status sampling and TEMP read as separate steps; capture DATA_READY before a read clears it. Handle equal successive raw values as distinct measurements only with real conversion evidence.
- Record sample timestamp, freshness and read-to-clear history in the retained result. No sensor GPIO ISR can directly mutate the driver; owner may check application-supplied GPIO state.
- Support other sensor instances using the same bus between polling turns, without holding locks or assuming pointer persistence.
- Reject or preserve uncertain freshness after outside configuration reads, raw diagnostic interference, missed event or reset.

## Verification and exit criteria

Test interleaved two-sensor fake transport, one transfer per poll, 32-bit wrap, invalid/no clock, same temperature consecutive one-shots, missed ready and late successful read not published as timely result.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_30.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
