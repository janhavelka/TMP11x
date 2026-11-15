# 23 — Implement hardware offset for TMP117/TMP119 only

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [4](4_family-identity.md), [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [11](11_status-values.md), [12](12_i2c-register-transport.md), [16](16_temperature-codec.md), [17](17_config-codecs.md), [19](19_config-trust.md).

## Objective and boundaries

Expose the real temperature offset register at 0x07 only on variants where it exists, never overwrite TMP116 EEPROM3.

## Source audit

Verify TI TMP117/TMP119 `Temp_Offset` register, saturation behavior and its EEPROM-loaded reset semantics; verify TMP116 0x07 is EEPROM3. Inspect other driver codecs for signed fixed-point but derive vectors independently.

## Complete implementation work

- Implement `setTemperatureOffset`, `readTemperatureOffset`, desired offset snapshot and safe guarded 16-bit encoding for TMP117/M/N and TMP119.
- For TMP116/TMP116N, return `UNSUPPORTED` without any I2C transfer; also prevent the same access through generic raw-write or CLI paths.
- Distinguish offset applied by chip to sensor reading from an application software correction; do not reapply offset in software when reading TEMP.
- Preserve write/readback/dirty-state behavior and quantization; test saturation at boundaries and a negative offset.
- Document that a calibration offset is a volatile setting in this library, not a request to rewrite EEPROM or claim certified accuracy.

## Verification and exit criteria

Test 0x07 writes for TMP117/TMP119 only, TMP116 rejected with **zero I2C**, correct sign/rounding, ambiguous failed write and verified recovery.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_23.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
