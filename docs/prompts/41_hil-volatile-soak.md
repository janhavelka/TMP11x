# 41 — Qualify volatile settings, recovery and finite hardware soak

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Physical fixture qualification**. Type: **physical**.
Prerequisites: [31](31_failure-recovery.md), [39](39_python-hil-runner.md), [40](40_hil-readonly.md).
Produces: `volatile-hil-record`.
Consumes: `readonly-hil-record`, `hil-runner`, `fault-closure`.

## Objective and boundaries

Exercise configuration, recovery and sustained read behavior on a known fixture with verified writable-setting preimage/restoration and fail-closed EEPROM guards. Do not promise exact restoration of transient status or conversion history.

## Source audit

Revisit TMP11x EEPROM protection and TI power-up busy behavior, TMP1x2 full HIL and soak flow, current runner safety hooks and identified stage 40 fixture evidence.

## Complete implementation work

- Obtain explicit active-fixture authorization and capture raw CONFIG/THIGH/TLOW plus OFFSET only on supporting models, EEPROM_UL locked/not-busy evidence, temperature/ID and exact firmware identity. Derive the restoration profile from the per-model writable-field mask; TMP116 address 0x07 is EEPROM3, not OFFSET. Quiesce a pre-existing one-shot or decline the mutating trial rather than pretending its in-progress conversion can be restored.
- Run finite mode/cycle/averaging and limits/ALERT/offset volatile trials only when model supports them; verify write-readback, measurement fresh evidence and each restoration step. Never unlock or program EEPROM.
- Test disconnect/reconnect/power-cycle recovery only with an explicitly suitable fixture and quiesced or isolated bus. TMP117 SNOSD82D section 7.5.3.1 (PDF p.20) forbids connecting the device during active communication; do not hot-plug it into a running shared bus. Record acquisition counts, observed readiness and timing; these chips have no conversion sequence counter, so equal temperatures or a DATA_READY bit cannot establish how many conversions were missed or duplicated.
- Every failed/aborted trial attempts bounded restoration only while the same target/route, transport and locked/not-busy EEPROM state remain established. Refuse unsafe restoration after identity loss or uncertain lock state, retain the baseline for an operator, and report separate primary/restoration failures with observed state (or unknown when unreadable). Never send a reset or raw preimage write as unconditional cleanup.
- A sensor unavailable at stage 40 leaves this stage BLOCKED; prepare runner commands and guard tests without producing a false hardware PASS.

- Where the authorized fixture permits, add latch acknowledgement/re-arm, all alert routing/polarity modes, averaged-conversion timing/partial abort and full-profile recovery trials. Qualify physical pin behavior separately from decoded flags and keep the startup/EEPROM guard/restoration policy in every trial.
- Track source-unknown questions (write-phase behavior, continuous TEMP byte coherence, offset versus limit comparator) with exact fixture/revision/method and observed limits. A few successful observations cannot prove universal atomicity. Leave unavailable evidence NOT RUN; do not replace it with simulator assumptions or an unrequested destructive campaign.

## Verification and exit criteria

Verify actual physical transcripts, no EEPROM write/protection violations, restored writable fields/limits and supported offset, finite acquisition counters and failure handling. Compare CONFIG through the approved mask and canonical mode encoding; read-to-clear flags, busy status, self-clearing reset and one-shot transitions cannot equal an arbitrary raw preimage. See TMP117 Tables 7-6/7-10 (PDF pp.27-30), corresponding TMP116 Table 6 and TMP119 Table 8-5. Qualify per exact tested part and fixture only.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_41.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
