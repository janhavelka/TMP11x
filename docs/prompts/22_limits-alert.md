# 22 — Implement temperature limits, therm/alert and ALERT pin

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md), [16](16_temperature-codec.md), [17](17_config-codecs.md), [19](19_config-trust.md), [21](21_fresh-one-shot.md), [49](49_audit-20-21.md).
Produces: `limits-alert`.
Consumes: `register-ledger`, `behavior-vectors`, `temperature-codec`, `config-codec`, `config-trust`, `sync-acquisition`, `safe-apply-engine`, `register-fake`, `audit-20-21`.

## Objective and boundaries

Support complete programmable high/low limits and mode-specific alerts with truthful acknowledge behavior.

## Source audit

Compare TMP116/TMP117/TMP119 limit representations, alert vs therm behavior, polarity and DR/ALERT selection; inspect TMP1x2 thresholds/alert (but do not inherit fault-queue semantics) and OPT4001 interrupt handling.

## Complete implementation work

- Implement `setLimits`, `readLimits`, negative threshold encoding, therm/alert mode, polarity and ALERT vs data-ready pin selection, with readback. In therm mode reject TLOW > THIGH locally, including entry into therm with an inverted pair. This is conservative library policy for undocumented overlap behavior, not an encoding restriction. All limit writes occur inside the shared guarded SD apply phase. Defensively preserve ordered intermediate pairs when the current pair is verified valid (raise THIGH before crossing old THIGH; lower TLOW before crossing old TLOW). Unknown/currently inverted state requires explicit full-profile recovery and verification before therm operation; it does not select a different path that alone uses SD. Separate writes still are not atomic or glitch-free.
- Publish decoded `AlertStatus` with pending alert events, live therm state, source read timestamp/validity and explicit read-to-clear effects. Consume stage 19's sticky alert-mode evidence exactly once through the explicit alert consumer; unrelated reads/results may expose copies without draining it. Preserve generation/mode provenance and loss indicators across profile changes; therm HIGH is live hysteresis state, not a sticky software event. In therm mode, low flag semantics differ; do not infer a symmetric window.
- Optional GPIO read callback must not configure a pin or be invoked from ISR. Physical pin state and sensor register flags remain distinct observations.
- Document the exact read effects: CONFIG clears DATA_READY and alert-mode high/low flags; TEMP clears DATA_READY but does not acknowledge high/low flags. In therm mode, HIGH_Alert sets above THIGH and clears below TLOW only at conversion completion; LOW_Alert is always zero and CONFIG reads do not clear HIGH_Alert (TMP116 pp.15-16; TMP117 pp.16-17; TMP119 pp.18-19).
- POL=1 is active high and POL=0 active low. DR/Alert=1 routes DATA_READY to the pin, not a second threshold interrupt. Do not implement SMBus alert-response traffic in this first-release driver; flag acknowledgement by addressed CONFIG read is sufficient for the selected scope.
- TMP117 p.15 and TMP119 p.17 mention an ALERT flag at one-shot completion. Do not interpret this as unconditional HIGH_Alert/LOW_Alert assertion: their detailed flag tables and selected DR/Alert routing determine comparator flags and pin behavior. Retain the prose ambiguity in the ledger.
- Verify no unsupported TMP1x2 fault queue, interrupt queue or extended temperature mode leaks into API.

- Test latch re-arm explicitly: out-of-range then in-range before CONFIG acknowledgement retains an alert; acknowledgement clears it; a later still-out-of-range conversion reasserts. Exercise all eight T/nA×POL×DR/Alert combinations, independently observing logical status and pin interpretation. Active-high open-drain means released/pulled high, not driven high.
- Add typed limit/alert wrappers and public observations using the limit apply/readback/recovery already delivered by stages 18/19. Do not add a second engine. Document separate-write intermediate states and partial failures; no atomic or glitch-free limit-pair promise. Preserve any API ordering restriction as policy.
- Apply the shared SD/apply/final-readback boundary to limit-only and routing-only changes while the desired mode is continuous, as well as conversion-setting changes. Keep the valid-pair write order during that shared apply phase; it does not imply that these setters bypass shutdown or preserve an uninterrupted conversion cadence.
- Retain the unresolved offset/comparator ordering on supporting models and mode-specific effects of an outside ARA. Addressed CONFIG reads are the included acknowledgement path; no ARA transaction is emitted.
- Therm HIGH survives SD/aborted conversions while T/nA remains therm and no reset occurs; it changes at a later completed conversion under the documented comparisons. POL and DR/Alert govern the pin, not HIGH's retention. Pin persistence during SD with routing/polarity unchanged is an inference from those rules, not an explicit shutdown guarantee. Do not generalize this to DATA_READY or alert-mode flags on SD entry. GPIO reads never establish a new managed completion contract by themselves.

## Verification and exit criteria

Register tests: high/low positive and negative exact words, strict threshold
comparisons including equality, therm/hysteresis retention, CONFIG versus TEMP
read effects, disabled GPIO, polarity and explicit data-ready output mode.
Test an out-of-range owned OS whose readiness poll consumes HIGH: the alert
consumer still receives HIGH exactly once even if a later transfer fails.
Test therm inversion refusal and intermediate valid-pair write ordering.
Exercise therm [0,10] °C to alert [20,10] °C through the shared apply
engine. A failed, mismatched or late SD/alert readback must stop the
inverting TLOW write. On the return to therm, verify the ordered pair
before enabling hysteresis.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_22.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
