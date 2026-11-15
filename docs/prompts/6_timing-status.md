# 6 — Verify conversion, averaging, freshness and alert timing

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [3](3_ti-reference-intake.md), [5](5_register-ledger.md).
Produces: `behavior-contract`, `behavior-vectors`.
Consumes: `register-ledger`, `register-validator`, `ti-archive`.

## Objective and boundaries

Establish precise conversion scheduling and read-to-clear rules for all supported parts before implementing sensor reads.

## Source audit

Revisit the inaccessible TI E2E DATA_READY race lead recorded in the
[intake notes](../reference/extracted/intake-notes.md). The thread title is
not verified silicon behavior. Resolve any relevant race using accessible
primary evidence and record remaining uncertainty before defining freshness.

Inspect TMP116/TMP117/TMP119 conversion/averaging tables, start-up EEPROM latency, DATA_READY and ALERT mode descriptions. Compare TMP1x2 `docs/owner-operations.md`, OPT4001 ready/counter behavior and their test clocks; no TMP1x2 35-ms EM wait is reusable.

## Complete implementation work

- Create `docs/device-behavior.md`: documented conversion mode state transitions, conversion/standby time, averaging count, one-shot trigger/completion and separate earliest-poll time, expected duration and operation timeout. Record MIN/TYP/MAX and conditions. TMP116 single-conversion timing is 13.5/15.5/17 ms (p.5); TMP117 is 13/15.5/17.5 ms (p.6), and TMP119 is 13/15.5/17.5 ms without averaging (p.7). Derive averaged bounds explicitly; rounded 125/500/1000-ms table values are not automatically worst-case deadlines.
- Distinguish continuous cycle period from standby time: the cycle tables include active averaging time, and there is no standby when averaging fills/exceeds the selected period. One-shot duration depends on AVG, not CONV. TMP116 p.14 specifies one-shot entry from shutdown; use a documented shutdown-to-one-shot sequence rather than assuming an arbitrary mode rewrite is a safe trigger.
- Record the TMP116 POR-busy contradiction (p.17 versus p.26/p.29). A clear busy flag alone cannot prove its reset-load finished. Define an explicit startup/reset timing origin and guard policy with stated uncertainty; approximate 1.5-ms POR and TMP117/TMP119 2-ms software-reset text are different conditions, not interchangeable guaranteed maxima.
- Describe precisely when CONFIG/TEMP reads clear readiness/alert, effects of diagnostics, and what evidence is required for `freshConversion`. Do not treat time elapsed alone as universal completion in continuous mode.
- Define startup invalidity `0x8000` until first completed conversion and what information proves completion. Preserve raw numeric representation independent of valid sample state.
- Create `docs/registers/behavior-vectors.json` containing independent temperature/count, threshold, timing and event traces with negative limits, sign/rounding, reset state and repeated reads. Include source pages, conditions and unknown assumptions. These are research inputs, not executed driver tests; stages 16–23 and 32 consume applicable vectors.

- Specify block accumulation, not a moving average: TEMP/DATA_READY update only when the full block completes. Provide traces for nonoverlapping blocks and shutdown during a partial block; stages 20/21/32 execute them. Do not invent the chip's internal rounding if not specified. Preserve all 32 CONV×AVG table entries separately from derived timing bounds.
- Maintain an explicit UNKNOWN list for same-value CONFIG writes, AVG/CONV changes mid-block, shutdown status retention, exact read-clear edge and TEMP byte latching during a conversion update. A two-byte transaction does not prove an undocumented latch. Define conservative profile/freshness policy and an evidence gate for stronger continuous-read coherence claims; do not invent a double-read workaround.
- Document non-atomic multi-register application and the unresolved offset-to-limit-comparator ordering on TMP117/119. Use the per-chip coverage notes; a self-authored fake cannot settle either question.
- Separate the 20–40 ms hardware serial-interface timeout (35 ms typical prose, line-held/no-clock condition) from conversion duration, callback timeout and whole-operation deadline. It is not a universal maximum total transfer length.

## Verification and exit criteria

Review timing/wait start events with a second agent where possible. Validate each literal against TI source; no assumptions from old TMP1x2 conversion-rate enums. Update the stage 5 ledger with resolved timing/read-effect metadata and run its validator. Driver execution is not an exit gate before stage 10 exists.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_6.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
