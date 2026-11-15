# 26 — Implement read-only register snapshots and guarded raw access

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md).

## Objective and boundaries

Make diagnostics useful without bypassing register safety, readiness accounting or health/error visibility.

## Source audit

Inspect TMP1x2 `readSnapshot`, `readRegisterRaw`, `BusOperations`, OPT4001 clear-on-read FLAGS and test CLI dump behavior. Recheck source ledger access and model-specific 0x07 alias.

## Complete implementation work

- Implement the stage 8 bound-only diagnostic route: validated binding permits allowed register reads without initialization or any implicit CONFIG write. Keep health-neutral diagnostics, observed raw state, unknown startup/sample validity and destructive-read effects explicit; do not claim managed configuration trust.
- Implement checked read of documented registers with exact per-model address validation, read-to-clear disclosures, raw snapshots and optional tracked readback.
- Diagnostic snapshots must record register read order and whether DATA_READY/alert events may have been consumed; no fake atomic multi-register snapshot or unbounded looping.
- If a raw volatile write facility is retained, require a narrow allowlist, startup/EUN/busy guard and dirty-state invalidation. Reject DEVICE_ID, TEMP, EEPROM_UL and EEPROM word writes. Keep 0x07 offset typed-only on TMP117/119 as required by stage 24. Raw CONFIG must reject Soft_Reset, one-shot commands and reserved-bit writes; it cannot bypass the typed operation and freshness contracts.
- Do not permit unsupported-family access by raw pointer escaping. Core diagnostics operate only on the bound address; address scans belong to the application and do not prove family or grade. A live diagnostic read can acknowledge status even when it writes no data register.
- Add a concise register dump format with unsigned hex, signed decoded units where known, source-model and status flag caveats.

## Verification and exit criteria

Tests exercise every register address and model, skipped holes, alias 0x07, repeated clear-on-read reads, raw write denial, dirty evidence and no extra callbacks on forbidden operations.
Include a freshly bound, uninitialized transcript of identity, CONFIG, TEMP
and permitted EEPROM-shadow observations with zero data-register writes.
Verify unknown validity/trust is reported and no hidden `begin` occurs.

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
