# 21 — Implement fresh-conversion acquisition and one-shot behavior

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [6](6_timing-status.md), [9](9_owner-deadline-contract.md), [16](16_temperature-codec.md), [18](18_safe-initialize.md), [19](19_config-trust.md), [20](20_conversion-settings.md).

## Objective and boundaries

Establish a trustworthy `freshConversion` contract despite clear-on-read DATA_READY and the initial -256°C placeholder.

## Source audit

Read TI data-ready clearing, one-shot trigger and timing sequences. Check OPT4001 `fresh` vs `latest` APIs and TMP1x2 manual one-shot logic only for API inspiration.

## Complete implementation work

- Implement explicit one-shot trigger, readiness observation, latest sample read, `tryReadFresh` and bounded synchronous `readBlocking` as contracted. Where a flag was cleared by an intervening raw/diagnostic read, report uncertain freshness instead of guessing.
- When reading CONFIG to check DATA_READY, capture status before it clears; handle DATA_READY and alert flags separately and avoid duplicating completion on a stale bit.
- Disallow prematurely returning initial `0x8000` as fresh before completed conversion including averaging; support a read of raw reset value for diagnostics.
- Avoid infinite loops: blocking helpers require a monotonic clock/yield or reject, and cooperative jobs later use owner-provided time. Do not hold I2C mutex while waiting.
- Explicitly model simultaneous diagnostics, one-shot trigger, shutdown, averaging change and bus access failures.

## Verification and exit criteria

Native fake-sensor tests exercise readiness cleared by CONFIG/TEMP, repeated same raw temperature with new conversion, missed ready event, initial reset value, clock timeout and no fresh fabrication.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_21.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
