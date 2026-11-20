# Common execution contract (applies to every numbered prompt)

## Enter

For reference preparation, use prompts 1–3 as the evidence checklist. For a
promptset audit, review every requested prompt and its shared contracts without
executing the implementation tasks. Stop at the requested scope; do not silently
start the whole sequence.
Preparatory evidence may be reused later after checking its hashes/revisions.

1. Read root AGENTS, active prompt, its prerequisites/reports and relevant
   [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md),
   [EVIDENCE](EVIDENCE.md), [SILICON](SILICON.md),
   [FULL_COVERAGE](FULL_COVERAGE.md), [COVERAGE](COVERAGE.md),
   [CLI_CONTRACT](CLI_CONTRACT.md), [HANDOFFS](HANDOFFS.md), [SOURCE_MAP](SOURCE_MAP.md),
   [VERIFICATION](VERIFICATION.md) and [DELEGATION](DELEGATION.md).
2. Inspect Git branch/HEAD/upstream/dirty status. Preserve unrelated work and
   verify actual prerequisite artifacts/tests. `NOT_STARTED`, `PARTIAL`,
   `BLOCKED`, `COMPLETE` refer to stage ownership, not product qualification.
3. Recheck relevant `janhavelka` sibling repository HEAD/selected code and
   the exact-part TI source. Prefer existing clean local checkouts read-only;
   inspect remote HEAD without fetching into sibling repositories. Use an
   external clone if a different revision is needed. Record changed revisions
   and dirty files; never cite uncommitted code as an immutable commit.
   Verify acquired PDFs with `python scripts/check_references.py`; do not
   overwrite pinned bytes when a TI symlink publishes a newer revision.
   A secondary library's API name alone does not establish semantic equivalence.
4. At entry identify owned files, exact source evidence, tests, hardware
   availability and this stage's only delivered outcome.
5. Follow the `prompts` array in SEQUENCE for execution order. Check the active
   prompt's `Consumes` against actual producer artifacts and required evidence.
   Verify the underlying artifacts directly. If a producer's MCU link is
   blocked, retain that gate and consume only its verified source artifacts.

## Implement a complete work unit

- Verify addresses, register fields, reserved bits, reset domains, read/clear
  effects, timing and model applicability against original TI tables; record
  conflicts and next evidence instead of guessing.
- Build usable behavior, validation, failure/cancellation handling, focused
  tests, documentation/Doxygen and affected CLI/tool changes together. Do not
  leave production placeholders that return invented success.
- Update CLI/tool integrations in the same stage once those consumers exist.
  Before their producer stage, record a named forward owner and test existing
  policy/core seams instead; do not require future CLI/raw/job implementations
  to close an earlier stage. HANDOFFS identifies those integration boundaries.
- Preserve bus and instance ownership. A bus callback completes one physical
  bounded transfer. No I2C outside the owner, no locks held during conversion
  waits, no hidden retry, no bus-wide commands, no EEPROM unlock/program.
- A register-read-only session uses bus-silent binding and explicit diagnostics,
  including at firmware boot; it must not implicitly call a writing initializer.
  CONFIG/TEMP reads can still consume flags, so read-only is not side-effect-free.
- A write that may have reached hardware leaves state potentially dirty.
  Retain trusted versus observed state, failure evidence and operation identity.
  No silent reset/rollback or software-fabricated freshness.
- Apply the selected model's capability gates on all paths, including raw,
  diagnostic, CLI and cooperative routes; TMP116 `0x07` is EEPROM3, while
  TMP117/TMP119 `0x07` is temperature offset.
- Reconcile affected R/F/X rows in FULL_COVERAGE, including deliberate
  exclusions and application-owned electrical/thermal requirements. Unknown
  silicon behavior is an evidence obligation, not permission to invent a
  simulator rule. Full profile trust/recovery includes limits and supported
  offset as well as stable CONFIG fields.
- Use meaningful public Doxygen and readable source comments. Keep README,
  CHANGELOG (once created), feature matrix, architecture, example guides and
  tests current. Research stages need not update README/CHANGELOG/user guides;
  they create or update their named evidence artifacts, including stage 5's
  ledger validator and regression tests. Follow each stage's explicit outputs.

## Delegate / review

Delegate bounded disjoint read-only research, test implementation or independent
review when it reduces risk. The lead owns shared public API, state machine,
architecture, integration, exact-source decisions and all Git mutations. Have
independent reviewers challenge precision, EEPROM safety and destructive reads.
Do not require a specific model or paid account feature to do the work.

Stages of type `audit` require parallel subagent review. Re-read the specified
original prompts in full and have reviewers examine disjoint aspects of the
actual change. The lead verifies findings directly, fixes confirmed in-scope
gaps, repeats the affected verification and records the reviewed revisions,
findings, fixes and final results. If a fix invalidates evidence from another
stage, record the stale evidence and its owner in the handoff. If a run cannot
use parallel reviewers, record the outstanding review and keep the audit gate open.

## Verify / checkpoint

Run stage-specific focused tests, `python scripts/check_promptset.py`,
`git diff --check`, and other affected test/build/packaging checks. Report
actual command, tool version, working tree/commit and PASS/FAIL/NOT RUN/BLOCKED
rather than generic success claims. Changes to checker behavior also require
`python scripts/test_check_promptset.py`; reference changes require the offline
reference verifier; reference tooling changes also require
`python scripts/test_check_references.py`. Do not run implementation-dependent
commands before the relevant sources exist. Native simulator results do not prove
hardware or ESP-IDF linking; no hardware is *not* a software failure.
Once stage 5 creates `tools/check_register_ledger.py`, run it for changed ledger
facts/policies and their consumers; run its regressions with
`python tools/test_check_register_ledger.py`. Stage 10 registers both in the
host entrypoint named in `docs/testing.md`. Once stage 10 establishes that host test
entrypoint, register each later suite there, including CLI/runner tests from
37–39. An earlier stage's PASS does not cover subsequently introduced tests.

Update `docs/reports/prompt_N.md` with: input revisions and selected source
pages, exact work, accepted/rejected sibling patterns, tests/commands/results,
known gaps, current commit and upstream sync status. Update PROGRESS, coverage
and `docs/reports/index.md` at this checkpoint. A preparation report may be separate from stage
reports; distinguish complete intake from still-open stage obligations.
Record consumed/produced artifact IDs with actual paths, revision/check evidence
and next integration owner. Changes to public contracts or producer ownership
must update affected prompts, SEQUENCE, HANDOFFS and tests together.
Record the input/base SHA before committing; identify the resulting commit
in the handoff or a later checkpoint rather than inventing a self-referential
commit hash in a file being committed. Commit/sync completed work using the
existing current branch. On sync failure retain work and report the unsynced
SHA; do not force-push.

## Blocked tasks and the final verdict

A fixture grant given only in a user message applies while that message is
visible in the current task context. To persist the grant across sessions,
the user writes it into the authorization record at a path they name. Agents
never create or modify the authorization record. Read
the instruction/record itself and verify its scope: board/port, exact part/address,
bus/mux participants, upload and permitted observe/volatile/power-cycle actions.
An agent report, summary or prior-session paraphrase is never the source of a
grant; it may point to the original, which must be checked directly. A claim
that a record was approved is not approval evidence. If neither authorized
source can be read directly, physical work is BLOCKED. A valid record persists
across sessions within its scope; no expiry or repeat confirmation is required.
Do not infer a target from port enumeration or another fixture's grant. No
mandatory JSON format is required. Verify board/route/address and compatible identity
before a mutating campaign and again after reconnect/reset/rebind/mismatch;
ID bits alone do not prove exact BOM model. An unexplained target change stops
dependent work until its scope is resolved.
If essential target/scope information is missing, keep dependent physical work
BLOCKED and continue independent software work.

A blocked stage keeps its exact remaining scope and required evidence. In a
whole-set invocation, continue independent later work whose concrete inputs
are satisfied; return if the dependency becomes available. An absent physical
TMP116/TMP117/TMP119 fixture leaves stages 40–41 and audit 54 BLOCKED. Package,
CI and final-review work can proceed on its own prerequisites and report the
overall product as hardware-unqualified.
No stage can be marked COMPLETE with required owned work missing; a final
review/report can complete while reporting a PARTIAL overall qualification.

Remote CI: every stage before 43 in the execution sequence, including the
interleaved audits, skips deliberate remote dispatch, polling and waiting.
This overrides the generic major-change dispatch rule during this sequence.
Stage 10 creates local test scaffolding; stage 43 creates the initial workflow
and runs the comprehensive checkpoint. Preserve any automatic triggers and
required protection already present; automatic runs are not forbidden. If
stage 44 changes CI-covered inputs, execute the stage 43 checkpoint procedure
for the resulting SHA within stage 44 and record its terminal result or BLOCKED
before closing the final report. An older green SHA does not cover later edits.
