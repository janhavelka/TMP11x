# 37 — Create one shared bounded diagnostic CLI engine

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [27](27_sync-helpers-cache.md), [28](28_cooperative-admission.md), [30](30_cooperative-measurement.md), [34](34_arduino-examples.md), [35](35_idf-examples.md).

## Objective and boundaries

Unify Arduino, native IDF and compiled native CLI behavior through a single framework-neutral command processor.

## Source audit

Inspect TMP1x2 `examples/common/Tmp1x2Cli.cpp/.h`, CLI output and diagnostics source, TCA9548A read-only commands, and DP8382x `docs/prompts/CLI_CONTRACT.md`; use this promptset CLI contract as the selected boundary.

## Complete implementation work

- Build one reusable parser/dispatcher under `examples/common/`; neither firmware backend may create a second command parser. No `String`, uncontrolled heap allocations or unbounded line buffering.
- Establish read-only commands `help`, `info`, `identity`, `temp`, `latest`, `fresh`, `status`, `caps`, `config`, `eeprom`, `dump` with explicit per-model capability and side-effect labels.
- Define deterministic tagged records for runner correlation, bounded output, invalid/malformed input errors, no-echo/quiet behavior, and consistent status/operation names.
- Keep console processing separate from the transport; application adapters own `Wire`, `Serial`, IDF USB/UART tasks and output flow control.
- Add a host compiled CLI fixture driven by scripted fake transport, with parser tests and exact complete help matrix; `eeprom` commands must remain read-only.

## Verification and exit criteria

Check fuzzed line lengths, split input, overlapping pending operations, no terminal newline, invalid model/enum, quiet-batch overflow and exact command parity on both framework examples. No hardware activity is required.

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
