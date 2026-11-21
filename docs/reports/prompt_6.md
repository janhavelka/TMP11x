# Prompt 6 — Conversion timing, freshness and alerts

**PARTIAL. Phase a completed on 2026-11-21.** Prompt 6 is the first unfinished
entry in the sequence. Its prerequisites are prompts 3 and 5. The clean entry
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
