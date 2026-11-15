# 20 — Implement continuous operation, averaging and cycle timing

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [6](6_timing-status.md), [8](8_api-architecture.md), [16](16_temperature-codec.md), [17](17_config-codecs.md), [18](18_safe-initialize.md), [19](19_config-trust.md).

## Objective and boundaries

Provide fully typed mode/cycle/averaging controls and accurately report conversion time versus sample frequency and standby time.

## Source audit

Compare TI cycle and averaging timing tables for TMP116/TMP117/TMP119; inspect OPT4001 mode API, TMP1x2 rate setters and wrong extended-mode timing assumptions.

## Complete implementation work

- Implement exact per-model mode, conversion-cycle and averaging selectors, pure timing helpers and safe configuration application. Distinguish nominal cycle-table values, specified single-conversion limits, derived averaged bounds and application timeout margins; do not label nominal 125/500/1000-ms values guaranteed maxima.
- Keep one-shot trigger separate from continuous mode and avoid implicit changes to averaging or cycle on read.
- Invalidate current-generation sample evidence after conversion-affecting changes. Shutdown aborts an active conversion; changing settings does not relabel an existing TEMP value as a conversion under the new profile.
- Report configured/estimated period and latency, not a measured actual update rate. There is no exposed sample counter. The CC cycle is active conversion plus standby; when averaging exceeds the selected cycle, standby is zero. OS has no standby and CONV does not affect its duration (TMP116 pp.12-14,27; TMP117 pp.14-15,28; TMP119 pp.16-17,35).
- Ensure timing values reflect maximum needed for averaged result, not an optimistic single ADC sub-conversion.

## Verification and exit criteria

Test all legal mode/cycle/average combinations and bounds, false early readiness, correct mode after changing settings and unchanged bus state on invalid requests.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_20.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
