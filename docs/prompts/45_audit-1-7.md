# 45 — Audit preparation and silicon evidence (prompts 1–7)

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Independent review**. Type: **audit**.
Prerequisites: [1](1_ecosystem-core-audit.md), [2](2_tooling-peer-audit.md), [3](3_ti-reference-intake.md), [4](4_family-identity.md), [5](5_register-ledger.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md).
Produces: `audit-1-7`.
Consumes: `ecosystem-audit`, `tooling-plan`, `ti-archive`, `variant-contract`, `board-guide`, `register-ledger`, `feature-ledger`, `register-validator`, `behavior-contract`, `behavior-vectors`, `safety-policy`.

## Objective and boundaries

Re-read original prompts 1–7 in full and perform a fresh,
independent audit of the completed work.

## Source audit

Check pinned-source provenance, model applicability, every register and
side-effect claim, literal timing vectors, and the EEPROM/reset exclusions.

## Complete implementation work

Use subagents in parallel to inspect requirements coverage, implementation
correctness, edge cases, tests, scope, and code simplicity. Verify their
findings against the actual artifacts, code and diff. Use original sources
and tests to confirm each finding independently of earlier summaries.

Fix every confirmed gap or bug within the original scope. Prefer simple,
readable corrections and remove unnecessary complexity. Update affected
tests, documentation, and handoffs where the fixes require it.

## Verification and exit criteria

Repeat the required verification from the audited prompts and continue until
the completed work satisfies their requirements. Record any blocked check
with the specific missing evidence and retain its original qualification gate.

## Documentation and delegation

Use [DELEGATION.md](DELEGATION.md) for disjoint, read-only reviewer briefs.
The lead reconciles findings, makes scoped fixes, and owns all Git changes.

## Checkpoint and report

Finish with a concise report in `docs/reports/prompt_45.md` covering audit
findings, fixes made, and final verification results. Record reviewed revisions
and any remaining evidence gates. Follow COMMON for the report index, progress
and commit checkpoint.
