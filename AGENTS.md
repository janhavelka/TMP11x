# TMP11x agent instructions

Applies to the full repository including prompt execution. This is the single
source of project instructions for Codex and Claude; keep `CLAUDE.md` as an
import, not a second rulebook. Read `docs/prompts/COMMON.md` and the selected
numbered prompt before stage work.

## Scope and source authority

- Determine whether the user requested preparation, one stage, or the whole
  sequence. Repository review/reference intake does not authorize implementing
  all 44 stages. Resume from actual artifacts and `docs/prompts/PROGRESS.md`.
- `docs/prompts/START_HERE.md` is the canonical execution guide;
  `docs/prompts/SEQUENCE.json` owns stage ordering, dependencies and declared
  producer/consumer IDs. `docs/prompts/HANDOFFS.md` explains usable outputs,
  later integration owners and conditional firmware/hardware gates.
- Exact-part TI PDFs pinned in `docs/reference/manifest.json` own silicon
  facts. Sibling implementations establish reusable conventions, not register
  behavior. Seed notes and public drivers must be checked against the PDFs.
- Use clean, pinned sibling sources or record local modifications separately.
  Reuse an existing local checkout read-only; check its remote HEAD without
  changing it. Put any new clone outside this repository.
- Preparation can supply evidence for stages 1–3. Mark a stage COMPLETE only
  when all its exit criteria are met; record partial work and the next gate.

## Git / teamwork

- At entry: inspect `git status`, branch, HEAD, configured remote and upstream;
  fetch/check for incoming changes when possible. Preserve all unrelated and
  concurrent changes. Never force-push, reset, discard or stash others' work.
- This initial execution remains on the current `main` branch (or the one
  checkout branch explicitly supplied by the user). No per-prompt branches or
  worktrees unless the user requests them.
- One lead agent owns shared API contracts, integration, reports and Git
  mutations. Subagents receive disjoint write scopes or read-only reviews.
- After a coherent scope passes relevant local checks, stage only owned hunks,
  commit and push to the configured **new TMP11x** remote if authorized.
  Verify synchronization; otherwise report precisely why push is unavailable.
- Sibling repositories are **read-only**. Never edit, push to, dispatch CI on,
  or merge into TMP1x2, OPT4001, ADS1115, 24Cxx, FieldCore-node, etc.

## CI and verification

- Inspect current workflows/source before changing them. Run focused local
  checks per stage; do not repeatedly dispatch remote CI for minor changes.
- For this initial promptset, stage 43 owns the single comprehensive remote-CI
  checkpoint. Keep automatic triggers; run local builds/tests at all relevant
  earlier stages. A later source edit is not covered by an older green commit.
- Treat host tests, full linked MCU builds, remote CI and physical HIL as
  **different** evidence layers. Mark unavailable checks `NOT RUN` or `BLOCKED`.
- On Windows reuse VS Code-managed PlatformIO via `scripts/pio.cmd` once
  established from sibling precedent. Do not use an unrelated `pio` on PATH
  or install a second PlatformIO Core.
- Available preparation checks: `python scripts/check_promptset.py`,
  `python scripts/test_check_promptset.py`, `python scripts/check_references.py`,
  `python scripts/test_check_references.py` and `git diff --check`.
  Firmware/C++ gates start when their sources exist.

## Engineering boundaries

- Core: C++17, portable `include/TMP11x/` and `src/` with no Arduino/ESP-IDF,
  FreeRTOS, logging, heap allocation, tasks, sleeps, pin/bus ownership, retries
  or hidden general-call reset.
- One owner serializes instance state and shared I2C; callback attempts are
  synchronous, terminal, timeout bounded, pointer-write/repeated-START/read.
- Stable names: `TMP11x`, `Config`, `Status`/`Err`, `DriverState`,
  `HealthSnapshot`, `Sample`, `begin`, `bind`, `recover`, `start*`, `poll`,
  `cancel`, `takeResult`; exact final signatures belong to the API contract.
- The **selected BOM model** determines features. Device ID does not prove
  TMP117 versus TMP119 if the documented distinguishing bits are revision bits.
- Do not equate a temperature register read with fresh conversion. CONFIG and
  TEMP reads can clear DATA_READY. Observe EEPROM lock/busy and startup/reset
  timing before writes; TMP116's documented startup busy behavior is contradictory.
- One-shot and reset are explicit operations, not persistent settings replayed
  by initialization/recovery. Read-only firmware must use bind-only diagnostics
  without initialization writes; it still reports destructive-read effects.
- First-release nonvolatile behavior is read-only; no EEPROM unlocking,
  reprogramming, startup persistence or factory traceability modification.
- Verify literal register addresses, mask/writes, conversion timing and exact
  variant differences against the applicable TI datasheet. No guesswork.
- Prefer small, direct code over speculative frameworks. New public behavior
  must have tests and meaningful Doxygen on the same stage.
- Keep framework/board code in examples/adapters. A shared CLI belongs in
  `examples/common/`; native ESP-IDF must not compile an Arduino facade.
- Once created at stage 10, `library.json` owns versioning. Generate checked-in
  `Version.h`; do not copy a sibling's package identity or hand-edit the version.
- Use short, precise, understandable engineering language. Preserve source
  attribution and license notices for adapted MIT code. Vendor documents and
  incompatible reference code retain their own terms; exclude references from
  runtime packages, and never copy GPL code into the MIT core.
