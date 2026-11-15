# 6 — Verify conversion, averaging, freshness and alert timing

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [3](3_ti-reference-intake.md), [5](5_register-ledger.md).

## Objective and boundaries

Establish precise conversion scheduling and read-to-clear rules for all supported parts before implementing sensor reads.

## Source audit

Revisit the inaccessible TI E2E DATA_READY race lead recorded in the
[intake notes](../reference/extracted/intake-notes.md). The thread title is
not verified silicon behavior. Resolve any relevant race using accessible
primary evidence and record remaining uncertainty before defining freshness.

Inspect TMP116/TMP117/TMP119 conversion/averaging tables, start-up EEPROM latency, DATA_READY and ALERT mode descriptions. Compare TMP1x2 `docs/owner-operations.md`, OPT4001 ready/counter behavior and their test clocks; no TMP1x2 35-ms EM wait is reusable.

## Complete implementation work

- Create `docs/device-behavior.md`: documented conversion mode state transitions, conversion/standby time, averaging count, one-shot trigger/completion and earliest conservative software deadline. Record MIN/NOM/MAX and conditions; derive separate timing estimates per exact model if they differ.
- Describe precisely when CONFIG/TEMP reads clear readiness/alert, effects of diagnostics, and what evidence is required for `freshConversion`. Do not treat time elapsed alone as universal completion in continuous mode.
- Define startup invalidity `0x8000` until first completed conversion and what information proves completion. Preserve raw numeric representation independent of valid sample state.
- Create independent temperature/count, threshold and timing test vectors with negative limits, sign/rounding, reset state and repeated reads.

## Verification and exit criteria

Review timing/wait start events with a second agent where possible. Validate each literal against TI source; no assumptions from old TMP1x2 conversion-rate enums.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_6.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
