# TMP11x implementation progress

**State:** All six model selections support guarded initialization and recovery, configuration verification and dirty-state tracking, typed conversion settings and timing estimates, latest and fresh temperature reads, and explicit one-shot acquisition. The driver also supports signed temperature limits, therm hysteresis, retained alert events and optional ALERT pin sampling.
**Target:** 55 stages in manifest order, including 11 new audit checkpoints. TMP117 is primary, with all six selected BOM-model capability gates covered by every relevant stage.
**Repository:** `main`, tracking `origin/main` at `https://github.com/janhavelka/TMP11x.git`.
**Preparation input HEAD:** `6c5e4496d5c896af689228d4bee4382a53c20626` (clean and synchronized at entry).
**Hardware:** Physical TMP11x board and test wiring NOT CONFIRMED.
**Last completed stage:** 22 (temperature limits and alert handling).
**Last update:** 2026-11-26. Next eligible stage is [23](23_offset-feature.md), opt-in hardware offset.

Preparation evidence includes the [6b173cc second follow-up](../reports/audit-followup-6b173cc.md),
the [cb73740 follow-up](../reports/audit-followup-cb73740.md#response--2026-11-17),
[individual audit responses](audit.md#response--2026-11-17),
[full promptset audit](../reports/promptset-audit.md)
and [datasheet/functionality gap audit](../reports/feature-gap-audit.md).
The latest [handoff consistency audit](../reports/handoff-audit.md) assigns
every planned deliverable a producer and checks consumer dependencies.

| Stage range | Objective | Current state and evidence |
|---|---|---|
| 1 | Current sibling API/ownership audit | COMPLETE — [report](../reports/prompt_1.md) |
| 2 | Toolchain/package/prompt workflow audit | COMPLETE — [report](../reports/prompt_2.md) |
| 3 | Official source intake | COMPLETE — [report](../reports/prompt_3.md); optional TI code access gaps retained |
| 4 | Model identity and board integration | COMPLETE — [report](../reports/prompt_4.md), [variant contract](../variants.md), [board guide](../board-integration.md) |
| 5 | Per-model register, field and feature ledger | COMPLETE — [report](../reports/prompt_5.md), [register ledger](../registers/tmp11x.json), [feature coverage](../feature-coverage.md), [validator](../../tools/check_register_ledger.py) |
| 6 | Conversion timing, freshness and alert behavior | COMPLETE — [report](../reports/prompt_6.md), [behavior contract](../device-behavior.md), [literal vectors](../registers/behavior-vectors.json) |
| 7 | EEPROM and reset safety policy | COMPLETE — [report](../reports/prompt_7.md), [safety policy](../eeprom-and-reset-policy.md), [register ledger](../registers/tmp11x.json). TMP116 POR busy and addressed-reset effects remain open. |
| [45](45_audit-1-7.md) | Audit prompts 1–7 before freezing the API | COMPLETE — [report](../reports/prompt_45.md), including independent findings, fixes and final checks. Open silicon evidence and later qualification gates remain recorded. |
| [8](8_api-architecture.md) | Public API and variant architecture | COMPLETE — [report](../reports/prompt_8.md), [API contract](../api-contracts.md), [architecture](../architecture.md) |
| [9](9_owner-deadline-contract.md) | Cooperative ownership, timing and result contract | COMPLETE — [report](../reports/prompt_9.md), [owner contract](../owner-operations.md) and eleven acceptance traces |
| [46](46_audit-8-9.md) | Audit prompts 8–9 before production code starts | COMPLETE — [report](../reports/prompt_46.md), independent reviews, contract corrections and repeat acceptance checks |
| [10](10_package-foundation.md) | Independent C++17 library foundation | COMPLETE — [report](../reports/prompt_10.md), [host test entrypoint](../testing.md), version tools and verified source exports |
| [11](11_status-values.md) | Portable Status, model and value types | COMPLETE — [report](../reports/prompt_11.md), six model selections, shared capability table and strict Release acceptance |
| [12](12_i2c-register-transport.md) | Bounded 16-bit I2C register transport | COMPLETE — [report](../reports/prompt_12.md), six-model command table, private transport and reusable register fixture. All eight Release tests and the tests-off consumer passed. |
| [13](13_identity-selection.md) | Validate device family for the selected BOM model | COMPLETE — [report](../reports/prompt_13.md). All nine Release tests and the tests-off consumer passed. |
| [14](14_binding-lifecycle.md) | Bus-silent binding, teardown and public identity diagnostics | COMPLETE — [report](../reports/prompt_14.md). All ten Release tests and the tests-off consumer passed. |
| [15](15_passive-health.md) | Passive transport health | COMPLETE — [report](../reports/prompt_15.md). All six model selections recovered from OFFLINE on an ordinary tracked read. All 11 Release tests and the tests-off consumer passed. |
| [16](16_temperature-codec.md) | Signed temperature and threshold codecs and latest reads | COMPLETE — [report](../reports/prompt_16.md). All 65,536 register words round-tripped. Latest-read tests covered all six models and preserved the previous sample on failure or expiry. All 13 Release tests and the tests-off consumer passed. |
| [17](17_config-codecs.md) | Type-safe CONFIG codecs and validation | COMPLETE — [report](../reports/prompt_17.md). All 3,072 steady profiles matched their expected encodings across six models. All 14 Release tests and the tests-off consumer passed. |
| [47](47_audit-10-17.md) | Audit prompts 10–17 before initialization writes | COMPLETE — [report](../reports/prompt_47.md#phase-d-acceptance). Corrected the CMake 3.16 host test command. All 14 Release tests, the extracted consumer and all 393,216 public identity cases passed. |
| [18](18_safe-initialize.md) | EEPROM-safe initialization and configuration writes | COMPLETE — [acceptance report](../reports/prompt_18.md#phase-d-acceptance). All 15 Release tests passed, including guarded begin, recovery and latest reads on six model selections. |
| [19](19_config-trust.md) | Observed configuration, verification and dirty state | COMPLETE — [acceptance report](../reports/prompt_19.md#phase-d-acceptance). All 16 Release tests passed. Partial verification retained the completed reads and detected drift. Guarded replay restored trust. |
| [48](48_audit-18-19.md) | Audit prompts 18–19 before adding settings and acquisition | COMPLETE — [acceptance report](../reports/prompt_48.md#phase-d-acceptance). All 16 Release tests passed, including renewed startup and EEPROM guards, partial verification and alert provenance across recovery. |
| [20](20_conversion-settings.md) | Conversion settings and timing | COMPLETE — [acceptance report](../reports/prompt_20.md#phase-d-acceptance). All 19 Release tests passed, including 384 steady profiles, 192 timing selections and shutdown during averaging. |
| [21](21_fresh-one-shot.md) | Fresh-conversion acquisition and explicit one-shot | COMPLETE — [acceptance report](../reports/prompt_21.md#phase-d-acceptance). All 20 Release tests passed, including guarded one-shot writes, single-use readiness and preservation of outputs on timeout. |
| [49](49_audit-20-21.md) | Audit prompts 20–21 before extending synchronous features | COMPLETE — [acceptance report](../reports/prompt_49.md#phase-d-acceptance). All 20 Release suites passed, including fresh reads after recovery, one-shot guard renewal and deadline clipping across clock wrap. |
| [22](22_limits-alert.md) | Temperature limits, therm mode and ALERT | COMPLETE — [acceptance report](../reports/prompt_22.md#phase-d-acceptance). All 22 Release suites passed, including ordered limit writes, eight pin routes and single delivery of an alert after a failed one-shot TEMP read. |
| [23](23_offset-feature.md) | Opt-in hardware temperature offset | PARTIAL — [phase-b report](../reports/prompt_23.md#phase-b--unmanaged-offset-in-replay-and-verification). Guarded replay preserves unmanaged calibration and explicit verification checks its continuity. Managed writes, managed recovery and sample attribution remain open. |
| 24–27 | EEPROM reads, reset, diagnostics and synchronous API | NOT_STARTED |
| [50](50_audit-22-27.md) | Audit prompts 22–27 before cooperative work | NOT_STARTED |
| 28–33 | Cooperative owner operations and adversarial native tests | NOT_STARTED |
| [51](51_audit-28-33.md) | Audit prompts 28–33 before firmware and CLI integration | NOT_STARTED |
| 34–36 | Arduino, ESP-IDF and P4 integration | NOT_STARTED |
| [52](52_audit-34-36.md) | Audit prompts 34–36 with P4 evidence where available | NOT_STARTED |
| 37–39 | Shared CLI and Python HIL runner | NOT_STARTED |
| [53](53_audit-37-39.md) | Audit prompts 37–39 before physical qualification and packaging | NOT_STARTED |
| 40–41 | Physical HIL qualification (fixture-dependent) | NOT_STARTED; physical evidence NOT RUN |
| [54](54_audit-40-41.md) | Audit prompts 40–41 once physical evidence exists | NOT_STARTED; fixture-dependent |
| 42 | Package and documentation | NOT_STARTED |
| [55](55_audit-42.md) | Audit prompt 42 before CI and final closure | NOT_STARTED |
| 43 | First remote CI checkpoint | NOT_STARTED |
| 44 | Final independent audit | NOT_STARTED |

The planning review covered the original 44 prompts and shared contracts.
Stage 4 verified archive integrity and documented the six exact BOM models,
identity rules and board conditions. TMP119M remains outside the selected scope.
Stage 6 reviewed the TMP117 E2E DATA_READY capture and retained its conflicting
routing details. The exact clear edge and TEMP byte latching remain unknown.
The related TMP116 thread and optional TI source code were inaccessible.
The subsequent completeness audit adds full per-chip source inventories and
46 mandatory register/behavior/exclusion rows, with focused additions to
32 prompts. The audit established obligations that each stage verifies against
its own deliverables. Stage 25 now explicitly depends on
stage 23 for nonfactory offset reset/recovery tests.
The handoff audit adds `Produces`/`Consumes` declarations to all 44 prompts,
originally 60 artifact IDs and 226 required-input links. Stage 17 additionally requires
16 and stage 23 requires 21. Research, private engines, public integration,
firmware source/link evidence and physical gates now have explicit boundaries
in [HANDOFFS](HANDOFFS.md). No implementation-stage status changed.

The 2026-11-17 review rechecked all 68 later findings and every earlier audit,
with responses in each report. That checkpoint had 61 producer IDs and 263
required-input links, including the stage 12 reusable register fake. Shared
safety, acquisition, offset and integration contracts are reconciled; the
checker now validates coverage IDs/owners and pinned PDF page anchors.
The configured PlatformIO launcher selects the active Core rather than the
inactive default installation. Reference PDF names from `19facbe` remain
descriptive and the original bytes stay pinned. No driver, MCU, CI or physical
qualification stage was executed by this review.

The cb73740 follow-up checks all 42 submitted top-level items and their nested
requests. It reconciles startup-origin defaults/invalidation, one-shot order
and provenance, bounded guard polling, SDK tick/budget admission, fixture
authority and CLI producer boundaries. Prior response overclaims and auditor
citation errors are explicitly corrected. No new implementation stage was run.
That checkpoint retained 61 producer IDs and 263 required-input links.

The 2026-11-18 second follow-up checks all 17 new findings. Startup now uses
only internal FIRST_ACK evidence; Config seeds/caller timestamps are deferred.
Lifecycle/reset, no-wait guard paths and all-settings SD wording agree across
consumers. Fixture grants must be read directly. Stage 10's register-validator
input brings current metadata to 61 producer IDs and 264 required-input links.
SDK evidence names the fixed bus-clear wait and pre-conversion overflow cap.
No implementation, MCU, CI or physical qualification stage was executed.

The [report index](../reports/index.md) separates completed preparation from
stage-owned evidence. Final commit/upstream synchronization is recorded in the
task handoff; use Git history to identify the commit containing this checkpoint.
A stage may be COMPLETE only when its owned exit criteria are met. A required
physical stage remains BLOCKED when invoked without a fixture; software tests
and compiled firmware never substitute for physical evidence.
