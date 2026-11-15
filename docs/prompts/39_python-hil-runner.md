# 39 — Implement correlated Python HIL runner and finite campaigns

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [37](37_cli-baseline.md), [38](38_cli-device-operations.md).

## Objective and boundaries

Provide unattended but safe diagnostic verification tooling that records reproducible real-serial evidence, never invents fixture results and never performs EEPROM programming.

## Source audit

Study TMP1x2 `tools/hil_tmp1x2_runner.py` and parser/unit tests, OPT4001 and LDC1614 runners, DP8382x runner/event contracts and this promptset `CLI_CONTRACT` and `VERIFICATION`. Reuse parsing/record ideas selectively, not board pin assumptions.

## Complete implementation work

- Build Python CLI tool with explicit serial port, baud, expected firmware version/identity/model, optional fixture/board description, command timeout and output directory.
- Correlate requests and completions by nonce/sequence; detect split/late/duplicated/missing records, reboot/line noise, incomplete transactions and failure while waiting. Keep finite buffer/record/time bounds and return meaningful nonzero exits.
- Implement read-only identity, configuration, temperature, device-capability, flags and EEPROM campaigns by default. More intrusive volatile config/restoration trials must be opt-in and require explicit fixture authorization.
- Record exact raw transcript, structured JSON summary, input environment/firmware git identity, document revision, suite counts, PASS/FAIL/SKIP/NOT RUN and restoration outcome; no synthetic serial printout may be marked physical PASS.
- Provide a dry-run plan that opens no port, unit tests for runner parser/record integrity, and a compiled fake CLI integration harness for full protocol exercise.

## Verification and exit criteria

Test interrupted sessions, nonce mismatch, malformed values, missing verification, restore failure, false-positive PASS string, bounded soak termination and no fixture behavior. Mark actual hardware NOT RUN when absent.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_39.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
