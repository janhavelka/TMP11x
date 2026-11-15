# 41 — Qualify volatile settings, recovery and finite hardware soak

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Physical fixture qualification**. Type: **physical**.
Prerequisites: [31](31_failure-recovery.md), [39](39_python-hil-runner.md), [40](40_hil-readonly.md).

## Objective and boundaries

Exercise configuration, recovery and sustained read behavior on a known fixture with verified preimage/restore, without risking nonvolatile EEPROM state.

## Source audit

Revisit TMP11x EEPROM protection and TI power-up busy behavior, TMP1x2 full HIL and soak flow, current runner safety hooks and identified stage40 fixture evidence.

## Complete implementation work

- Obtain explicit active-fixture authorization and capture initial CONFIG/THIGH/TLOW/OFFSET as raw preimage plus EEPROM_UL locked proof, temperature/ID and exact firmware identity.
- Run finite mode/cycle/averaging and limits/ALERT/offset volatile trials only when model supports them; verify write-readback, measurement fresh evidence and each restoration step. Never unlock or program EEPROM.
- Test unplug/replug/power cycling and recovery only when electrical setup permits safe handling; record physical behavior, duration, sample counts and missing/duplicate conversion evidence. Do not use unbounded or unattended unsafe writes.
- Every failed/aborted test attempts bounded restoration and reports independent primary and restoration results; if restore cannot be proven, record a hard failure and current hardware state.
- A sensor unavailable at stage40 leaves this stage BLOCKED; prepare runner commands and guard tests without producing a false hardware PASS.

## Verification and exit criteria

Verify actual physical transcripts, no EEPROM write/protection violations, exact restored raw register values, stable soak counters and failure handling. Qualify per exact tested part and fixture only.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_41.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
