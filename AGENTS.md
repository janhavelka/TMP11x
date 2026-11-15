# TMP11x agent instructions

Applies to the full repository including prompt execution. Read
`docs/prompts/COMMON.md` and the selected numbered prompt before work.

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
  established from sibling precedent. Do not install a second PlatformIO Core.

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
  TEMP reads can clear DATA_READY. Observe EEPROM lock/busy before writes.
- First-release nonvolatile behavior is read-only; no EEPROM unlocking,
  reprogramming, startup persistence or factory traceability modification.
- Verify literal register addresses, mask/writes, conversion timing and exact
  variant differences against the applicable TI datasheet. No guesswork.
- Prefer small, direct code over speculative frameworks. New public behavior
  must have tests and meaningful Doxygen on the same stage.
- Use short, precise, understandable engineering language. Preserve source
  attribution and license notices for adapted MIT code.
