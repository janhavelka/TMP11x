# 55 — Audit package and documentation (prompt 42)

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Independent review**. Type: **audit**.
Prerequisites: [42](42_package-docs.md).
Produces: `audit-42`.
Consumes: `package-qualification`, `package-doc-tools`.

## Objective and boundaries

Re-read original prompt 42 in full and perform a fresh,
independent audit of the completed work.

## Source audit

Check installed and relocated consumers, package exclusions and dependency
independence, generated version and license, meaningful Doxygen and
reproducible user instructions against actual test and build evidence.

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

Finish with a concise report in `docs/reports/prompt_55.md` covering audit
findings, fixes made, and final verification results. Record reviewed revisions
and any remaining evidence gates. Follow COMMON for the report index, progress
and commit checkpoint.
