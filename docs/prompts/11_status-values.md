# 11 — Implement portable Status, model and value types

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [10](10_package-foundation.md).

## Objective and boundaries

Deliver robust pure types and local validation that define exact, stable source-independent contracts.

## Source audit

Read TMP1x2 `Status.h`, `Config.h`, `TMP1x2.h` structs, `test/helpers/test_helpers.cpp`; verify no enum ordinal ABI is assumed across packages. Recheck TMP11x variant/ID ledger.

## Complete implementation work

- Implement `Status`, `Err`, `errorName`, `DriverState`, `Model`, relevant capability selectors and typed `Config` without Arduino/ESP-IDF dependencies. Messages have static lifetime.
- Implement pure model/address validity, capability `hasOffset`, EEPROM word count, documented supported addresses 0x48–0x4B, rate/average enum bounds and chosen precision/value helper types.
- Define `Sample` and `IdentityInfo` with raw register, exact signed counts, °C, timestamp validity, freshness proof and selected BOM model (not fabricated auto-discovery).
- Add public Doxygen with error meanings, units, input/output semantics and invalid-model behavior. Reject unsupported features with meaningful status, not default-success placeholders.

## Verification and exit criteria

Native tests cover every valid/invalid model, address boundaries, no heap requirement, `Status` formatting and header-only inclusion. Build with strict warnings and test assertions active.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_11.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
