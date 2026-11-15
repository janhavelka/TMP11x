# 38 — Add full safe configuration, job and diagnostic CLI routes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md), [25](25_per-device-reset.md), [26](26_diagnostic-access.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md), [30](30_cooperative-measurement.md), [31](31_failure-recovery.md), [37](37_cli-baseline.md).
Produces: `cli-complete`, `firmware-cli-handoff`.
Consumes: `cli-baseline`, `cli-protocol`, `limits-alert`, `offset-feature`, `readonly-eeprom`, `device-reset`, `diagnostics`, `configuration-jobs`, `measurement-jobs`, `fault-closure`.

## Objective and boundaries

Expose the actually implemented TMP11x chip functions through the CLI without introducing a bypass for EEPROM safety or global reset.

## Source audit

Review current public API and tests, TMP1x2 shared CLI measured one-shots/owner-mode controls, OPT4001 flag read-to-clear command warnings and the promptset `CLI_CONTRACT` source.

## Complete implementation work

- Add the typed mutation and owner-job routes in [CLI_CONTRACT](CLI_CONTRACT.md): use canonical `avg` and `limits`, with alert polarity/pin-function options and `job` subcommands defined once. Do not introduce competing `average`/`threshold` spellings or an uncontracted `stress` route. Gate every command by implemented capability and document bus-silent local commands separately from register writes.
- Every device-mutating CLI route calls the library public API; **no CLI-only register writer** can open EEPROM_UL or ignore the busy/lock guard. Do not invent a reset command until stage 25 source proof permits it.
- Render raw/cached/observed values and timestamp validity distinctly, including readiness flag consumption and one-shot evidence. Do not show a newly completed measurement from a stale data-ready read.
- Preserve one owner token/result, per-transfer budgets, cancellation and status diagnostics from library, not synthetic CLI state. Include error reasons and safe no-I/O refusals. Explicit setting commands leave the requested settings applied; only temporary diagnostic campaigns capture and restore a baseline. Restoration cannot replay raw status/self-clearing bits or guarantee reconstruction of consumed flags.
- Register native CLI tests and shared grammar checks so Arduino and IDF use the same exact commands and help definitions. A finite `watch` observes existing conversions by default; any one-shot-triggering mode requires the explicit mutating option and its own deadline/cancellation behavior.
- Enforce the shared one-shot policy: desired continuous mode rejects without I2C; the caller explicitly requests shutdown before one-shot or one-shot watch. Do not silently change the persistent profile or restore a previous mode inside the command.
- Complete stage 37's grammar/help and register every command/job test in the standard host suite. Integrate the shared processor, final routes and application identity/session fields into available stage 34/35 firmware sources. Rebuild affected targets when their toolchain exists; preserve pending source/link gates when unavailable without blocking the host CLI work.
- Produce `docs/firmware-cli-handoff.md` listing each backend/target, final protocol version, source/build/image identity and current full-link result or exact missing gate. A pre-37/38 example build is not a compatible runner image. Stage 40 requires one actual matching image from this handoff; stage 42/43 must refresh links if later changes invalidate them.

- Reconcile R/F/X coverage against canonical commands and output fields: all stable settings, both limits, supported offset, all permitted shadow words, identity, status and profile trust need a route. Excluded ARA/broadcast/persistence/alternate-family UID procedures must have no command/raw escape. Report unsupported/unknown explicitly; do not infer electrical grade or factory serial validity.

- Register CLI forbidden-pattern tests for general-call 0x00, ARA 0x0C, EEPROM_UL writes, EEPROM-shadow writes and alternate-family UID procedures. Exercise malformed/raw inputs and verify that no route bypasses typed capability or safe-write guards.

## Verification and exit criteria

Run negative/positive scenarios for every command; test no hidden EEPROM unlock/write, no unbounded watch, disallowed model operations, stale sample age, invalid arguments and terminal result ordering.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_38.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
