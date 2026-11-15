# 26 — Implement read-only register snapshots and guarded raw access

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md).

## Objective and boundaries

Make diagnostics useful without bypassing register safety, readiness accounting or health/error visibility.

## Source audit

Inspect TMP1x2 `readSnapshot`, `readRegisterRaw`, `BusOperations`, OPT4001 clear-on-read FLAGS and test CLI dump behavior. Recheck source ledger access and model-specific 0x07 alias.

## Complete implementation work

- Implement checked read of documented registers with exact per-model address validation, read-to-clear disclosures, raw snapshots and optional tracked readback.
- Diagnostic snapshots must record register read order and whether DATA_READY/alert events may have been consumed; no fake atomic multi-register snapshot or unbounded looping.
- If a raw volatile write facility is retained, it must have a narrow explicit allowlist, EUN/busy guard, full dirty-state invalidation and outright rejection of EEPROM addresses. Reject attempts to write DEVICE_ID/TEMP.
- Do not permit a user to select an unsupported family by raw pointer escaping. An `i2cAddress` scan is read-only and cannot identify grade.
- Add a concise register dump format with unsigned hex, signed decoded units where known, source-model and status flag caveats.

## Verification and exit criteria

Tests exercise every register address and model, skipped holes, alias 0x07, repeated clear-on-read reads, raw write denial, dirty evidence and no extra callbacks on forbidden operations.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_26.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
