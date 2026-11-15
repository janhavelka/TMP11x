# 40 — Qualify read-only TMP11x hardware identity and measurement

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Physical fixture qualification**. Type: **physical**.
Prerequisites: [39](39_python-hil-runner.md).

Physical execution also requires one identified, fully linked compatible
firmware from stage 34 **or** stage 35, its observation startup path, and an
authorized fixture. The dependency list does not waive those runtime gates.

## Objective and boundaries

Run register-write-free physical observations only on a known authorized sensor fixture; otherwise deliver a complete executable qualification recipe and preserve BLOCKED/NOT RUN status. Reads are not side-effect-free: CONFIG and TEMP can acknowledge status flags.

## Source audit

Recheck exact actual PCB/BOM chip marking, board power/pullups/ADD0/ALERT/wiring, firmware image identity, TI specifications and the vetted runner. Read recent hardware/bench facts; do not assume that all TMP11x parts are physically available.

## Complete implementation work

- Document physical setup, VDD, I2C lines/pullups, clock, GPIO, address, ambient probe/thermometer (if present), exact model/grade/lot, date, run command and firmware SHA.
- On a user-authorized fixture, run identity, initial temperature/provenance, observations of the existing mode/averaging, safe EEPROM reads and status-flag read-to-clear checks. Use observation firmware that binds without `begin`, reset, configuration or one-shot writes, including after a serial-triggered reboot. A shutdown sensor or absent readiness evidence yields NOT READY/unknown freshness, not a request to trigger conversion. Keep calibrated accuracy testing distinct from room-temperature plausibility.
- Capture raw transcript/JSON and per-part evidence rows; tests of one TMP117 do not qualify TMP116 or TMP119 or specific package/grade. No nonvolatile writes or bus-wide reset, even for troubleshooting.
- On absent hardware, record stage as BLOCKED, store the ready commands/expected observations and resume only after fixture data becomes available.
- Ensure an observation campaign never writes register data, clears factory state or labels stale raw data fresh. Pointer-write/repeated-START/read is allowed. Record that CONFIG/TEMP reads consume flags; those events cannot be restored, and repeated equal readings neither prove nor disprove a new conversion.

- Record the electrical/accuracy conditions from F02–F04/F16–F17: exact ADD0 strap/pinout, supply vs grade/temperature, bus rate, pullups, bypass and thermal fixture. Optional logic-analyzer evidence may qualify ACK/NACK/repeated START and timing; a printed clock setting does not.
- Observe arbitrary EEPROM-shadow values without a zero/nonzero serial acceptance rule. State which grade/measurement-condition claims remain untested; read-only room-temperature plausibility does not close full-range accuracy, thermal settling or byte-coherence questions.

## Verification and exit criteria

Physical PASS only with identified hardware, one compatible fully linked firmware image and actual unmodified runner transcripts. Both Arduino and IDF builds are not needed to qualify one fixture run. Review failed/mismatched IDs and unstable readings as evidence; keep host/software claims separate. No absolute accuracy/traceability qualification follows merely from an ID or EEPROM read.

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
