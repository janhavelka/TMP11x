# 25 — Implement supported per-device reset and invalidation safely

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [9](9_owner-deadline-contract.md), [14](14_binding-lifecycle.md), [18](18_safe-initialize.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md), [23](23_offset-feature.md).
Produces: `device-reset`.
Consumes: `safety-policy`, `owner-contract`, `binding-api`, `sync-initialize-recover`, `config-trust`, `sync-acquisition`, `offset-feature`, `register-fake`.

## Objective and boundaries

Handle documented individually addressed software reset, if supported, without sending general-call reset to every device on the shared bus.

## Source audit

TMP117 p.28 Table 7-6 and TMP119 p.35 Table 8-5 explicitly describe CONFIG
bit 1 `Soft_Reset` as R/W, triggering a 2-ms reset and reading zero. Their
figures label it R-0; record that inconsistency without ignoring the detailed
field definitions. TMP116 p.26 reserves bits 1:0. Review TMP1x2 external reset
handling, not its bus-wide reset implementation.

## Complete implementation work

- Record the addressed reset capability from TMP117/TMP119's detailed field definitions; TMP116 remains unsupported. Freeze the conservative library encoding: last verified stable CONFIG with MOD=00/01, read-only bits 15:12 and reserved bit 0 zero, OR 0x0002. Never copy transient MOD=11 or stale raw flags. This is library write policy using documented fields, not proof of simultaneous non-reset-field effects. If verified stable input/startup/safety prerequisites cannot be established, emit no reset write; local invalidation is not physical reset.
- For supported reset, establish startup readiness and EUN=0/busy=0 before writing CONFIG; reset is not a way to escape an unlocked device safely. Emit one reset-command write to the selected address and invalidate current configuration/conversion/live-alert trust before the attempt. Preserve unconsumed historical alert events with their old generation and reset/unknown provenance; reset must not silently acknowledge them. Retain uncertainty if the write fails.
- Clear the internal startup origin/proof before the reset attempt too. After the separate documented reset wait, establish new FIRST_ACK evidence from the first successful compatible post-reset read and complete the startup policy before any further write. Never reuse a pre-reset ACK timestamp; failed/ambiguous reset attempts also discard it. Reported external reset uses argument-free `invalidateDeviceState()` and explicit recovery under stable power.
- Addressed Soft_Reset's EEPROM reload, EUN effect, companion-bit application and bus availability are not explicit in the source. General-call reset's reload/relock description does not prove them. Invalidate the old safety observation and honor a policy wait of at least the documented 2 ms before rechecking 0x04 and full profile/identity observations. Require EUN=0/busy=0 plus startup policy again before any subsequent write. A zero Soft_Reset bit is never completion proof. Keep every register untrusted until observed; no assumed factory image or silent relock. Reapply desired owned volatile settings only when the caller explicitly selected that behavior.
- Never use I2C general-call address 0x00, global reset from TMP1x2 or TCA9548A RESET as a shortcut. No reset in begin()/recover() without explicit caller request.
- Include reset decision in device ledger and per-variant capability matrix.

- Test both uncertain addressed-reset outcomes without promoting either to fact: changed/unchanged settings and EUN/busy response, including transport failure during the interval. Separate confirmed POR/general-call fixtures reload only the persistent CONFIG subset, never MOD1/one-shot/status as stored settings. Use nonfactory limits, managed/unmanaged offset and EEPROM shadows. Explicit recovery reapplies only the owned profile and must not overwrite unmanaged calibration.

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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_25.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
