# 46 — Audit API and owner contracts (prompts 8–9)

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Independent review**. Type: **audit**.
Prerequisites: [8](8_api-architecture.md), [9](9_owner-deadline-contract.md).
Produces: `audit-8-9`.
Consumes: `api-contract`, `owner-contract`.

## Objective and boundaries

Re-read original prompts 8–9 in full and perform a fresh,
independent audit of the completed work.

## Source audit

Check public API completeness, model gates, transport ownership, destructive
reads, operation deadlines, cancellation, and exactly-once results against the
silicon evidence.

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

Finish with a concise report in `docs/reports/prompt_46.md` covering audit
findings, fixes made, and final verification results. Record reviewed revisions
and any remaining evidence gates. Follow COMMON for the report index, progress
and commit checkpoint.
