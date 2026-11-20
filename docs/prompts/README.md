# TMP11x implementation and audit promptset

Use the `prompts` array in [SEQUENCE.json](SEQUENCE.json) to select the first
eligible unfinished stage. The table below lists the same execution order.
IDs 1–44 identify the original prompts. IDs 45–55 identify interleaved audit
checkpoints. Start with [START_HERE.md](START_HERE.md), load
[COMMON.md](COMMON.md) for each stage, and verify actual artifacts and tests
when resuming from [PROGRESS.md](PROGRESS.md).
The [promptset audit](../reports/promptset-audit.md) records the planning review
of the original 44 prompts and remaining source conflicts.
The subsequent [feature-gap audit](../reports/feature-gap-audit.md) covers
all five pinned datasheets and fills register, behavior and integration gaps.
Its mandatory R/F/X requirements are in [FULL_COVERAGE](FULL_COVERAGE.md).
The [handoff inventory](HANDOFFS.md) maps every stage's usable outputs and
integration boundaries. `Produces`/`Consumes` IDs in SEQUENCE and each prompt
are checked for unique ownership, known inputs and dependency reachability.
See the [handoff consistency audit](../reports/handoff-audit.md) for corrected
missing producers, future-stage requirements and integration/evidence gaps.
The [2026-11-17 finding responses](audit.md#response--2026-11-17) assess every
claim in the later 68-finding audit and record applied/qualified/rejected fixes.

Each prompt is a bounded work unit with evidence inputs, verification and a
report/commit checkpoint. Audit prompts name the original range, require
parallel reviewers and scope fixes to those prompts. A single-stage invocation
ends at its checkpoint. Use the whole-set invocation in START_HERE to continue.

## Shared working contracts

| File | Purpose |
|---|---|
| [COMMON.md](COMMON.md) | Required task intake, safe execution, blocked stages and commit/sync |
| [HANDOFFS.md](HANDOFFS.md) | Every producer/consumer boundary, artifact ownership and conditional evidence gates |
| [SOURCE_MAP.md](SOURCE_MAP.md) | Exact sibling GitHub repositories, source code routes and adopted behavior |
| [SILICON.md](SILICON.md) | Initial TI part comparison and flagged source contradictions |
| [EVIDENCE.md](EVIDENCE.md) | Primary PDF authority, hashes, literal vectors and source-status limits |
| [CONVENTIONS.md](CONVENTIONS.md) | Public type, status, model and I2C callback naming |
| [ENGINEERING.md](ENGINEERING.md) | EEPROM, freshness, ownership, conversion state and recovery rules |
| [FULL_COVERAGE.md](FULL_COVERAGE.md) | Complete per-silicon feature and per-variant capability obligations |
| [COVERAGE.md](COVERAGE.md) | Non-normative navigation summary; FULL_COVERAGE owns exact rows and stages |
| [VERIFICATION.md](VERIFICATION.md) | Native, firmware, HIL, package, docs and CI evidence gates |
| [CLI_CONTRACT.md](CLI_CONTRACT.md) | Bounded shared console commands and HIL record format |
| [DELEGATION.md](DELEGATION.md) | Disjoint subagent delegation with lead integration |
| [PROGRESS.md](PROGRESS.md) | Actual stage status, preparation evidence and remaining gates |

## Ordered prompts

| # | Complete task | Requires | Type |
|---:|---|---|---|
| 1 | [Audit current sensor-driver ownership and APIs](1_ecosystem-core-audit.md) | — | software |
| 2 | [Audit build, examples, prompt and test infrastructure](2_tooling-peer-audit.md) | 1 | software |
| 3 | [Acquire and pin official TI source documents](3_ti-reference-intake.md) | 1, 2 | software |
| 4 | [Freeze included models, IDs and capability classes](4_family-identity.md) | 3 | software |
| 5 | [Create exhaustive per-variant register and bitfield ledger](5_register-ledger.md) | 3, 4 | software |
| 6 | [Verify conversion, averaging, freshness and alert timing](6_timing-status.md) | 3, 5 | software |
| 7 | [Set nonvolatile, identity and reset safety policy](7_eeprom-reset-policy.md) | 3, 4, 5, 6 | software |
| 45 | [Audit preparation and silicon evidence (prompts 1–7)](45_audit-1-7.md) | 1, 2, 3, 4, 5, 6, 7 | audit |
| 8 | [Freeze public API and variant architecture](8_api-architecture.md) | 1, 2, 4, 5, 6, 7, 45 | software |
| 9 | [Freeze cooperative ownership, timing and result contract](9_owner-deadline-contract.md) | 8, 6, 7 | software |
| 46 | [Audit API and owner contracts (prompts 8–9)](46_audit-8-9.md) | 8, 9 | audit |
| 10 | [Create the independent buildable library skeleton](10_package-foundation.md) | 2, 8, 9, 46 | software |
| 11 | [Implement portable Status, model and value types](11_status-values.md) | 8, 9, 10 | software |
| 12 | [Implement bounded 16-bit I2C register transactions](12_i2c-register-transport.md) | 5, 8, 9, 10, 11 | software |
| 13 | [Implement identity validation and explicit model capabilities](13_identity-selection.md) | 4, 5, 8, 11, 12 | software |
| 14 | [Implement bus-silent binding and lifecycle state](14_binding-lifecycle.md) | 8, 9, 11, 12, 13 | software |
| 15 | [Implement tracked transport health and error observation](15_passive-health.md) | 8, 11, 12, 14 | software |
| 16 | [Implement signed temperature and threshold codecs](16_temperature-codec.md) | 5, 6, 11, 12, 13, 14, 15 | software |
| 17 | [Implement type-safe CONFIG codecs and validation](17_config-codecs.md) | 5, 6, 7, 8, 11, 12, 16 | software |
| 47 | [Audit library foundation and read path (prompts 10–17)](47_audit-10-17.md) | 10, 11, 12, 13, 14, 15, 16, 17 | audit |
| 18 | [Implement EEPROM-safe initialization and configuration writes](18_safe-initialize.md) | 7, 8, 9, 12, 13, 14, 16, 17, 47 | software |
| 19 | [Implement observed configuration, verification and dirty state](19_config-trust.md) | 7, 8, 15, 17, 18 | software |
| 48 | [Audit safe initialization and configuration trust (prompts 18–19)](48_audit-18-19.md) | 18, 19 | audit |
| 20 | [Implement continuous operation, averaging and cycle timing](20_conversion-settings.md) | 6, 8, 16, 17, 18, 19, 48 | software |
| 21 | [Implement fresh-conversion acquisition and one-shot behavior](21_fresh-one-shot.md) | 6, 9, 16, 18, 19, 20 | software |
| 49 | [Audit conversion settings and freshness (prompts 20–21)](49_audit-20-21.md) | 20, 21 | audit |
| 22 | [Implement temperature limits, therm/alert and ALERT pin](22_limits-alert.md) | 5, 6, 7, 16, 17, 19, 21, 49 | software |
| 23 | [Implement hardware offset for TMP117/TMP119 only](23_offset-feature.md) | 4, 5, 7, 11, 12, 16, 17, 19, 21, 49 | software |
| 24 | [Implement safe EEPROM reads and traceability metadata](24_readonly-eeprom.md) | 4, 5, 7, 11, 12, 13, 19, 49 | software |
| 25 | [Implement supported per-device reset and invalidation safely](25_per-device-reset.md) | 5, 7, 9, 14, 18, 19, 21, 23 | software |
| 26 | [Implement read-only register snapshots and checked diagnostic access](26_diagnostic-access.md) | 5, 7, 12, 13, 19, 21, 22, 23, 24 | software |
| 27 | [Complete synchronous API, samples and snapshots](27_sync-helpers-cache.md) | 16, 19, 20, 21, 22, 23, 24, 25, 26 | software |
| 50 | [Audit synchronous features and diagnostics (prompts 22–27)](50_audit-22-27.md) | 22, 23, 24, 25, 26, 27 | audit |
| 28 | [Implement bus-silent jobs and exactly-once results](28_cooperative-admission.md) | 9, 14, 15, 27, 50 | software |
| 29 | [Implement cooperative initialization, settings and recovery](29_cooperative-config.md) | 18, 19, 25, 28 | software |
| 30 | [Implement owner-driven fresh measurement and one-shot jobs](30_cooperative-measurement.md) | 20, 21, 22, 28, 29 | software |
| 31 | [Harden cancellation, ambiguous effects, timeout and health](31_failure-recovery.md) | 15, 19, 25, 28, 29, 30 | software |
| 32 | [Complete independent multi-model device register simulators](32_variant-model-fixtures.md) | 13, 17, 18, 21, 23, 24, 29, 30, 31 | software |
| 33 | [Close independent native correctness and source-consumer gates](33_native-suite.md) | 16, 17, 18, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 | software |
| 51 | [Audit cooperative operations and native qualification (prompts 28–33)](51_audit-28-33.md) | 28, 29, 30, 31, 32, 33 | audit |
| 34 | [Implement Arduino/PlatformIO adapters and reusable example](34_arduino-examples.md) | 27, 30, 33, 51 | software |
| 35 | [Implement native ESP-IDF examples and framework adapter](35_idf-examples.md) | 27, 30, 33, 51 | software |
| 36 | [Qualify clean ESP32-P4 integration and shared-bus consumer](36_p4-consumer.md) | 33, 35 | software |
| 52 | [Audit firmware adapters and P4 integration (prompts 34–36)](52_audit-34-36.md) | 34, 35 | audit |
| 37 | [Create one shared bounded diagnostic CLI engine](37_cli-baseline.md) | 27, 28, 30, 33, 51 | software |
| 38 | [Add full safe configuration, job and diagnostic CLI routes](38_cli-device-operations.md) | 22, 23, 24, 25, 26, 28, 29, 30, 31, 37 | software |
| 39 | [Implement correlated Python HIL runner and finite campaigns](39_python-hil-runner.md) | 37, 38 | software |
| 53 | [Audit CLI and HIL runner (prompts 37–39)](53_audit-37-39.md) | 37, 38, 39 | audit |
| 40 | [Qualify read-only TMP11x hardware identity and measurement](40_hil-readonly.md) | 39, 53 | physical |
| 41 | [Qualify volatile settings, recovery and finite hardware soak](41_hil-volatile-soak.md) | 31, 39, 40 | physical |
| 54 | [Audit physical qualification evidence (prompts 40–41)](54_audit-40-41.md) | 40, 41 | audit |
| 42 | [Finish packaged consumers, public Doxygen and user documentation](42_package-docs.md) | 33, 34, 35, 38, 39, 52, 53 | software |
| 55 | [Audit package and documentation (prompt 42)](55_audit-42.md) | 42 | audit |
| 43 | [Run one dedicated remote CI integration checkpoint](43_ci-checkpoint.md) | 33, 35, 42, 55 | ci |
| 44 | [Perform independent silicon/API/safety/consumer closure](44_final-independent-audit.md) | 31, 32, 33, 38, 39, 42, 55 | audit |

## Completion policy

Hardware prompts 40–41 and physical evidence audit 54 are fixture-gated.
If the required evidence is unavailable, record the physical work as BLOCKED
or NOT RUN and continue eligible software work. Firmware builds require
the matching target toolchains. Stage 43 owns the first deliberate remote CI
checkpoint. Stage 44 reruns that procedure for CI-covered fixes before closure
and reconciles required software and physical evidence.
Stage 40 additionally requires one current fully linked observation image from
a stage 34 **or** 35 backend after the final stage 38 CLI integration, matching
the runner protocol/build identity, and an authorized fixture. It does not
require both frameworks. Stage 37's host CLI tests do not require an SDK.
