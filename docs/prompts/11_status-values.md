# 11 — Implement portable Status, model and value types

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [10](10_package-foundation.md).
Produces: `value-types`.
Consumes: `api-contract`, `owner-contract`, `build-foundation`, `variant-contract`, `register-ledger`.

## Objective and boundaries

Deliver robust pure types and local validation that define exact, stable source-independent contracts.

## Source audit

Read TMP1x2 `Status.h`, `Config.h`, `TMP1x2.h` structs, `test/helpers/test_helpers.cpp`; verify no enum ordinal ABI is assumed across packages. Recheck TMP11x variant/ID ledger.

## Complete implementation work

- Implement `Status`, `Err`, `errorName`, `DriverState`, `Model`, relevant capability selectors and typed `Config` without Arduino/ESP-IDF dependencies. Messages have static lifetime.
- Implement pure model/address validity, capability `hasOffset`, EEPROM word count, documented supported addresses 0x48–0x4B, conversion-cycle/average enum bounds and chosen precision/value helper types. Model CONV as a cycle setting whose effective period also depends on AVG, not TMP1x2's independent conversion-rate enum.
- Own the single pure model/capability table used by stage 13's identity engine and later gates. Identity matching must consume this table, not introduce a second competing capability implementation.
- Implement the current Config with binding and stable CONFIG/limit settings. Stage 23 introduces its configurable offset member/control and corresponding trust fields; do not expose an accepted numeric default-zero request before its apply engine exists. Hardware `hasOffset` describes the model capability, not proof that the staged API has already implemented its control.
- Include stage 8's `StartupOrigin` seed in Config: FIRST_ACK default with ignored timestamp, or CALLER_TIMESTAMP in the authoritative clock epoch. Validate the source enum and representation locally; timed startup validates timestamp recency/future ambiguity. This seed is device-state metadata, excluded from immutable transport-binding comparisons and settings replay. Stage 14 copies it; stages 18/19 establish or invalidate the evidence.
- Define `Sample` with raw temperature, exact signed counts, degrees Celsius, timestamp validity, acquisition kind, profile generation, configuration trust and freshness evidence; reserve the stage 8 offset-provenance representation as unknown until stage 23 populates it. Define `IdentityInfo` separately with raw identity, documented family/revision interpretation and selected BOM model. A decoded sample value alone conveys neither startup validity nor proof of a new conversion.
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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_11.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
