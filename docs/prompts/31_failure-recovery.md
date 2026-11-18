# 31 — Harden cancellation, ambiguous effects, timeout and health

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [15](15_passive-health.md), [19](19_config-trust.md), [25](25_per-device-reset.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md), [30](30_cooperative-measurement.md).
Produces: `fault-closure`.
Consumes: `transport-health`, `config-trust`, `device-reset`, `job-lifecycle`, `configuration-jobs`, `measurement-jobs`, `register-fake`.

## Objective and boundaries

Close adversarial failure paths so unattended devices can rejoin after I2C errors or power cycles without silent incorrect samples.

## Source audit

Revisit TMP1x2 owner/dirty/health/timeout fault tests, ADS1115 indeterminate write cases and exact IO callback implementation. Check error fallback behavior of Arduino and IDF adapters only as planned, not yet production.

## Complete implementation work

- Inject I2C_NACK, TIMEOUT, BUS and malformed terminal-callback IN_PROGRESS during every READ/CONFIG/RECOVER phase. With Status-only callbacks, the adapter must map a short transfer to an error: a core cannot detect a callback that falsely reports success without exposing byte counts. Test that boundary in mock adapters now and real adapters at stages 34/35.
- Distinguish tracked bus health, whole-operation timeout, failed precondition, unsupported variant, EEPROM unsafe, and configuration dirty status. Counter saturation and last error preservation must be deterministic.
- Harden the synchronous recovery delivered in stage 18 and cooperative recovery from 29 after outside reset/power interruption: require identity/locked-state proof before writes; preserve desired settings; do not automatically reset the I2C bus. This stage adds adversarial closure, not the first working recover API.
- Power-loss tests stop using prior trust immediately, cancel/take any owned result, call `invalidateDeviceState()`, restore stable power and explicitly recover. Test clearing old internal startup/one-shot proof, preserving binding/desired/health/history, BUSY rejection, power restored after bind and recovery anchored at its first successful compatible read. End/begin and driver-issued/reported resets must also discard old proof. Reusing a valid settings Config after invalidation has no startup-seed comparison. Retain the stable-power/no-reset assumption through dependent operations.
- Cancellation retains terminal result and uncertain write effects, plus possible destructive-read consumption. Failed CONFIG/TEMP reads can lose status evidence without altering settings. Keep those two uncertainties separate; no automatic retrigger, write replay or fake rollback.
- Verify polling/reading does not silently consume a retained terminal result; unrelated I2C stays BUSY until consumption. Reject invalid relative deadlines. Unsigned elapsed >= timeout expires, including an apparent backward step; a gap of 2^32 ms or more cannot be detected from a 32-bit clock and is an owner obligation, not a claimed runtime check.
- Exercise late physical successes for every phase: retain transport health, observed words, sticky alert/possible-loss records and possible write effects, while refusing timely sample/cache publication, initialization or dirty-state clearance. The selected API retains but ignores caller `nowMs`; vary it independently to prove hook-controlled scheduling/deadlines/timestamps. A physically successful late trigger retains owned-trigger provenance; an ambiguous trigger cannot establish clean trust.

## Verification and exit criteria

Use independent reviewer/fault matrix; run all registered host fault suites with strict warnings, active asserts and relevant sanitizers if available. Capture all open gaps.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_31.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
