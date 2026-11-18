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
- Continuous-cycle tables specify nominal values, not maximum cycle duration. A bounded library deadline may use `max(nominal cycle with a labelled margin, derived active duration) + policy slack`; this is timeout policy, not a silicon maximum. Retain the TMP117 electrical row's one-shot condition when deriving a continuous estimate from it. Earliest polling and timeout are distinct. Default one-shot polling starts after the count-scaled single-conversion maximum, with sparse subsequent polls; this precision policy addresses TMP119 p.23's recommendation to avoid I2C during active conversion. Min/typ-based latency policies must be explicit and disclose that they can communicate during conversion; no schedule alone certifies accuracy or completion.
- For continuous-fresh blocking helpers/jobs, use one readiness poll per nominal CONV×AVG cycle by default, rounded up to the authoritative clock's resolution and paced from the baseline/last poll. A due time past the deadline expires without another read. This is configurable traffic policy, not a guaranteed quiet window or conversion maximum; active-limited continuous modes may never have standby. Untimed `tryReadFresh` is one finite attempt and its caller owns pacing. These rules limit only this driver's own polling, not other sensor/mux/bus traffic; retain TMP119 p.23's precision caveat.
- Record the TMP116 POR-busy contradiction (p.17 versus p.26/p.29). A clear busy flag alone cannot prove its reset-load finished. Define an explicit startup/reset timing origin and guard policy with stated uncertainty; approximate 1.5-ms POR and TMP117/TMP119 2-ms software-reset text are different conditions, not interchangeable guaranteed maxima.
- Include TMP117 p.7 `tRESET=1.5 ms` as TYP with no MAX. FIRST_ACK is the only startup policy: internally timestamp completion of the first successful compatible read in the current begin/recover operation when proof is absent. Reads are possible during load (TMP116 p.17, TMP117 p.18, TMP119 p.20), so this anchors a conservative wait but proves neither completed POR nor a guaranteed maximum. Require continuously stable power/no reset from that read through all dependent operations. Stage 8 defines the lifecycle and stage 19 clears runtime origin/proof after reported power loss/reset; ordinary operations reuse still-valid proof. No Config seed or caller timestamp is accepted.
- Describe precisely when CONFIG/TEMP reads clear readiness/alert, effects of diagnostics, and what evidence is required for `freshConversion`. Do not treat time elapsed alone as universal completion in continuous mode.
- Define startup invalidity `0x8000` until first completed conversion and what information proves completion. Preserve raw numeric representation independent of valid sample state.
- Create `docs/registers/behavior-vectors.json` containing independent temperature/count, threshold, timing and event traces with negative limits, sign/rounding, reset state and repeated reads. Include source pages, conditions and unknown assumptions. These are research inputs, not executed driver tests; stages 16–23 and 32 consume applicable vectors.

- Specify block accumulation, not a moving average: TEMP/DATA_READY update only when the full block completes. Provide traces for nonoverlapping blocks and shutdown during a partial block; stages 20/21/32 execute them. Do not invent the chip's internal rounding if not specified. Preserve all 32 CONV×AVG table entries separately from derived timing bounds.
- Maintain an explicit UNKNOWN list for same-value CONFIG writes, AVG/CONV changes mid-block, MOD readback during/after OS, MOD=11 writes during active OS, DATA_READY/alert-mode flag retention on SD entry, inverted therm limits, exact read-clear edge and TEMP byte latching during a conversion update. Therm HIGH changes at completed comparisons and survives an abort/shutdown while T/nA remains therm and no reset occurs (TMP116 p.16; TMP117 p.17; TMP119 p.19); POL and DR/Alert affect the pin, not this status flag. Corresponding ALERT-pin persistence in shutdown with unchanged routing/polarity is an inference, not explicit shutdown text. Do not make therm HIGH an adverse random-clear fixture. A two-byte transaction does not prove an undocumented latch.
- Freeze freshness boundaries: managed OS checks local desired shutdown before I2C, then 0x04, compares preflight CONFIG before any write, establishes SD, reads CONFIG to consume stale readiness, then triggers exactly once. Preflight drift rejects without CONFIG writes; a MOD=11 exception requires retained owned-trigger provenance. MOD readback alone proves neither completion nor abortion. All managed profile changes in continuous mode, including limits/routing-only changes, use SD, apply fields, clear stale readiness, enter desired CC, then verify stable fields by final CONFIG readback. This readback establishes the current-generation baseline without publishing consumed readiness, and retains alert events. Do not declare one discarded completion sufficient to resolve unknown mid-block behavior. Read-only try-fresh performs no implicit mode writes and requires clean verified owned settings. If no baseline exists under that trusted generation, its first read only establishes one and cannot publish fresh data; otherwise require new DATA_READY evidence after the baseline.
- TMP116 p.14 omits TMP117/TMP119's explicit comparison-after-averaging phrase. TEMP block completion is documented (p.11); comparator block-end timing is an inference for TMP116 and needs labelled fixtures/physical evidence. Preserve that distinction.
- Document non-atomic multi-register application and the unresolved offset-to-limit-comparator ordering on TMP117/119. Use the per-chip coverage notes; a self-authored fake cannot settle either question.
- Cover F13's reset/provenance boundary: EEPROM address reads observe volatile shadows, and reset/loading can replace those observations without proving factory identity or NIST certification. Carry unknown/nonfactory contents and reset invalidation through behavior vectors; no presumed unique-data pattern is a validity test.
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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_6.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
