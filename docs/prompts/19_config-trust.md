# 19 — Implement observed configuration, verification and dirty state

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [15](15_passive-health.md), [17](17_config-codecs.md), [18](18_safe-initialize.md).

## Objective and boundaries

Track whether the requested volatile settings still match hardware after diagnostics, outside writes, ambiguous I2C errors and rebinds.

## Source audit

Inspect TMP1x2 `src/TMP1x2Configuration.cpp` CONFIG/threshold comparisons, dirty/recovery semantics and tests; compare ADS1115 readback and optional ownership discipline.

## Complete implementation work

- Add typed setters for supported stable settings and `readConfiguration`, `verifyConfiguration`, observed-versus-desired `SettingsSnapshot`, dirty reason and last successful verification time.
- A failed write attempt may reach the sensor: mark hardware config dirty even when callback returns NACK/timeout unless the failure demonstrably proves no effect. Preserve the user's desired target for recovery.
- A live read of changed stable fields or an explicitly reported external reset invalidates cached assumptions; monitoring status flags alone is not a stable-config mismatch. The driver cannot detect an unannounced outside writer or reset from a cache. Document exclusive device ownership and the caller's invalidation duty.
- Implement `invalidateDeviceState()` bus-silently and explicitly document effect on prior sample freshness and pending owner jobs.
- Centralize destructive-read bookkeeping for every CONFIG/TEMP path, including failed transfers that may already have cleared flags in hardware. CONFIG consumes DATA_READY and alert-mode flags, but therm-mode HIGH_Alert survives the read; TEMP consumes DATA_READY, not high/low alert flags. Verification must record these effects without treating them as configuration mismatches.
- Compare canonical stable configuration: MOD=10 reads as continuous MOD=00; one-shot is an operation that returns to shutdown, and Soft_Reset always reads zero. Exclude status/command bits from stable readback. See TMP116 p.26 Table 6, TMP117 pp.27-28 Table 7-6 and TMP119 pp.34-35 Table 8-5.

- Trust covers the entire desired profile, not only CONFIG. Observe/compare both limits and, once stage 23 lands, the supported offset; preserve per-field validity when only part was read. An unobserved field is unknown, and detecting a mismatch must not silently adopt it or overwrite the desired replay target. The coverage row F11 closes only when replay/readback includes every owned field.

## Verification and exit criteria

Test readback disagreement, external mutation, failure after first/middle write, no stale sample publication, and state recovery after verified replay. Readback must not blindly compare read-to-clear bits.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_19.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
