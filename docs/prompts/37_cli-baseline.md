# 37 — Create one shared bounded diagnostic CLI engine

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [27](27_sync-helpers-cache.md), [28](28_cooperative-admission.md), [30](30_cooperative-measurement.md), [33](33_native-suite.md).
Produces: `cli-baseline`, `cli-protocol`.
Consumes: `sync-api`, `job-lifecycle`, `measurement-jobs`, `native-qualification`, `host-test-entrypoint`, `version-tools`.

## Objective and boundaries

Unify Arduino, native IDF and compiled native CLI behavior through a single framework-neutral command processor. Attach the stage 34/35 adapters when their source inputs exist; an unavailable SDK must not block the host engine/runner. Keep unverified framework links open in their owning stages rather than claiming them from host tests.

## Source audit

Inspect TMP1x2 `examples/common/Tmp1x2Cli.cpp/.h`, CLI output and diagnostics source, TCA9548A read-only commands, and DP8382x `docs/prompts/CLI_CONTRACT.md`; use this promptset CLI contract as the selected boundary.

## Complete implementation work

- Build one reusable parser/dispatcher under `examples/common/`; neither firmware backend may create a second command parser. No `String`, uncontrolled heap allocations or unbounded line buffering.
- Implement the canonical observation commands from [CLI_CONTRACT](CLI_CONTRACT.md), including `read`, `latest` and `caps`; do not invent competing `temp`/`fresh` names. `read` observes TEMP without claiming a new conversion; cached `latest` and model capabilities are bus-silent. CONFIG/TEMP diagnostics disclose read-to-clear effects. One-shot triggering belongs to the explicit mutating route in stage 38.
- Define deterministic tagged records for runner correlation, bounded output, invalid/malformed input errors, no-echo/quiet behavior, and consistent status/operation names. A line that overflows or loses bytes must be rejected through its next delimiter; never dispatch a valid-looking prefix or suffix of a truncated command. Output loss must be observable so the runner cannot accept a partial success record.
- Produce `docs/cli-protocol.md` plus checked-in golden transcripts under `test/cli/`. Freeze protocol version, complete command grammar, record schema/size bounds, firmware versus library identity, boot/session/request identity, restart detection, unknown/null fields, and request acceptance versus operation completion. `job start` acknowledgement is not a completed measurement; terminal request and operation records must be distinguishable. Bus-silent commands never probe the sensor just to fill an unknown ID field.
- Keep console processing separate from the transport; application adapters own `Wire`, `Serial`, IDF USB/UART tasks and output flow control.
- Add a host compiled CLI fixture driven by scripted fake transport, with parser tests and help for implemented observation routes. The complete grammar is frozen here, but stage 38 implements mutation/jobs and the final help matrix; unimplemented commands must not advertise successful operation. Test bus-silent binding/startup and read-only EEPROM/observation commands before initialization. Host tests require no MCU SDK or hardware.
- Register the fixture/parser/golden-transcript tests in the standard host entrypoint from stages 10/33 and update `docs/testing.md`. Stage 38 completes firmware source wiring and fresh link evidence; stage 39 consumes this exact protocol and fixture rather than creating an unrelated transcript generator.

## Verification and exit criteria

Check fuzzed line lengths, split input, no terminal newline, invalid model/enum
and quiet-batch overflow in the compiled host fixture. Stage 38 executes live
job/command parity tests once those routes exist. Check available framework
source adapters now; missing sources/SDKs leave explicit stage 38 integration
or target-link gates, not a dependency on both frameworks for the host engine.
No hardware activity is required.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_37.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
