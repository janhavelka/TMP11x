# 25 — Implement supported per-device reset and invalidation safely

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [9](9_owner-deadline-contract.md), [14](14_binding-lifecycle.md), [18](18_safe-initialize.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md).

## Objective and boundaries

Handle documented individually addressed software reset, if supported, without sending general-call reset to every device on the shared bus.

## Source audit

TMP117 p.28 Table 7-6 and TMP119 p.35 Table 8-5 explicitly describe CONFIG
bit 1 `Soft_Reset` as R/W, triggering a 2-ms reset and reading zero. Their
figures label it R-0; record that inconsistency without ignoring the detailed
field definitions. TMP116 p.26 reserves bits 1:0. Review TMP1x2 external reset
handling, not its bus-wide reset implementation.

## Complete implementation work

- Record a source-verified reset capability for each part and define `softResetDevice` only when the exact datasheet proves a safe individually addressed command and write encoding.
- If the source remains ambiguous, report capability gated and do not emit a reset write. Use `invalidateDeviceState` as the supported local operation; do not disguise it as physical reset.
- For supported reset, establish startup readiness and EUN=0/busy=0 before writing CONFIG; reset is not a way to escape an unlocked device safely. Emit one reset-command write to the selected address, invalidate configuration/conversion/alert evidence before the attempt, and retain uncertainty if the write fails.
- The sensor reloads its EEPROM-backed state; software must not assume factory defaults or write them back as reset proof. Honor the documented 2-ms reset interval before readiness/readback checks, using stage 6's explicit timing policy. A cleared Soft_Reset bit is not completion evidence because it always reads zero. Reapply desired volatile settings only when the caller explicitly selected that behavior.
- Never use I2C general-call address 0x00, global reset from TMP1x2 or TCA9548A RESET as a shortcut. No reset in begin()/recover() without explicit caller request.
- Include reset decision in device ledger and per-variant capability matrix.

## Verification and exit criteria

Native tests assert one addressed reset write plus separately counted preflight
and post-reset reads, zero bus-wide commands, unsupported TMP116 error before
I2C, no CONFIG write when EUN is set/busy/startup is uncertain, failed-reset
trust invalidation, and no premature readiness from the self-cleared command.

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
