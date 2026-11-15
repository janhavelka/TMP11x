# 38 — Add full safe configuration, job and diagnostic CLI routes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md), [25](25_per-device-reset.md), [26](26_diagnostic-access.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md), [30](30_cooperative-measurement.md), [31](31_failure-recovery.md), [37](37_cli-baseline.md).

## Objective and boundaries

Expose the actually implemented TMP11x chip functions through the CLI without introducing a bypass for EEPROM safety or global reset.

## Source audit

Review current public API and tests, TMP1x2 shared CLI measured one-shots/owner-mode controls, OPT4001 flag read-to-clear command warnings and the promptset `CLI_CONTRACT` source.

## Complete implementation work

- Add typed `mode`, `cycle`, `average`, `threshold`, `alert`, `polarity`, `ready`, `offset`, `verify`, `recover`, `invalidate`, `job`, `cancel`, `result`, `watch` and `stress` commands with help/mode/variant gates.
- Every device-mutating CLI route calls the library public API; **no CLI-only register writer** can open EEPROM_UL or ignore the busy/lock guard. Do not invent a reset command until stage25 source proof permits it.
- Render raw/cached/observed values and timestamp validity distinctly, including readiness flag consumption and one-shot evidence. Do not show a newly completed measurement from a stale data-ready read.
- Preserve one owner token/result, per-transfer budgets, cancellation and status diagnostics from library, not synthetic CLI state. Include error reasons and safe no-I/O refusals.
- Register native CLI tests and shared grammar checks so Arduino and IDF use the same exact commands and help definitions.

## Verification and exit criteria

Run negative/positive scenarios for every command; test no hidden EEPROM unlock/write, no unbounded watch, disallowed model operations, stale sample age, invalid arguments and terminal result ordering.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_38.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
