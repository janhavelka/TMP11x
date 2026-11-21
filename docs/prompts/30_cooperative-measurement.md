# 30 — Implement owner-driven fresh measurement and one-shot jobs

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [20](20_conversion-settings.md), [21](21_fresh-one-shot.md), [22](22_limits-alert.md), [28](28_cooperative-admission.md), [29](29_cooperative-config.md).
Produces: `measurement-jobs`.
Consumes: `sync-acquisition`, `conversion-settings`, `limits-alert`, `job-lifecycle`, `configuration-jobs`, `register-fake`.

## Objective and boundaries

Provide cooperative sampling with correct readiness evidence across wait turns and interleaved other I2C activity.

## Source audit

Compare TI timing/data-ready/one-shot sections and TMP1x2 `startRead` job state machine, while replacing all TMP1x2 EM/conversion timing and format checks.

## Complete implementation work

- Reuse stage 21's acquisition engine for three explicit read kinds. LATEST requires initialization and reads TEMP once in either desired mode, including with dirty settings. Continuous fresh and owned shutdown one-shot each require verified clean settings and their own readiness baseline.
- Budget 0x04 safety guard, preflight CONFIG comparison, explicit SD write, CONFIG stale-ready baseline, one OS trigger, wait, CONFIG observation and TEMP read as distinct steps, each at most one physical callback. Preflight drift or unowned MOD=11 latches dirty state with zero CONFIG writes; only retained owned-trigger provenance permits normalization of residual OS to SD. Freshness-producing continuous jobs also check 0x04 and require their established current-generation baseline; latest-only jobs do not claim freshness. Apply stage 6/21's sparse default poll schedule. Bind evidence to request/profile and consume it once; ambiguous read failures can consume hardware evidence.
- Record host observation timestamp validity, freshness evidence and possible read-to-clear effects in the retained result. Every successful CONFIG read updates shared sticky alert evidence, including late callbacks and later-failed composites; result copies do not drain the explicit alert consumer. Recheck completion time before publishing: late successful reads update physical health/evidence but not outputs/cache/timely success. GPIO observation alone does not supply a new completion protocol, and no ISR may mutate the driver.
- Support other sensor instances using the same bus between polling turns, without holding locks or assuming pointer persistence.
- Reject or preserve uncertain freshness after configuration/diagnostic reads by outside masters or other instances, a missed event or reset. This instance's unrelated diagnostics are already BUSY-gated and cannot interleave with its job.

- Carry stage 6/21's accumulated-average, aborted-partial-block and TEMP coherence qualifications into cooperative results. Changes to limits/offset/settings use the shared profile-generation policy; no accepted job may relabel an earlier sample under a new profile or resolve unknown silicon semantics via a fake.

## Verification and exit criteria

Apply stage 21's shutdown-only managed one-shot policy unchanged: reject
desired continuous before admission, explicitly establish guarded SD then a
CONFIG baseline before trigger, retain desired shutdown regardless of the
undocumented during/after-OS MOD readback, and preserve
uncertain conversion evidence on cancel/timeout without mode restore/retrigger.
The preflight CONFIG comparison precedes the SD write; no acquisition silently
repairs unknown configuration drift. Retain alert events from every CONFIG read.
Run lifecycle/invalidation integration tests from stages 14/19/28 against real
measurement jobs, not only the private state-transition component.

Test interleaved two-sensor fake transport, one transfer per poll, 32-bit wrap, invalid/no clock, same temperature consecutive one-shots, missed ready and late successful read not published as timely result.
Test LATEST after a failed settings write leaves an initialized shutdown
target dirty. A timely TEMP yields dirty trust and `freshConversion=false`
after one callback. At expiry the same complete word appears only as
`lastRead` in the timed-out result. The historical cache remains intact.
Include immediate EUN refusal, busy-through-deadline/startup refusal without
CONFIG writes or fresh results, and timed busy-to-safe guard progression without
premature writes; retain per-poll transfer/work bounds. Also cover
both compatible post-OS MOD outcomes, residual conversion after cancellation,
continuous first-baseline suppression and one-time alert consumption.
Also test MOD=01/11 during readiness polling and after timeout/cancel under
retained acknowledged-trigger provenance; unowned MOD=11 remains drift. A
terminal physically successful trigger retains that provenance even when the
whole operation times out; it is not fresh evidence or permission to publish a
sample. Successful explicit SD or invalidation ends the exception; an ambiguous
SD write invalidates trust instead of proving shutdown.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_30.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
