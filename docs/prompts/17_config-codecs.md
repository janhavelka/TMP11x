# 17 — Implement type-safe CONFIG codecs and validation

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [11](11_status-values.md), [12](12_i2c-register-transport.md).

## Objective and boundaries

Make configuration assembly/decoding independent from physical I2C, reserved-bit modification and status read-to-clear effects.

## Source audit

Read original CONFIG table for each part, including mode[11:10], CONV[9:7], AVG[6:5], T/nA[4], POL[3], DR/ALERT[2], status[15:12] and bit[1:0]. Compare TMP1x2 `packConfiguration` and ADS1115 configuration mask patterns.

## Complete implementation work

- Implement `encodeConfiguration`, `decodeConfiguration`, validation of typed settings, stable writable masks and documented legal values. Preserve reserved bits and command-only bits; never RMW a destructive status flag as if persistent.
- Distinguish volatile hardware state from reset-loaded EEPROM defaults. Do not hardcode `0x0220` as proof of live configuration or assume a sensor has factory calibration.
- Enumerate allowed conversion mode, 3-bit cycle selection, 2-bit averaging, thresholds, therm/alert, polarity and data-ready output. Explicitly handle model-specific soft-reset bit and reserved bits.
- Create pure expected-register comparisons of only stable fields, and a configuration snapshot with raw observed status flags and explicit read-to-clear documentation.
- Add independent literal bit/word vectors and invalid enum/reserved-bit tests for all supported families.

## Verification and exit criteria

Check all emitted CONFIG writes against a manually reviewed mask ledger; run exhaustive combinations of legal mode/cycle/average settings and rejected reserved encodings.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_17.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
