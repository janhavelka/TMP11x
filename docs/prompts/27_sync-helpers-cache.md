# 27 — Complete synchronous API, samples and snapshots

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [16](16_temperature-codec.md), [19](19_config-trust.md), [20](20_conversion-settings.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md), [25](25_per-device-reset.md), [26](26_diagnostic-access.md).

## Objective and boundaries

Make the simple blocking user-facing API fully functional and consistent before integrating owner jobs and CLI.

## Source audit

Audit TMP1x2 `readTemperature`, `readBlocking`, getters/cached sample, settings and health snapshots, OPT4001 timing helpers, and CMake application consumer.

## Complete implementation work

- Provide stable conventional public method names and aliases only when their behavior is identical. Ensure latest/try-fresh/blocking semantics are unambiguous in signatures and documentation.
- Implement `getLastSample`, timestamp validity, wrap-safe age check and unit helpers. Cached sample age is NOT proof of newly completed continuous conversion.
- Make all getters either explicitly cached/bus-silent or explicitly live I2C, without hidden side effects. Provide `getSettingsSnapshot` and `readRegisterSnapshot` separate from source live read state.
- Test simple initialization, calibration, limits, offset and read on one TMP117 plus at least one TMP116 and TMP119 identity fixture through public API.
- Provide a minimal framework-neutral C++ consumer file that uses injected callbacks; no CLI/Arduino dependencies.

## Verification and exit criteria

Run native public API and compile consumers with all supported models, no callback/clock cases, and complete test registration. Update Doxygen for all public entry points.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_27.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
