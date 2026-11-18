# 20 — Implement continuous operation, averaging and cycle timing

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [6](6_timing-status.md), [8](8_api-architecture.md), [16](16_temperature-codec.md), [17](17_config-codecs.md), [18](18_safe-initialize.md), [19](19_config-trust.md).
Produces: `conversion-settings`.
Consumes: `behavior-contract`, `behavior-vectors`, `config-codec`, `safe-apply-engine`, `config-trust`, `register-fake`.

## Objective and boundaries

Provide fully typed mode/cycle/averaging controls and accurately report conversion time versus sample frequency and standby time.

## Source audit

Compare TI cycle and averaging timing tables for TMP116/TMP117/TMP119; inspect OPT4001 mode API, TMP1x2 rate setters and wrong extended-mode timing assumptions.

## Complete implementation work

- Implement exact per-model mode, conversion-cycle and averaging selectors, pure timing helpers and safe configuration application. Distinguish nominal cycle-table values, specified single-conversion limits, derived averaged bounds and application timeout margins; do not label nominal 125/500/1000-ms values guaranteed maxima.
- Keep one-shot trigger separate from continuous mode and avoid implicit changes to averaging or cycle on read.
- Every managed change reuses the guarded apply engine's SD → apply fields → CONFIG baseline → desired mode → final CONFIG readback boundary, including limit-only and routing-only changes. Invalidate prior current-generation sample evidence. Final readback verifies stable settings and establishes the current-generation baseline; readiness consumed there is not published as a fresh sample. Capture its alert events. Shutdown aborts an active conversion; a read-only try-fresh never performs these writes implicitly. An unverified change or outside access requires explicit recovery before managed freshness. Do not claim that discarding a fixed number of completions proves unspecified averaging behavior.
- Report configured/estimated period and latency, not a measured actual update rate. There is no exposed sample counter. The CC cycle is active conversion plus standby; when averaging exceeds the selected cycle, standby is zero. OS has no standby and CONV does not affect its duration (TMP116 pp.12-14,27; TMP117 pp.14-15,28; TMP119 pp.16-17,35).
- Report derived count-scaled active bounds with their source conditions, not an optimistic single ADC sub-conversion. Continuous nominal periods have no specified maximum; the stage 6 margin/slack is deadline policy. TMP117's single-conversion electrical row is conditioned on one-shot mode, so its use as a continuous estimate must remain labelled.

- Exercise all 32 CONV×AVG pairs using independent nominal table literals and separately labelled derived bounds. Model nonoverlapping accumulation blocks; intermediate ADC sub-conversions do not publish TEMP or DATA_READY, and shutdown abort does not manufacture a final averaged sample.
- Reuse stage 6's explicit unknowns for same-value and in-flight CONFIG changes. A conservative new-profile acquisition policy is library behavior, not a claim that silicon always restarts or preserves averaging on those writes. Document current/noise/latency tradeoffs without promising the headline accuracy/current under every setting.
- TMP116 comparator timing at averaged-block end is an inference from its completed-result model; only TMP117/TMP119 explicitly say comparisons include averaging. Preserve that distinction in tests and documentation.

## Verification and exit criteria

Test all legal mode/cycle/average combinations and bounds, false early readiness, correct mode after changing settings and unchanged bus state on invalid requests.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_20.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
