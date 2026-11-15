# 25 — Implement supported per-device reset and invalidation safely

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [9](9_owner-deadline-contract.md), [14](14_binding-lifecycle.md), [18](18_safe-initialize.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md).

## Objective and boundaries

Handle documented individually addressed software reset, if supported, without sending general-call reset to every device on the shared bus.

## Source audit

Investigate TMP117/TMP119 CONFIG bit 1 `Soft_Reset` versus TMP116 reserved bits and exact write/readback procedure. Check TI wording/access table carefully: some figures mark it read-only while naming reset. Review TMP1x2 external reset handling, not its bus-wide reset implementation.

## Complete implementation work

- Record a source-verified reset capability for each part and define `softResetDevice` only when the exact datasheet proves a safe individually addressed command and write encoding.
- If the source remains ambiguous, report capability gated and do not emit a reset write. Use `invalidateDeviceState` as the supported local operation; do not disguise it as physical reset.
- For a proven reset, implement only selected-device address, restore EEPROM-loaded startup state, invalidate conversion/alert evidence, wait for EEPROM busy to clear and optionally reapply desired volatile settings under normal locked-state checks.
- Never use I2C general-call address 0x00, global reset from TMP1x2 or TCA9548A RESET as a shortcut. No reset in begin()/recover() without explicit caller request.
- Include reset decision in device ledger and per-variant capability matrix.

## Verification and exit criteria

Native tests assert exactly one addressed command for supported part, zero bus-wide commands, unsupported variant error before I2C, failed reset trust and post-reset re-read evidence.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_25.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
