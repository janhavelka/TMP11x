# TMP11x numbered implementation promptset (1–44)

Run ordered stages in **numeric order**, not alphabetic filename order. This
promptset contains **plans**, not a built or validated TMP11x driver. Start with
[START_HERE.md](START_HERE.md) and load [COMMON.md](COMMON.md) for every stage.
The complete dependency manifest is [SEQUENCE.json](SEQUENCE.json). Resume from
[PROGRESS.md](PROGRESS.md), verifying completed artifacts/tests rather than
trusting status text alone.
The [promptset audit](../reports/promptset-audit.md) records the review of all
44 prompts and remaining source conflicts; it is not stage execution evidence.
The subsequent [feature-gap audit](../reports/feature-gap-audit.md) covers
all five pinned datasheets and fills register, behavior and integration gaps.
Its mandatory R/F/X requirements are in [FULL_COVERAGE](FULL_COVERAGE.md).

Each prompt is a bounded engineering work unit with evidence inputs, exact
scope, concrete code/documentation, adversarial tests and a report/commit
checkpoint. One stage does **not** authorize the next by default; the whole-set
invocation in START_HERE explicitly authorizes continuation.

## Shared working contracts

| File | Purpose |
|---|---|
| [COMMON.md](COMMON.md) | Required task intake, safe execution, blocked stages and commit/sync |
| [SOURCE_MAP.md](SOURCE_MAP.md) | Exact sibling GitHub repositories, source code routes and adopted behavior |
| [SILICON.md](SILICON.md) | Initial TI part comparison and flagged source contradictions |
| [EVIDENCE.md](EVIDENCE.md) | Primary PDF authority, hashes, literal vectors and source-status limits |
| [CONVENTIONS.md](CONVENTIONS.md) | Public type, status, model and I2C callback naming |
| [ENGINEERING.md](ENGINEERING.md) | EEPROM, freshness, ownership, conversion state and recovery rules |
| [FULL_COVERAGE.md](FULL_COVERAGE.md) | Complete per-silicon feature and per-variant capability obligations |
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
| 8 | [Freeze public API and variant architecture](8_api-architecture.md) | 1, 2, 4, 5, 6, 7 | software |
| 9 | [Freeze cooperative ownership, timing and result contract](9_owner-deadline-contract.md) | 8, 6, 7 | software |
| 10 | [Create the independent buildable library skeleton](10_package-foundation.md) | 2, 8, 9 | software |
| 11 | [Implement portable Status, model and value types](11_status-values.md) | 8, 9, 10 | software |
| 12 | [Implement bounded 16-bit I2C register transactions](12_i2c-register-transport.md) | 5, 8, 9, 10, 11 | software |
| 13 | [Implement identity validation and explicit model capabilities](13_identity-selection.md) | 4, 5, 8, 11, 12 | software |
| 14 | [Implement bus-silent binding and lifecycle state](14_binding-lifecycle.md) | 8, 9, 11, 12, 13 | software |
| 15 | [Implement tracked transport health and error observation](15_passive-health.md) | 8, 11, 12, 14 | software |
| 16 | [Implement signed temperature and threshold codecs](16_temperature-codec.md) | 5, 6, 11, 12, 13, 14, 15 | software |
| 17 | [Implement type-safe CONFIG codecs and validation](17_config-codecs.md) | 5, 6, 7, 8, 11, 12 | software |
| 18 | [Implement EEPROM-safe initialization and configuration writes](18_safe-initialize.md) | 7, 8, 9, 12, 13, 14, 16, 17 | software |
| 19 | [Implement observed configuration, verification and dirty state](19_config-trust.md) | 7, 8, 15, 17, 18 | software |
| 20 | [Implement continuous operation, averaging and cycle timing](20_conversion-settings.md) | 6, 8, 16, 17, 18, 19 | software |
| 21 | [Implement fresh-conversion acquisition and one-shot behavior](21_fresh-one-shot.md) | 6, 9, 16, 18, 19, 20 | software |
| 22 | [Implement temperature limits, therm/alert and ALERT pin](22_limits-alert.md) | 5, 6, 7, 16, 17, 19, 21 | software |
| 23 | [Implement hardware offset for TMP117/TMP119 only](23_offset-feature.md) | 4, 5, 7, 11, 12, 16, 17, 19 | software |
| 24 | [Implement safe EEPROM reads and traceability metadata](24_readonly-eeprom.md) | 4, 5, 7, 11, 12, 13, 19 | software |
| 25 | [Implement supported per-device reset and invalidation safely](25_per-device-reset.md) | 5, 7, 9, 14, 18, 19, 21, 23 | software |
| 26 | [Implement read-only register snapshots and guarded raw access](26_diagnostic-access.md) | 5, 7, 12, 13, 19, 21, 22, 23, 24 | software |
| 27 | [Complete synchronous API, samples and snapshots](27_sync-helpers-cache.md) | 16, 19, 20, 21, 22, 23, 24, 25, 26 | software |
| 28 | [Implement bus-silent jobs and exactly-once results](28_cooperative-admission.md) | 9, 14, 15, 27 | software |
| 29 | [Implement cooperative initialization, settings and recovery](29_cooperative-config.md) | 18, 19, 25, 28 | software |
| 30 | [Implement owner-driven fresh measurement and one-shot jobs](30_cooperative-measurement.md) | 20, 21, 22, 28, 29 | software |
| 31 | [Harden cancellation, ambiguous effects, timeout and health](31_failure-recovery.md) | 15, 19, 25, 28, 29, 30 | software |
| 32 | [Complete independent multi-model device register simulators](32_variant-model-fixtures.md) | 13, 17, 18, 21, 23, 24, 29, 30, 31 | software |
| 33 | [Close independent native correctness and packaging gates](33_native-suite.md) | 16, 17, 18, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 | software |
| 34 | [Implement Arduino/PlatformIO adapters and reusable example](34_arduino-examples.md) | 27, 30, 33 | software |
| 35 | [Implement native ESP-IDF examples and framework adapter](35_idf-examples.md) | 27, 30, 33 | software |
| 36 | [Qualify clean ESP32-P4 integration and shared-bus consumer](36_p4-consumer.md) | 33, 35 | software |
| 37 | [Create one shared bounded diagnostic CLI engine](37_cli-baseline.md) | 27, 28, 30, 33 | software |
| 38 | [Add full safe configuration, job and diagnostic CLI routes](38_cli-device-operations.md) | 22, 23, 24, 25, 26, 28, 29, 30, 31, 37 | software |
| 39 | [Implement correlated Python HIL runner and finite campaigns](39_python-hil-runner.md) | 37, 38 | software |
| 40 | [Qualify read-only TMP11x hardware identity and measurement](40_hil-readonly.md) | 39 | physical |
| 41 | [Qualify volatile settings, recovery and finite hardware soak](41_hil-volatile-soak.md) | 31, 39, 40 | physical |
| 42 | [Finish packaged consumers, public Doxygen and user documentation](42_package-docs.md) | 33, 34, 35, 38, 39 | software |
| 43 | [Run one dedicated remote CI integration checkpoint](43_ci-checkpoint.md) | 33, 35, 42 | ci |
| 44 | [Perform independent silicon/API/safety/consumer closure](44_final-independent-audit.md) | 31, 32, 33, 38, 39, 42 | software |

## Completion policy

Hardware prompts 40–41 are intentionally fixture-gated. If no real authorized
fixture exists, complete software/runner work and mark physical work BLOCKED or
NOT RUN; do not change software evidence to PASS. Firmware builds similarly
require actual installed target toolchains. Remote CI is reserved for prompt
43. Final stage 44 reconciles every required software and physical scope
without inventing completions or overwriting unresolved source conflicts.
Stage 40 additionally requires one fully linked, matching observation firmware
from stage 34 **or** 35 and an authorized fixture; it does not require both
frameworks for one hardware run. Stage 37's host CLI tests do not require an SDK.
