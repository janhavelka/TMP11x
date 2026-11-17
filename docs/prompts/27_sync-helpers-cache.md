# 27 — Complete synchronous API, samples and snapshots

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [16](16_temperature-codec.md), [19](19_config-trust.md), [20](20_conversion-settings.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md), [25](25_per-device-reset.md), [26](26_diagnostic-access.md).
Produces: `sync-api`.
Consumes: `latest-read-engine`, `config-trust`, `conversion-settings`, `sync-acquisition`, `limits-alert`, `offset-feature`, `readonly-eeprom`, `device-reset`, `diagnostics`, `register-fake`.

## Objective and boundaries

Make the simple blocking user-facing API fully functional and consistent before integrating owner jobs and CLI.

## Source audit

Audit TMP1x2 `readTemperature`, `readBlocking`, getters/cached sample, settings and health snapshots, OPT4001 timing helpers, and CMake application consumer.

## Complete implementation work

- Provide the distinct latest, continuous fresh and owned one-shot methods frozen in stage 8, including explicitly named blocking variants. No mode-dependent read silently triggers a conversion; manual trigger/readiness helpers stay private to the engine. Add an alias only for a named consumer and identical semantics.
- Implement `getLastSample`, timestamp validity, wrap-safe age check and unit helpers. Use the authoritative clock and a valid same-epoch sample timestamp for age; otherwise return unknown age. A timestamp records host acquisition/observation, not physical conversion time; cached age is not proof of a new conversion. Preserve acquisition kind, original profile generation, trust and offset provenance in cached samples. Define stale-cache access after reconfiguration/reset separately from a current-generation valid sample.
- Make all getters either explicitly cached/bus-silent or explicitly live I2C, without hidden side effects. Provide `getSettingsSnapshot` and `readRegisterSnapshot` separate from source live read state.
- Test initialization, limits, offset and read through public API on TMP117, TMP116 and TMP119 fixtures. Offset on TMP116 must reject before I2C. Setting an offset is not a complete calibration procedure or certified accuracy evidence; do not add an unplanned calibration API.
- Provide a minimal framework-neutral C++ consumer file that uses injected callbacks; no CLI/Arduino dependencies.

- Reconcile every included register/feature row in [FULL_COVERAGE](FULL_COVERAGE.md) against the actual public API and snapshots. Verify full-profile settings/limits/offset replay, explicit readback validity and absence of accidental raw-only features. Record the later job/CLI owner for each appropriate operation and justified not-applicable routes.

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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_27.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
