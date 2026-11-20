# 44 — Perform independent silicon/API/safety/consumer closure

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Package, CI and final review**. Type: **audit**.
Prerequisites: [31](31_failure-recovery.md), [32](32_variant-model-fixtures.md), [33](33_native-suite.md), [38](38_cli-device-operations.md), [39](39_python-hil-runner.md), [42](42_package-docs.md), [55](55_audit-42.md).
Produces: `final-audit`.
Consumes: `fault-closure`, `variant-fixtures`, `native-qualification`, `cli-complete`, `hil-runner`, `package-qualification`, `feature-ledger`, `audit-42`.

## Objective and boundaries

Re-read original prompts 1–43 in full using the [ordered index](README.md)
and perform a fresh, independent audit of the completed work. Close software
scope with precise evidence-based status by exact variant.

## Source audit

Trace original TI PDF register fields and side effects against actual production
source, literal tests and package consumers. Recheck TMP1x2 public naming and
ownership and the accepted TMP11x requirement matrix. Review available stage 43
CI evidence and stage 40/41 physical evidence, including stage 54 findings if
available. Apply COMMON's separate evidence gates to final qualification.

## Complete implementation work

Use subagents in parallel to inspect requirements coverage, implementation
correctness, edge cases, tests, scope, and code simplicity. Verify their findings
against the actual code, diff, original sources and tests independently of
earlier summaries. Fix every confirmed gap or bug within the original scope.
Prefer simple, readable corrections and remove unnecessary complexity.

- Review device ID/revision policy, TMP116 0x07 versus TMP117/TMP119 0x07, EEPROM lock/busy and blocked programming, DATA_READY/ALERT destructive reads, threshold order, signed temperature arithmetic and 32-bit timing wrap.
- Reconcile the canonical FULL_COVERAGE evidence columns row-by-row for TMP116/TMP116N, TMP117/M/N and TMP119, including stage 36's P4 evidence or recorded limitation. Software/hardware suffix grades are not equivalent to individually measured accuracy qualifications. EEPROM traceability content and a readable ID do not establish calibrated system accuracy.
- Independently test failed/ambiguous I2C writes, late one-shot, staged config cancellation, simultaneous sensor instances, fresh sample claims, unsupported operation early refusal and exported package independence. Review actual API/CLI/Python parity, register-write-free observation startup and masked writable-field restoration; do not require restoration of consumed flags or unknown conversion history.
- Fix confirmed defects in their owner module with tests/docs, not by weakening error checks. Keep unresolved silicon contradictions and missing hardware/SDK/CI evidence explicit.
- If fixes change inputs covered by the completed CI checkpoint, execute stage 43's checkpoint procedure for the resulting SHA within this stage. Record its terminal CI result or precise BLOCKED status before closing the final report; an older green run does not validate changed source.
- Produce the single final report `docs/reports/prompt_44.md`, exact reproducible build/consumer instructions, git SHA/upstream verification and a prioritized finite list of residual obligations. Do not create a fictitious release tag or claim that TMP119 was electrically tested because TMP117 passed.

- Reconcile every R/F/X identifier and stage 5 split row against code, executing tests or its explicit integration/evidence disposition. Check every register bit/reset domain, pointer hole, full-profile replay, all timing/routing combinations, exact-grade conditions and excluded-family negative cases.
- Independently challenge source-unknown behaviors: no fake-derived proof of byte latching, conversion restart, atomic multi-register updates or offset/comparator ordering. Audit both sides of retained vendor conflicts and bound all claims to applicable source/fixture evidence. An intentional exclusion is visible scope, not a missing row or silent successful no-op.

## Verification and exit criteria

Repeat required verification and continue until the implementation satisfies
the original prompts. Run the affected focused and integrated software tests
and docs/package checks. Report the six canonical FULL_COVERAGE evidence columns
separately, with the exact SHA for remote CI. Overall status remains PARTIAL if
required physical or model evidence remains unavailable.

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
PASS/FAIL/NOT RUN/BLOCKED. Finish with a concise report in
`docs/reports/prompt_44.md` covering audit findings, fixes made, and final
verification results. Include sources, owned artifacts, named test cases and
open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
