# 26 — Implement read-only register snapshots and checked diagnostic access

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md).
Produces: `diagnostics`.
Consumes: `register-ledger`, `safety-policy`, `binding-api`, `config-trust`, `sync-acquisition`, `limits-alert`, `offset-feature`, `readonly-eeprom`, `register-fake`.

## Objective and boundaries

Make diagnostics useful without bypassing register safety, readiness accounting or health/error visibility.

## Source audit

Inspect TMP1x2 `readSnapshot`, `readRegisterRaw`, `BusOperations`, OPT4001 clear-on-read FLAGS and test CLI dump behavior. Recheck source ledger access and model-dependent 0x07 (TMP116 EEPROM3 / TMP117–TMP119 Temp_Offset).

## Complete implementation work

- Implement the stage 8 bound-only diagnostic route: validated binding permits allowed register reads without initialization or any implicit CONFIG write. Keep health-neutral diagnostics, observed raw state, unknown startup/sample validity and destructive-read effects explicit; do not claim managed configuration trust.
- Before a multi-register snapshot/dump or EEPROM snapshot, read the family's 0x0F ID and stop on mismatch/transport failure before touching CONFIG/TEMP/foreign status registers. Binding alone is not compatible-identity evidence. Do not add a foreign-device override or fallback probe. Explicit single-register diagnostic observations remain labelled unverified when identity is unavailable and cannot authorize mutation or freshness.
- Implement one health-neutral diagnostic read route with exact per-model address validation, read-to-clear disclosures and raw snapshots. Managed verification may use the same internal transport with tracked accounting; do not add a second public diagnostic API solely to change health counters.
- Diagnostic snapshots must record register read order and whether DATA_READY/alert events may have been consumed; no fake atomic multi-register snapshot or unbounded looping.
- First release has no public arbitrary register-data write API/CLI route. Mutations use typed guarded settings/offset/reset/acquisition operations only. The private transport/write policy still rejects DEVICE_ID, TEMP, EEPROM_UL and EEPROM word writes, and ordinary CONFIG writes reject reset/one-shot/reserved bits. Keep negative tests at that existing seam; do not create a raw-write escape merely to test it.
- Do not permit unsupported-family access by raw pointer escaping. Core diagnostics operate only on the bound address; address scans belong to the application and do not prove family or grade. A live diagnostic read can acknowledge status even when it writes no data register.
- Add a concise register dump format with unsigned hex, signed decoded units where known, source-model and status flag caveats.

- A dump reads only explicit documented pointers. Reject 0x09–0x0E and nonzero upper pointer bits instead of masking aliases or probing alternate-family IDs. Never use a burst through 0x07 or skip model checks. Include each R row's exact read effects and report unavailable fields rather than fabricating zero defaults.
- Every successful CONFIG observation feeds sticky alert and same-generation readiness bookkeeping even if the dump later fails. Ambiguous destructive reads mark possible evidence loss. Diagnostics do not independently claim fresh conversion and are blocked while a cooperative job is active or its result is retained, under the shared ownership contract.

## Verification and exit criteria

Tests exercise every register address and model, skipped holes, model-dependent 0x07, repeated clear-on-read reads, private write-policy denial, dirty evidence and no extra callbacks on forbidden operations.
Include a freshly bound, uninitialized transcript of identity, CONFIG, TEMP
and permitted EEPROM-shadow observations with zero data-register writes.
Verify unknown validity/trust is reported and no hidden `begin` occurs.
For a TMP114/TMP118 mismatch, a dump stops after 0x0F and never reads foreign
0x02 alert status. Include CRC-enabled TMP114 transport-failure/0xFFFF adverse
fixtures with no retry, while distinguishing documented bad-CRC behavior from
the unknown exact response to an omitted CRC byte.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_26.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
