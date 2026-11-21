# Prompt 8 — Public API and variant architecture

**Phase a, 2026-11-21.** Stage 8 is eligible. Its seven prerequisite stages
are complete and all eight declared inputs are available. The clean baseline
was `521752a197d99b2d1096fa1ddd6bdcfc34d1849d` on `main`. Audit 45’s
commit message and three paths match the previous wrapper request. The wrapper
verified upstream equality at 19:01:25 Prague time. The sequence places 8
between audit 45 and stage 9. [Stage brief](../prompts/8_api-architecture.md),
[sequence](../prompts/SEQUENCE.json), [progress ledger](../prompts/PROGRESS.md).

## Inputs checked at the baseline

| Artifact | Producer and evidence carried into the public contract |
| --- | --- |
| `ecosystem-audit` | Stage 1. Typed status, synchronous borrowed callbacks, passive health, retained results and port attribution. [Peer comparison](../library-comparison.md), [source adoption](../source-adoption.md). |
| `tooling-plan` | Stage 2. Standalone C++17 public headers and an SDK-independent core. Build adapters, examples, tests and the vendor archive have separate package roles. [Toolchain and package plan](../toolchain-and-package-plan.md). |
| `variant-contract` | Stage 4. Select the model from the BOM. TMP116/N requires the full ID word `0x1116`. TMP117/M/N and TMP119 share low 12 bits `0x117`. Keep observed revision, family compatibility and exact-model evidence separate. [Variant contract](../variants.md). |
| `register-ledger` | Stage 5. TMP116/N has four readable EEPROM shadows, including `0x07`, and refuses offset and addressed reset before I2C. The other four models have three shadows, optional offset at `0x07` and a dedicated addressed reset. [Per-model register ledger](../registers/tmp11x.json). |
| `behavior-contract` | Stage 6. TEMP consumes DATA_READY. CONFIG also consumes alert-mode latches. Timing, stale baselines and the source of observed flags constrain what a successful read can publish. [Device behavior](../device-behavior.md), [literal vectors](../registers/behavior-vectors.json). |
| `safety-policy` | Stage 7. Mutations and fresh acquisition require an operation-local locked/idle observation at `0x04`. EUN set refuses both. Preserve the exclusions for EEPROM and shadow writes, unsafe raw access, general call and alert-response traffic. [Policy and transcripts](../eeprom-and-reset-policy.md). |
| `feature-ledger` | Stage 5. Map all 18 register rows, 20 behavior rows and eight exclusions to their API, job, CLI or application responsibilities and refusal paths. Name the reason where a passive getter needs no job. [Feature matrix](../feature-coverage.md), [coverage requirements](../prompts/FULL_COVERAGE.md). |
| `audit-1-7` | Stage 45. The accepted corrections cover binding validation order, health before initialization, unused read bits and the addressed-reset command. [Audit report](prompt_45.md). |

## Source checks

Two read-only checks covered peer code and silicon independently. The peer
check compared the following pinned files with the four clean local checkouts.
The inspected files matched. The last column records GitHub branch tips
observed separately during this run.

| Repository and pin | Implementation and test evidence | Remote revision |
| --- | --- | --- |
| TMP1x2 `afd5088` | `include/TMP1x2/{Config,Status,TMP1x2}.h` and `src/TMP1x2{,Configuration,Measurement,Operations}.cpp`. The `helpers` case `trackedWordObservations` distinguishes tracked reads. The `jobs` case `readFailureAndLatePublication` checks failure and cache publication. | `main@3da8dd9` |
| OPT4001 `441ef57` | `src/OPT4001.cpp` handles identity and captures readiness from destructive flag reads. `test/test_basic.cpp` covers elapsed time without a ready flag. | `main@f17d494` |
| ADS1115 `e07622a` | `src/ADS1115.cpp` preserves the first dirty fault. `test/test_basic.cpp` covers partial writes and generation commit after readback. | `main@1aab07b` |
| FieldCore-node `c40ea83` | `include/TunnelMonitor/i2c/I2cOwnerTransport.h` and `src/devices/coil_head/CoilHeadModule.cpp` show borrowed buffers and timeout clipping. `test/host/rodtester/coil_head_module.cpp` covers the deadline boundary. | `rodtester@453db51` |

The silicon check inspected the family maps, CONFIG, EEPROM, offset and ID
tables on pp. 24–31 of [SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24),
pp. 25–32 of [SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25)
and pp. 32–39 of [SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32).
Carry forward the TMP116 startup BUSY contradiction, CONFIG clear edge, TEMP
byte latching, MOD during and after one-shot, offset/comparator ordering,
addressed-reset effects and the TMP117 DATA_READY routing discrepancy.

## Phase-b brief and review cases

Phase b owns `docs/api-contracts.md` and `docs/architecture.md`, together
the `api-contract` handoff. Both paths were absent at entry. The first
consumer is stage 9, which owns the detailed deadline and result-lifetime
contract. Stage 46 reviews both documents before production work begins.
[Handoff inventory](../prompts/HANDOFFS.md), [owner contract brief](../prompts/9_owner-deadline-contract.md).

- **Public surface.** Specify `TMP11x::TMP11x`, the Config, Status,
  CommandTable, TMP11x and Version headers under `include/TMP11x/`, typed
  status, settings, identity, health, samples and snapshots. Give the binding
  its model, address, transport policy and borrowed callback contexts.
  A stateless callback accepts a null context. Adapter success promises a
  complete transfer. Distinguish timed operations requiring the clock hook
  from latest-value reads with an optional observation timestamp. Show
  adjacent TMP1x2 and TMP11x consumer sketches sharing one application I2C
  owner. [Public conventions](../prompts/CONVENTIONS.md).
- **Lifecycle and trust.** Specify the state required by each operation,
  failure outputs and callback counts. Cover rejected rebind, bus-silent
  lifecycle and invalidation, bound-only diagnostics, initialized dirty
  latest reads, and clean verified settings for fresh reads. Preserve
  historical sample and alert provenance on invalidation. Derive
  `isOnline()` from initialization plus READY/DEGRADED. Count tracked
  physical outcomes even during initialization. Check a short transfer,
  a late physical success and an unrelated diagnostic during a retained job
  result. [Engineering contract](../prompts/ENGINEERING.md).
- **Replay and recovery.** Keep binding immutable during configure, initialize
  and recover. Specify stable CONFIG fields, both signed limits and the
  ordering of guarded shutdown, apply, baseline, desired mode and final
  verification. A settings change commits its candidate on the first mutating
  write attempt, including an ambiguous failure. Check intermediate alert
  states during limit changes and dirty trust after failed final readback.
  When startup proof is absent,
  establish FIRST_ACK in the current begin/recover operation using the clock
  hook and the owner’s stable-power guarantee. Before stage 23 offset
  provenance is unknown. From stage 23 observe unmanaged calibration on
  supported models and replay only explicitly managed offset, including zero.
- **Acquisition.** Distinguish `readLatest`, `tryReadFresh`,
  `readFreshBlocking` and `readOneShotBlocking`, with explicit cooperative
  read kinds. For one-shot, check desired shutdown, initialization and trust
  locally, guard at `0x04`, compare all owned stable CONFIG fields, write
  shutdown, consume stale readiness, trigger once, poll readiness and read
  TEMP. Drift or unowned MOD=11 sets dirty before any CONFIG write. A known
  successful trigger retains ownership across completion, cancellation or
  timeout until explicit successful shutdown or invalidation. Record a late
  trigger’s physical effect separately from the timed-out result. Check cache
  generation, acquisition kind, configuration trust, observation time and
  offset provenance after a later settings change.
- **Implementation owners.** Assign each register function one owner. Stages
  12/13 supply private transport and identity, 14 exposes bound identity,
  18/19 own safe apply and trust, 20/22 add setters, 21 owns measurement,
  and 23 extends replay with offset. Stage 26 integrates bound diagnostics.
  Stages 29/30 drive the same apply and acquisition engines through one
  operation slot. Map the 46 coverage rows through the shared CLI, including
  each model refusal and application-owned function. [CLI contract](../prompts/CLI_CONTRACT.md).

## Phase-a checks

Python 3.12.10 passed `python -B scripts/check_promptset.py` (55 stages),
`python -B scripts/check_references.py` (14 pinned PDFs),
`python -B tools/check_register_ledger.py` (six BOM models) and
`python -B tools/test_check_register_ledger.py` (31 regressions).
Git 2.52.0.windows.1 reported zero commits on either side of
`HEAD...origin/main`. Native driver execution, MCU links, remote CI and
physical qualification were NOT RUN. Independent review of the authored
API and ownership documents belongs to phase c.
