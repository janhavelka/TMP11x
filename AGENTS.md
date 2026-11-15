# TMP11x repository conventions

These instructions apply to the whole repository. `AGENTS.md` is the single
source of project guidance; `CLAUDE.md` imports it.

## CI and local verification

- Work with CI in mind from the start: inspect the relevant workflows and checks
  before implementation, reuse their commands, and keep changes compatible with
  their platforms, pinned dependencies, generated files and test registration.
- Run focused local checks for each change. Reserve remote CI dispatch and waiting
  for major changes: substantial behavior/API changes, cross-module integration,
  persistence/security changes, build/toolchain/dependency/workflow changes, and
  release or major integration milestones. Judge impact, not just diff size.
- Small documentation, comment, formatting and low-risk isolated edits need
  appropriate local verification, not a CI run after every commit or push. Batch
  related work before requesting CI; do not repeatedly dispatch unchanged suites.
- For major changes, use existing CI and verify the exact final commit through
  terminal results; dispatch it if needed. Fix confirmed failures without weakening
  checks. Report checks not run, blocked or failed honestly; never claim an older
  green run verifies a new commit. Honor required merge/release checks and existing
  hooks; do not change workflow triggers or bypass checks to avoid CI.

For numbered prompt execution, follow the checkpoint policy in
[COMMON.md](docs/prompts/COMMON.md); it controls when the sequence runs its
comprehensive remote check. Keep automatic workflow triggers intact.

## Git workflow

- Before editing, inspect status, branch, HEAD, configured remote and upstream.
  Fetch/check incoming changes; fast-forward only when safe.
- Keep the whole promptset on its existing branch. Do not create a branch or
  worktree per prompt or agent unless the user asks.
- Assume concurrent agents. Preserve unrelated changes; never stash, discard,
  overwrite, or commit them. A dirty worktree alone is not a blocker.
- After relevant checks pass, stage only your own files or hunks, then commit
  and push each meaningful completed chunk, including multiple times within
  one prompt and before finishing. No further permission is needed.
- When using subagents, the lead owns shared contracts, integration and Git
  mutations. Assign disjoint write scopes or read-only reviews.
- Confirm synchronization with the intended repository's upstream. Report
  conflicts or unavailable access; never force-push to resolve them.
- Sibling repositories are read-only references. Inspect pinned clean sources
  or record local modifications separately; do not fetch into, edit, build,
  push to or dispatch CI on them. Put any needed separate clone outside this repo.

## Scope and navigation

- Work within the user's requested scope. An audit, preparation task or single
  prompt does not authorize executing the entire implementation sequence.
- Use [START_HERE](docs/prompts/START_HERE.md) for prompt execution and
  [PROGRESS](docs/prompts/PROGRESS.md) for current evidence. Read COMMON and the
  selected prompt before stage work. Verify actual artifacts; plans are not
  implemented behavior or passing tests.
- [SEQUENCE](docs/prompts/SEQUENCE.json) owns ordering, dependencies and artifact
  IDs; [HANDOFFS](docs/prompts/HANDOFFS.md) assigns producers and integration
  owners. Update connected contracts together when changing a handoff.
- Prefer small, direct changes to existing owners. Add abstractions only for
  a concrete current caller or test. Do not introduce speculative frameworks,
  placeholder drivers or successful no-op implementations.
- Use short, precise engineering language. Deliver behavior, meaningful public
  Doxygen, relevant tests and affected documentation together.

## Project guidance

- TMP11x targets TMP116/TMP116N, TMP117/TMP117M/TMP117N and TMP119, with TMP117
  as the primary intended device. TMP114/TMP118 and other similarly named
  families are outside this driver. Select the exact model from the BOM;
  compatible ID/revision bits do not prove an exact part or accuracy grade.
- Public headers belong in `include/TMP11x/`, implementation in `src/`.
  Keep the core standard C++17 with no Arduino, ESP-IDF, FreeRTOS, logging,
  allocation, tasks, sleeps, bus/pin ownership or hidden retries.
- Follow [CONVENTIONS](docs/prompts/CONVENTIONS.md) and
  [ENGINEERING](docs/prompts/ENGINEERING.md) for the typed Status/API, lifecycle,
  passive health, configuration trust and operation contracts. Count physical
  tracked transport attempts separately from precondition or logical failures.
- Applications own I2C initialization, pins, clocks, routing, serialization and
  recovery. Callbacks are synchronous, terminal, timeout-bounded and must not
  retain borrowed buffers or re-enter the instance. Public APIs are not
  internally thread-safe or ISR-safe; the owner serializes access.
- Binding, job admission, cancellation, result consumption and teardown are
  bus-silent. Explicit synchronous operations and owner-driven poll steps have
  bounded I/O and visible failure paths. Settings operations never replace the
  binding; recovery replays the owned profile without hidden reset or rollback.
- Treat temperature value, fresh-conversion evidence and configuration trust
  separately. CONFIG/TEMP reads can consume flags. Preserve dirty state after
  ambiguous writes and uncertain observations after destructive reads.
- Keep first-release EEPROM data words read-only, including their volatile
  shadows.
  Check startup readiness and EEPROM lock/busy before allowed volatile writes.
  No unlock/program/persistence path, general-call reset or SMBus alert-response
  traffic. TMP116 has no supported addressed reset or hardware offset; its
  `0x07` is EEPROM data. Apply model gates to every API, diagnostic and CLI route.
- One-shot and supported addressed reset are explicit operations, not replayable
  settings. Managed one-shot requires desired and verified shutdown. Observation
  firmware uses bind-only diagnostics without implicit initialization writes.
- Exact-part TI PDFs pinned in [the reference manifest](docs/reference/manifest.json)
  govern silicon facts. Recheck register masks, timing and side effects against
  those sources; retain contradictions and unknowns. [SILICON](docs/prompts/SILICON.md)
  and [FULL_COVERAGE](docs/prompts/FULL_COVERAGE.md) track the evidence and scope.
  Sibling implementations establish conventions, not this chip's protocol.
- Framework/board glue belongs in examples/adapters. Arduino and native ESP-IDF
  share one framework-neutral CLI under `examples/common/`; native IDF must
  not compile an Arduino facade. Keep examples and test doubles out of core.
- Once package metadata exists, `library.json` owns versioning. Use the
  repository's generator/check commands and keep `Version.h` checked in;
  do not hand-edit generated versions or copy another package's identity.
- On Windows use `scripts/pio.cmd` for the existing VS Code-managed PlatformIO.
  Do not use an unrelated executable on PATH or install another Core. Report a
  missing installation precisely and continue independent work.
- Run checks appropriate to the changed scope, using
  [VERIFICATION](docs/prompts/VERIFICATION.md) and the actual available tools.
  Preserve separate evidence for native tests, full MCU links, remote CI and
  physical fixtures. Do not invoke future tools or claim unavailable checks.
- Preserve source attribution and license notices for adapted MIT material.
  Vendor documents and incompatible reference code retain their own terms;
  keep URLs/revisions/hashes, exclude references from runtime packages and
  never copy GPL code into the MIT core.
