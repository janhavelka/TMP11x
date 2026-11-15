# 22 — Implement temperature limits, therm/alert and ALERT pin

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md), [16](16_temperature-codec.md), [17](17_config-codecs.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md).

## Objective and boundaries

Support complete programmable high/low limits and mode-specific alerts with truthful acknowledge behavior.

## Source audit

Compare TMP116/TMP117/TMP119 limit representations, alert vs therm behavior, polarity and DR/ALERT selection; inspect TMP1x2 thresholds/alert (but do not inherit fault-queue semantics) and OPT4001 interrupt handling.

## Complete implementation work

- Implement `setLimits`, `readLimits`, negative threshold encoding, strict ordering policy if documented, therm/alert mode, polarity and ALERT vs data-ready pin selection, with readback.
- Publish decoded `AlertStatus` with high/low flags, source read timestamp/validity and explicit read-to-clear effects. In therm mode, low flag semantics differ; do not infer a symmetric window.
- Optional GPIO read callback must not configure a pin or be invoked from ISR. Physical pin state and sensor register flags remain distinct observations.
- Document all scenarios where CONFIG/TEMP reads clear alert/data-ready status. One probe/dashboard action must not silently promise non-destructive observation.
- Verify no unsupported TMP1x2 fault queue, interrupt queue or extended temperature mode leaks into API.

## Verification and exit criteria

Register tests: high/low positive and negative exact words, therm/hysteresis state machine, flag clear-on-read, disabled GPIO, polarity and explicit data-ready output mode.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_22.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
