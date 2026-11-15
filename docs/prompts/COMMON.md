# Common execution contract (applies to every numbered prompt)

## Enter

For a preparation/review request, use prompts 1–3 as the evidence checklist
and stop at the requested scope. Do not silently start the whole sequence.
Preparatory evidence may be reused later after checking its hashes/revisions.

1. Read root AGENTS, active prompt, its prerequisites/reports and relevant
   [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md),
   [EVIDENCE](EVIDENCE.md), [SILICON](SILICON.md),
   [FULL_COVERAGE](FULL_COVERAGE.md), [SOURCE_MAP](SOURCE_MAP.md),
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

## Implement a complete work unit

- Verify addresses, register fields, reserved bits, reset domains, read/clear
  effects, timing and model applicability against original TI tables; record
  conflicts and next evidence instead of guessing.
- Build usable behavior, validation, failure/cancellation handling, focused
  tests, documentation/Doxygen and affected CLI/tool changes together. Do not
  leave production placeholders that return invented success.
- Preserve bus and instance ownership. A bus callback completes one physical
  bounded transfer. No I2C outside the owner, no locks held during conversion
  waits, no hidden retry, no bus-wide commands, no EEPROM unlock/program.
- A write that may have reached hardware leaves state potentially dirty.
  Retain trusted versus observed state, failure evidence and operation identity.
  No silent reset/rollback or software-fabricated freshness.
- Apply the selected model's capability gates on all paths, including raw,
  diagnostic, CLI and cooperative routes; TMP116 `0x07` is EEPROM3, while
  TMP117/TMP119 `0x07` is temperature offset.
- Use meaningful public Doxygen and readable source comments. Keep README,
  CHANGELOG, feature matrix, architecture, example guides and tests current.

## Delegate / review

Delegate bounded disjoint read-only research, test implementation or independent
review when it reduces risk. The lead owns shared public API, state machine,
architecture, integration, exact-source decisions and all Git mutations. Have
independent reviewers challenge precision, EEPROM safety and destructive reads.
Do not require a specific model or paid account feature to do the work.

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

Update `docs/reports/prompt_N.md` with: input revisions and selected source
pages, exact work, accepted/rejected sibling patterns, tests/commands/results,
known gaps, current commit and upstream sync status. Update PROGRESS and
coverage at this checkpoint. A preparation report may be separate from stage
reports; distinguish complete intake from still-open stage obligations.
Record the input/base SHA before committing; identify the resulting commit
in the handoff or a later checkpoint rather than inventing a self-referential
commit hash in a file being committed. Commit/sync completed work using the
existing current branch. On sync failure retain work and report the unsynced
SHA; do not force-push.

## Blocked tasks and the final verdict

A blocked stage keeps its exact remaining scope and required evidence. In a
whole-set invocation, continue independent later work whose concrete inputs
are satisfied; return if the dependency becomes available. An absent physical
TMP116/TMP117/TMP119 fixture leaves stages 40–41 BLOCKED; software stages 42–44
can still complete and report the overall product as hardware-unqualified.
No stage can be marked COMPLETE with required owned work missing; a final
review/report can complete while reporting a PARTIAL overall qualification.

Remote CI: for the first sequence, only prompt 43 deliberately triggers/checks
the comprehensive remote CI checkpoint, unless required automated triggers
already run on push. Existing automation triggers and protection remain intact;
do not dispatch or poll them after each minor prompt. If final-audit fixes
change CI-covered inputs, reopen checkpoint 43 for the new SHA rather than
claiming the earlier run covers those changes.
