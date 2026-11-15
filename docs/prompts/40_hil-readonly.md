# 40 — Qualify read-only TMP11x hardware identity and measurement

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Physical fixture qualification**. Type: **physical**.
Prerequisites: [34](34_arduino-examples.md), [35](35_idf-examples.md), [39](39_python-hil-runner.md).

## Objective and boundaries

Run nonmutating physical measurements only on a known authorized sensor fixture; otherwise deliver a complete executable qualification recipe and preserve BLOCKED/NOT RUN status.

## Source audit

Recheck exact actual PCB/BOM chip marking, board power/pullups/ADD0/ALERT/wiring, firmware image identity, TI specifications and the vetted runner. Read recent hardware/bench facts; do not assume that all TMP11x parts are physically available.

## Complete implementation work

- Document physical setup, VDD, I2C lines/pullups, clock, GPIO, address, ambient probe/thermometer (if present), exact model/grade/lot, date, run command and firmware SHA.
- On a user-authorized fixture, run identity, initial temperature/freshness, time/averaging observations, safe read-only EEPROM and status-flag read-to-clear checks. Keep a qualified temperature reference distinct from expected room-temperature plausibility.
- Capture raw transcript/JSON and per-part evidence rows; tests of one TMP117 do not qualify TMP116 or TMP119 or specific package/grade. No nonvolatile writes or bus-wide reset, even for troubleshooting.
- On absent hardware, record stage as BLOCKED, store the ready commands/expected observations and resume only after fixture data becomes available.
- Ensure a read-only campaign never configures a sensor, clears factory state or exercises stale raw data as fresh.

## Verification and exit criteria

Physical PASS only with identified hardware and actual unmodified runner transcripts. Review failed/mismatched IDs and unstable readings as evidence; keep host/software claims separate.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_40.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
