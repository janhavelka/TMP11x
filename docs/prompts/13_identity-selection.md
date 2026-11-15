# 13 — Implement identity validation and explicit model capabilities

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [4](4_family-identity.md), [5](5_register-ledger.md), [8](8_api-architecture.md), [11](11_status-values.md), [12](12_i2c-register-transport.md).
Produces: `identity-engine`.
Consumes: `variant-contract`, `register-ledger`, `value-types`, `register-transport`.

## Objective and boundaries

Prove the compatible device family without pretending TMP117 versus TMP119 grade is uniquely encoded in the revision nibble.

## Source audit

Check exact TI ID sections, documented revision fields, TMP119 contradictory default table, Linux driver device matches; inspect OPT4001 identity APIs and exact-bits native tests.

## Complete implementation work

- Implement the private identity read/match engine and selected-model validation over stage 12's binding view, reading 0x0F with no data-register write. Retain raw ID, recognized group, revision field, selected BOM model and confidence/ambiguity. Stage 14 connects public `readIdentity`/`probe` to real binding; test the engine directly here without an initializer or fake lifecycle.
- TMP116 group expects all 16 documented ID bits 0x1116; it has no separately documented revision field. TMP117 and TMP119 accept verified base DID[11:0] `0x117` with revision separately recorded; do not reject an otherwise compatible 117-family response solely for a different revision nibble. Do not extrapolate that policy to undocumented TMP116 IDs or assert accuracy grade from an address ACK.
- Reject incompatible family IDs, malformed register reads, unsupported addresses and mismatched selected model/group. Distinguish transport errors from valid but unexpected identity.
- Specify `probe` as health-neutral diagnostics for stage 14's public route and stage 15's accounting integration. Discovery remains application-owned and never mutates hardware.
- Provide tests for known 0x1116, 0x0117, 0x2117 examples, alternate 117-family revision nibble, wrong full TMP116 ID and same-bus unrelated responders. The selected model gates capabilities after compatible-family validation; a compatible numeric response is not unique proof of physical part identity.

- Add named TMP114 and TMP118 negative responders from the [exclusion audit](../reference/extracted/excluded-families-audit.md). A mismatch at 0x0F terminates validation without data writes, alternate ID scans or the TMP118 UID unlock-like write procedure. Their different ID pointers/bit layouts must not be accepted as related revisions. If unrelated hardware happens to return compatible data, retain the documented family-confidence limitation.

## Verification and exit criteria

A strong independent reviewer challenges any claim that raw 0x2117 proves actual TMP119; source contradiction is retained in docs.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_13.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
