# Prompt 6 — Conversion timing, freshness and alerts

**COMPLETE on 2026-11-21.** The timing and read-effect contract, literal
vectors and ledger refinements passed acceptance. Its prerequisites are
prompts 3 and 5. The clean entry
revision was `4a8e4dcb2d4993b361b4d1f2e8081a6cdc1a37cf` on `main`,
equal to the local `origin/main`. Prompt 5's acceptance request matches the
message and three paths in `963472276e16268a99a35cc7aa1a22c67aa32252`.
The wrapper recorded upstream verification for that commit at 15:59:28 Prague
time. The intervening commit at HEAD changes references in prompts 35 and 39.
[Sequence](../prompts/SEQUENCE.json), [prompt 6](../prompts/6_timing-status.md).

## Verified inputs

Prompt 3 supplies the `ti-archive`. Prompt 5 supplies the `register-ledger`
and `register-validator`. The existing CONFIG/CONV field contains an 8 × 4
nominal cycle matrix in each of the three profiles. Phase b will refine timing
and read effects in this ledger and produce `behavior-contract` at
`docs/device-behavior.md` and `behavior-vectors` at
`docs/registers/behavior-vectors.json`. Both outputs were absent at entry.
[Archive manifest](../reference/manifest.json), [register ledger](../registers/tmp11x.json),
[validator](../../tools/check_register_ledger.py), [handoffs](../prompts/HANDOFFS.md).

| BOM selection and pinned TI source | Timing and modes (one-based PDF pages) | Status, reset and register evidence |
|---|---|---|
| [TMP116/N, SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5) | p.5 single conversion 13.5/15.5/17 ms (min/typ/max). pp.11–14 cover block update, shutdown abort and one-shot entry from shutdown. Table 7 p.27 includes standby footnotes. | pp.14–16 comparator and pin, p.17 startup, pp.25–29 TEMP/CONFIG and busy. pp.6, 21 serial timeout. |
| [TMP117/M/N, SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6) | p.6 gives 13/15.5/17.5 ms **in one-shot mode**. pp.12–15 describe accumulation and modes. Table 7-7 p.28 gives continuous cycles. | pp.15–17 comparator and pin, pp.7, 18 startup, pp.26–28 TEMP/CONFIG and addressed reset, p.31 offset. pp.7, 22 serial timeout. |
| [TMP119, SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7) | p.7 gives 13/15.5/17.5 ms **without internal averaging**. pp.14–17 describe accumulation and modes. Table 8-6 p.35 gives continuous cycles. p.23 addresses I²C activity during conversion. | pp.17–19 comparator and pin, pp.8, 20 startup, pp.33–35 TEMP/CONFIG and addressed reset, p.38 offset. pp.9, 24 serial timeout. |

### DATA_READY race

The previously inaccessible thread was readable on 2026-11-21. Adam Greig
reported a TMP117 logic-analyzer capture in which a CONFIG read returned
DATA_READY clear while a conversion completed during the transaction. The
routed ALERT signal deasserted, and the following poll also read zero. TI's
Ren Schackmann recommended one-shot for capturing each measurement and
explained that repeated TEMP reads cannot distinguish identical successive
results. The reply gives no bit-level clear or byte-latch specification.
Carry the missed indication as a possible trace. Its exact edge, incidence
and applicability to other parts remain open. A zero poll supplies no fresh
evidence. Sparse polling cannot guarantee capture of every continuous update.
The original intake records the historical access failure.
[TI E2E thread 1019457](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1019457/tmp117-data_ready-flag-cleared-incorrectly-if-data-becomes-ready-during-read-of-configuration-register),
[intake notes](../reference/extracted/intake-notes.md).

## Phase b acceptance cases

- **Timing and clocks.** Preserve all 32 table cells per profile. A continuous
  cycle includes active time, with zero standby when the averaging block fills
  the cycle. For counts 1, 8, 32 and 64, derive active min/typ/max separately.
  At 64, typical is 992 ms and the count-scaled upper times are 1088 ms for
  TMP116/N and 1120 ms for TMP117/M/N and TMP119. Keep TMP117's one-shot
  condition visible in any continuous estimate. CONV has no effect on one-shot
  duration. Distinguish expected duration, first poll and absolute deadline.
  Default one-shot polling starts at successful trigger completion plus the
  count-scaled upper time, rounded up to clock resolution, followed by sparse
  polls. Continuous waiting uses one poll per nominal cycle, rounded up from
  the baseline or last poll. A due time beyond the deadline expires without
  I/O. Specify a finite continuous margin and slack as library policy.
  Untimed callers pace their own attempts. Earlier polls may coincide with
  conversion, and other bus users can communicate during any wait. Keep the
  hardware's 20–40 ms serial timeout (35 ms typical under the specified held-line
  condition), callback budget and operation deadline separate.
- **Startup and reset.** Anchor FIRST_ACK internally at completion of the first
  successful compatible read during begin/recover when proof is absent. Reads
  can succeed during load. Require stable power through dependent operations,
  an explicit policy wait, a safe lock/busy observation at 0x04 and verified
  writes. Reuse valid proof until invalidated. TMP116 p.17 conflicts with
  pp.26, 29 on busy at POR. TMP117 p.7 gives a typical 1.5 ms reset with no
  maximum. The separate 2 ms addressed-reset text concerns TMP117/TMP119 only.
  After reset establish a new origin. Caller timestamps cannot seed it.
- **Freshness boundaries.** A managed one-shot rejects dirty settings or a
  desired mode other than shutdown before I²C. Guard 0x04, compare stable CONFIG
  before writing, explicitly enter shutdown, read CONFIG to consume old ready,
  then trigger exactly once. Drift means zero CONFIG writes. Only retained
  owned-trigger evidence admits MOD=11 at preflight. Completion requires new
  readiness evidence. For every managed continuous profile change, including a
  limit or pin-only change, enter guarded shutdown, apply/verify, clear stale
  ready, enter continuous mode and verify stable fields in the final CONFIG
  read. That last read establishes the generation baseline and retains alerts.
  A read-only try-fresh needs verified settings and its own 0x04 guard. A first
  read with no baseline establishes one without publishing. Include repeated,
  failed and diagnostic reads, with uncertainty when flags may have been
  consumed. One discarded result cannot establish the effects of a mid-block
  settings change. [Engineering contract](../prompts/ENGINEERING.md).
- **Temperature and averaging.** Pair signed counts with independent wire words,
  negative thresholds, rounding boundaries and signed endpoints. Reset TEMP
  `0x8000` has a numeric interpretation and lacks initial sample validity until
  a completed conversion including averaging. Trace two nonoverlapping blocks
  and a shutdown midway through a block. Only completed blocks publish. Use
  exactly representable averages where hardware rounding is unspecified.
- **Comparators and pin.** Exercise strict high/low crossings, equality,
  alert acknowledgement and re-arm, therm hysteresis and all eight
  T/nA × POL × DR/Alert combinations. Either TEMP or CONFIG consumes ready.
  CONFIG also acknowledges threshold flags in alert mode. In therm mode HIGH
  survives an abort while therm mode holds and no reset occurs, and LOW reads
  zero. Pin persistence through shutdown with unchanged routing and polarity
  is inferred. TMP116 block-end comparison is also inferred. TMP117 p.15 and
  TMP119 p.17 explicitly include averaging. Preserve the ledger's one-shot
  ALERT wording conflict and keep threshold events distinct from completion.
- **Unresolved effects and provenance.** Carry same-value CONFIG writes,
  mid-block AVG/CONV changes, MOD readback during/after one-shot, active
  retrigger, ready/alert-mode flag retention on shutdown, inverted therm
  limits, the read-clear edge and TEMP byte latching as unknown. Trace
  non-atomic multi-register apply and partial failure. Cover supported offset
  saturation while leaving offset/comparator ordering open. For F13, carry
  arbitrary nonfactory EEPROM shadow observations across reset as invalidated
  prior observations. Factory identity and NIST certification cannot be inferred
  from a word pattern. Use per-case pages, conditions and explicit assumptions.
  [Coverage obligations](../prompts/FULL_COVERAGE.md), [TMP116 notes](../reference/extracted/tmp116-coverage-audit.md),
  [TMP117 notes](../reference/extracted/tmp117-coverage-audit.md),
  [TMP119 notes](../reference/extracted/tmp119-coverage-audit.md).

## Source comparisons and verification

Independent read-only reviews checked timing origins against the three PDFs
and examined two clean sibling checkouts. In TMP1x2 at
`c91bac63313ea725ce174ab98abc50705c365639`, `docs/owner-operations.md`,
`src/TMP1x2Operations.cpp` and `test/jobs/test_jobs.cpp` supply patterns for
callback duration, clock wrap and deadline expiry without further traffic.
TMP11x derives its own wait and requires its authoritative clock. The TMP1x2
35 ms extended-mode wait does not apply. GitHub main had advanced to
`3da8dd94f6010ed6ad7aa5d11a1f98ef6bc390cc`. In OPT4001 at
`f17d4944640145773d3331e155a5bc728fcc4c66` (also GitHub main),
`src/OPT4001.cpp` and `test/test_basic.cpp` show how to retain a destructive
read and test a stalled clock. TMP11x has one DATA_READY bit and lacks the
optical device's sample counter and CRC.

Phase a checks used Python 3.12.10, PyMuPDF 1.27.2.2 and Git 2.52.0.windows.1.
`python -B scripts/check_references.py` verified all 14 PDF hashes.
`python -B tools/check_register_ledger.py` passed for six BOM selections and
`python -B tools/test_check_register_ledger.py` passed 22 regressions.
`python -B scripts/check_promptset.py` and the report and index whitespace
checks passed. Phase b owns the two research artifacts and ledger updates.
Stage 7 consumes the behavior contract for safety policy, audit 45 reviews
the evidence, and stages 16–23 and 32 execute the applicable vectors. Driver
tests, MCU builds, remote CI and physical timing qualification were not run.

## Phase b deliverables and verification

Phase b began on clean `main` at `4481bb457472a0a6b002f9ea19234e93ab3c0462`,
equal to local `origin/main`. The prior request's message and sole report
path match that commit. The wrapper verified upstream at 16:18:08 Prague
time. Local TMP1x2 and OPT4001 remained clean at the phase-a source revisions.

The [device behavior contract](../device-behavior.md) supplies
`behavior-contract`. The [literal vectors](../registers/behavior-vectors.json)
supply `behavior-vectors`. The [register ledger](../registers/tmp11x.json) now
records the conditioned electrical row beside each AVG field, the typical
reset interval and missing maximum, the startup wait origin, and the
effects of simultaneous or failed CONFIG/TEMP reads. The historical source
conflicts remain open.

| Case | Phase b evidence |
|---|---|
| Timing and standby | All 96 nominal cells agree with the three individual TI tables and the stage 5 ledger. All 32 TMP116 footnotes agree with p.27. Four count-scaled min/typ/max tuples per profile retain the conditions of the electrical row. TMP117's continuous upper estimate extrapolates its one-shot row. |
| Clock and reset origin | Five scheduling cases cover a late trigger origin relative to admission, an insufficient remaining budget, a delayed CC poll, active-limited CC and a late destructive read. Two reset cases distinguish FIRST_ACK from bind/read start and from a completed addressed-reset command. |
| Arithmetic and comparison | Eleven signed TEMP/limit words, 14 host-quantization cases and three supported offset results cover negative fractions, endpoints, half-count ties, rejection and saturation. The example host codec rounds ties away from zero. Eight pin routes and separate alert/therm traces cover equality, acknowledgement, re-arm and abort. |
| Acquisition | Eleven event traces include two successive averaging blocks, residual work after OS, preflight drift, all-settings CC baseline, ordered diagnostic reads, the TMP117 race and a partial therm-limit write. Nonfactory EEPROM observations become historical across reset even when their bits repeat. |

The default software choices are a 3 ms startup wait from FIRST_ACK,
5 ms between subsequent OS polls and 10 ms conversion-window slack. CC
adds a 25% nominal margin before comparing against the active estimate.
The first OS poll uses the count-scaled upper estimate and CC polls use
the nominal cycle. A fixed whole-operation deadline covers every transfer.
These values give implementers bounded, testable defaults. TI gives no
guaranteed maximum CC period or POR load time.

We reread the TMP117 E2E capture and its TI response. A related TMP117
one-shot user also reported occasional missed indications with early polling
on one of two devices. TI proposed a delayed latest read and requested a
timeout capture to diagnose the cause. The report does not establish that
the continuous-read race caused those timeouts. The related TMP116 thread
822036 returned HTTP 403 during this check. The exact clear edge and TEMP
byte-latch behavior remain unresolved.
[Continuous case](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1019457/tmp117-data_ready-flag-cleared-incorrectly-if-data-becomes-ready-during-read-of-configuration-register),
[one-shot case](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/909104/tmp117-polling-the-data-ready-flag-seems-to-clear-it-inadvertently-when-using-1-shot-mode).

| Local check | Result on the phase b files |
|---|---|
| `python -B scripts/check_references.py` | PASS, all 14 pinned file sizes and SHA-256 digests. |
| `python -B tools/check_register_ledger.py` and `python -B tools/test_check_register_ledger.py` | PASS, six selected models and 22 regressions. |
| `python -B scripts/check_promptset.py` | PASS, 55 stages and the existing handoff, coverage and source-page checks. |
| Focused Python and PyMuPDF checks | PASS, extracted cell and footnote literals, source groups, ledger agreement, scaled durations, clock examples, signed wire words, rounding, offset clamps, block means and eight pin routes. |
| Markdown links, PDF anchors, UTF-8 and whitespace | PASS on both new artifacts and the changed report and ledger. |

Independent timing and event-vector work was integrated and checked against
the pinned documents with Python 3.12.10 and PyMuPDF 1.27.2.2. Git 2.52.0.windows.1
reported a clean index. The wrapper owns staging, commit, push and upstream
verification for this phase. Phase c owns the fresh-session review. Driver
tests, MCU links, remote CI and physical qualification were not run.

## Phase c review

The reviewed revision was `bf016d2c403553f8b0d7b1107514e31b6d8fe133` on
clean `main`. Its message and four paths matched the phase b request.
The wrapper recorded upstream verification at 16:37:09 Prague time.
Two read-only reviewers independently examined timing and event semantics.
The lead checked their findings against the pinned tables, research artifacts
and stages 7, 19 and 21. The two sibling checkouts and their GitHub main
revisions matched the phase a comparison.

| Finding | Correction and evidence |
|---|---|
| Rounding the interval alone can shorten a physical wait when callback completion falls between clock ticks. | The scheduling rule now accounts for the resolution of the completion timestamp. In `reset-and-first-ack-between-ticks`, reset finishes at 50.9 ms. Tick 53 permits the next read. FIRST_ACK at 53.8 ms permits the guard at tick 57, where the original deadline leaves zero transfer budget. The aligned examples still use their original due times. [Reset command](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28), [vectors](../registers/behavior-vectors.json). |
| A safe 0x04 at entry does not cover a later CONFIG observation of busy or changed settings. | `0xB620` at SD preflight and `0xB220` on a CC readiness poll invalidate the guard while preserving HIGH. An untimed attempt stops. A timed path must recheck 0x04 before proceeding. `0xA240` under desired `0x0220` reveals AVG drift and latches dirty. All three suppress fresh publication at that observation. [CONFIG fields](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27), [trust contract](../prompts/19_config-trust.md). |
| A continuous readback fixture retained HIGH without specifying the comparator mode. The equality wording also combined two independent limit comparisons. | Both generations of the sticky-HIGH fixture now explicitly use alert mode. Each comparator applies a strict inequality against its own threshold. The therm trace retains HIGH as a hysteresis level across a partial-block abort. [TMP116 pp.15–16](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=15), [behavior contract](../device-behavior.md). |
| The TMP117 race report gives incompatible routing details. | The author describes bit 2 but gives CONFIG as `0x0040`, which selects AVG bit 6. The register map gives DR/Alert as `0x0004`. The trace retains the reported pin transition and two zero observations. Actual routing, exact clear edge and TEMP byte coherence remain open. Thread 909104 was also accessible. The linked TMP116 thread 822036 still returned HTTP 403. [Reported capture](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1019457/tmp117-data_ready-flag-cleared-incorrectly-if-data-becomes-ready-during-read-of-configuration-register), [TMP117 register map](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27). |

Focused Python 3.12.10 and PyMuPDF 1.27.2.2 assertions passed on the
reviewed worktree. Direct PDF extraction matched 96 nominal cycle cells,
32 TMP116 standby notes and the single-conversion rows underlying all 36
derived active values. Arithmetic checks covered five scheduling cases,
three clock origins, three new CONFIG words, 11 signed words, 14 quantization
cases, three offset clamps, two complete averaging blocks, the partial
block and eight pin routes. The trace review also checked both comparator
modes, owned-shot provenance after cancellation or ambiguous transfers,
diagnostic read order and F13 invalidation with changed or repeated shadows.

`python -B scripts/check_references.py` passed for 14 pinned PDFs.
`python -B tools/check_register_ledger.py` passed for six selected models
and `python -B tools/test_check_register_ledger.py` passed all 22 tests.
`python -B scripts/check_promptset.py` passed for 55 stages. Local link,
PDF page and Git whitespace checks passed. Phase d owns final acceptance
and the progress/index update. Driver tests, MCU links, remote CI and
physical qualification were not run in phase c.

## Phase d acceptance

Acceptance started with a clean index and worktree on `main` at
`d7fe1874d98151836080f3929bec8310f460b96d`, equal to local `origin/main`.
Its parent, message and four changed paths agree with the phase c baseline,
commit request and wrapper log. The wrapper verified that revision against
upstream at 16:48:34 Prague time. The acceptance changes are this report,
its index entry and the dedicated stage 6 progress row. The wrapper stages
those three files, checks the staged diff, commits and verifies upstream.

| Acceptance check | Result |
| --- | --- |
| `python -B scripts/check_references.py` and `python -B scripts/test_check_references.py` | PASS. All 14 pinned PDF hashes and nine reference-tool regressions. |
| `python -B tools/check_register_ledger.py` and `python -B tools/test_check_register_ledger.py` | PASS. The ledger covers six BOM selections and all 22 regressions passed. |
| `python -B scripts/check_promptset.py` and `python -B scripts/test_check_promptset.py` | PASS. The checker covers 55 stages and all 36 regressions passed. |
| Inline `python -B -` with PyMuPDF | PASS. The three pinned timing tables match the vectors and ledger on all 96 nominal cycle cells. The TMP116 table matches all 32 standby notes. The three electrical rows reproduce 36 conditioned active values. |
| Inline `python -B -` vector assertions | PASS. Five poll/deadline cases, three reset/FIRST_ACK origins, CONFIG busy and drift words, 11 signed words, 14 quantization cases, three offset clamps, eight pin routes and two complete block means all agree with their stated rules. The 11 traces have source attribution. Repeated TEMP reads and the partial-block abort retain the last completed word. |
| Local Markdown links, PDF page anchors and Git whitespace | PASS for the behavior contract and acceptance documents. The working diff passed `git diff --check`. The index was empty and passed `git diff --cached --check` before wrapper staging. |

Checks used Python 3.12.10, PyMuPDF 1.27.2.2 and Git 2.52.0.windows.1.
[Stage 7](../prompts/7_eeprom-reset-policy.md) consumes the behavior contract
for its EEPROM and reset policy. Audit 45 reviews the research, and
stages 16–23 and 32 use the applicable vectors for implementation and tests.
The exact CONFIG read-clear edge, TEMP byte latching, offset/comparator order
and other unknowns remain documented. The TMP117 race report is available
with conflicting routing details. The related TMP116 thread remains
inaccessible in the recorded source review.
[Device behavior contract](../device-behavior.md), [behavior vectors](../registers/behavior-vectors.json).

Native driver tests, MCU links, remote CI and physical qualification were
NOT RUN. Stage 43 owns the first remote checkpoint.
