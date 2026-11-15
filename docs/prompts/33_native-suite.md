# 33 — Close independent native correctness and packaging gates

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [16](16_temperature-codec.md), [17](17_config-codecs.md), [18](18_safe-initialize.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md), [25](25_per-device-reset.md), [26](26_diagnostic-access.md), [27](27_sync-helpers-cache.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md), [30](30_cooperative-measurement.md), [31](31_failure-recovery.md), [32](32_variant-model-fixtures.md).

## Objective and boundaries

Deliver one reproducible, executable, high-coverage native test suite rather than a collection of unregistered partial tests or documentation-only claims.

## Source audit

Check how TMP1x2 registers host test jobs and CMake/CTest checks (`test/`, `tools/check_contracts.py`, `docs/testing.md`), compare OPT4001 and ADS1115 native tests, and inspect this new repo's actual source and variant fake.

## Complete implementation work

- Register and run all earlier unit tests for every supported part, ensuring a nonzero test count, active assertions under optimized/Release builds, proper stdout/exit checks and finite test timeouts.
- Add independently derived literal vectors for signed measurements, all CONFIG field encodings, address and identity masks, read side effects, default/reset ambiguity, revision acceptance, EEPROM safety and exact error outputs. Tests must not just call the production codec to generate expected bytes.
- Prove all public headers can compile separately without Arduino/ESP-IDF. Build with a second compiler where available and with warning flags, C++17 mode, and sanitizers when supported. Do not weaken tests to make an unsupported compiler appear passing.
- Add deterministic property/negative tests for bad enums, timeout budgets, I2C callback failures, partial writes, token exhaustion, callback timing, dirty trust and unexpected register content.
- Track time/call counts and prove the core performs no hidden heap allocations, bus resets, waits, logging or global I2C initialization. Keep fakes and test-only clocks outside runtime package.
- Run a tests-OFF source/package consumer against the actual exported headers and library, including a checkout/archive with no sibling paths or reference PDFs. Record separately any install/export relocation gate deferred to stage 42; a native build-tree link is not installed-package qualification.

## Verification and exit criteria

Capture actual `cmake`, `ctest`, compiler, sanitizer, public-header and native consumer commands, count, duration/exit outcomes and unresolved failures in the stage report. All required executing cases must pass to call this stage Complete.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_33.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
