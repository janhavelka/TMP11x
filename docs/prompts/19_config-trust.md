# 19 — Implement observed configuration, verification and dirty state

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [15](15_passive-health.md), [17](17_config-codecs.md), [18](18_safe-initialize.md).
Produces: `config-trust`.
Consumes: `safe-apply-engine`, `sync-initialize-recover`, `config-codec`, `transport-health`, `safety-policy`, `behavior-vectors`, `register-fake`.

## Objective and boundaries

Track whether the requested volatile settings still match hardware after diagnostics, outside writes, ambiguous I2C errors and rebinds.

## Source audit

Inspect TMP1x2 `src/TMP1x2Configuration.cpp` CONFIG/threshold comparisons, dirty/recovery semantics and tests; compare ADS1115 readback and optional ownership discipline.

## Complete implementation work

- Extend stage 18's shared apply/readback/trust machinery with `readConfiguration`, `verifyConfiguration`, observed-versus-desired `SettingsSnapshot`, dirty reason and last verification time. Stages 20 and 22 own the typed mode/cycle/AVG and limit/alert convenience setters; those wrappers reuse this engine instead of duplicating writes or replay.
- A failed write attempt may reach the sensor: mark hardware config dirty even when callback returns NACK/timeout unless the failure demonstrably proves no effect. Preserve the user's desired target for recovery.
- A live read of changed stable fields or an explicitly reported external reset invalidates cached assumptions; monitoring status flags alone is not a stable-config mismatch. The driver cannot detect an unannounced outside writer or reset from a cache. Document exclusive device ownership and the caller's invalidation duty.
- Implement `invalidateDeviceState()` bus-silently for existing samples/trust, and define its pending-job/result policy for stage 28 to integrate and execute once slots exist.
- Centralize destructive-read bookkeeping for every CONFIG/TEMP path, including failed transfers that may already have cleared flags. Retain captured alert-mode HIGH/LOW evidence across later zero observations until explicit consumption, with mode/profile provenance and a possible-history-loss marker on ambiguity. Preserve historical pending alerts across generation changes; therm HIGH is a level observation, not the same sticky event. CONFIG consumes DATA_READY and alert-mode flags, whereas TEMP consumes only DATA_READY. Record these effects even on a late successful read without mislabeling a configuration mismatch.
- Compare canonical stable configuration: MOD=10 reads as MOD=00; Soft_Reset reads zero. Exclude status/command bits from stable readback. MOD during/after one-shot is undocumented: after owned DATA_READY completion, 01 or 11 can be compatible with desired shutdown, but neither proves completion/quiescence. Do not normalize unrelated observed mode away. Stage 21 explicitly establishes shutdown and a readiness baseline before every trigger, and compares all owned stable CONFIG fields before proceeding. See TMP116 p.26 Table 6, TMP117 pp.27-28 Table 7-6 and TMP119 pp.34-35 Table 8-5.

- Trust covers the entire owned desired profile, not only CONFIG. Observe/compare both limits and, once stage 23 lands, explicitly managed offset; unmanaged offset is observed provenance, never a replay target. Preserve per-field validity when only part was read. An unobserved owned field is unknown, and detecting mismatch must not silently adopt it or overwrite the target. F11 closes only when replay/readback includes every owned field.
- Apply stage 8's state gates: dirty/invalidated state refuses freshness-producing calls/jobs before I2C; initialized latest reads may return explicitly untrusted data. Recovery remains subject to binding, operation/result ownership and safety checks. A late successful read may update observations/alerts but cannot clear dirty, complete initialization or publish/cache a sample. No trust check silently repairs hardware as part of acquisition.

## Verification and exit criteria

Retained alert evidence spanning profile generations or mode changes must
carry a bounded mixed/unknown-generation marker. Never relabel old sticky
events as belonging only to the latest profile; consume them only explicitly.

Test readback disagreement, external mutation, failure after first/middle write, no stale sample publication, and state recovery after verified replay. Readback must not blindly compare read-to-clear bits.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_19.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
