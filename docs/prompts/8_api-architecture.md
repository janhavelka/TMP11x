# 8 — Freeze public API and variant architecture

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Architecture and package foundation**. Type: **software**.
Prerequisites: [1](1_ecosystem-core-audit.md), [2](2_tooling-peer-audit.md), [4](4_family-identity.md), [5](5_register-ledger.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md).
Produces: `api-contract`.
Consumes: `ecosystem-audit`, `tooling-plan`, `variant-contract`, `register-ledger`, `behavior-contract`, `safety-policy`, `feature-ledger`.

## Objective and boundaries

Choose one coherent driver model and no unnecessary shared foundation with TMP1x2. Document real type and method semantics before copying code.

## Source audit

Revisit `TMP1x2/include/TMP1x2/{Config,Status,TMP1x2}.h`, `src/TMP1x2*.cpp`, `OPT4001` IDs/freshness, ADS1115 dirty profile, and FieldCore-node current integration seams read-only. Verify source revisions and tests.

## Complete implementation work

- Create `docs/api-contracts.md` and `docs/architecture.md`. Select namespace `TMP11x`, class `TMP11x`, exact public file names, C++17, `Config` with selected `Model`, address, borrowed I2C callbacks, user/time/GPIO contexts, timeout and desired sensor settings.
- Define `Status`, health, identity/family confidence, sample provenance, snapshots, typed setters/getters, limits, offset and safe EEPROM reads; normal latest and confirmed-fresh methods distinct.
- Limit replayable desired mode to continuous or shutdown. One-shot and software reset are explicit operations, never persistent Config values reapplied by begin/recover. Freeze distinct latest, continuous try-fresh and owned one-shot acquisition requests for both synchronous and cooperative APIs.
- Freeze this managed one-shot sequence: local desired-shutdown/initialized/clean checks (reject before I2C) → 0x04 safety guard → read CONFIG and compare owned stable fields → guarded SD write → CONFIG stale-ready baseline → one trigger → readiness polls → TEMP. Preflight mismatch or unowned MOD=11 latches dirty with zero CONFIG writes; never repair drift before checking it. Desired mode stays shutdown. From a physically successful trigger, MOD=01/11 is compatible by owned-trigger provenance during the shot and after completion/cancel/timeout, until successful explicit SD or invalidation/rebind. A late physical trigger success retains provenance but cannot make the operation timely; an ambiguous write invalidates trust. MOD never proves completion or quiescence. Cancellation/timeout never silently restores or retriggers.
- Separate binding fields (model, address, callbacks/contexts, clock epoch and transport policy) from mutable sensor settings even if both live in `Config`. CONFIGURE/RECOVER and INITIALIZE jobs reuse an established binding; settings operations cannot replace it. Binding changes require explicit bus-silent rebind under the lifecycle/result rules.
- Freeze a bound-only diagnostic read route for permitted ID, TEMP, CONFIG and EEPROM-shadow registers without begin/recover or implicit settings writes. Normal managed APIs may require initialization; diagnostic results instead report unknown startup/configuration/freshness and explicit read effects. This route must make stage 40's register-write-free firmware possible without weakening managed guards.
- Map every register functionality to *one* owner (transport/identity/config/measurement/diagnostic). Choose one managed desired-profile replay engine and one cooperative operation vocabulary.
- Assign implementation ownership now: stages 12/13 deliver private transport/identity engines, 14 attaches public bound identity/probe routes, 18 implements real synchronous begin/recover and safe apply, 19 extends trust/readback, 20/22 add typed setting wrappers, and 23 extends apply with offset. Stages 29/30 reuse those engines for jobs. No stage depends on a successful placeholder.
- Specify failure-output policy, read-to-clear side effects and package eligibility for public APIs. Avoid inheriting TMP1x2 extended-mode API or including TMP1x2 as dependency.
- Freeze transport success as completion of the entire requested transfer. With the sibling's Status-only callback, the adapter must detect short reads/writes and return a terminal error; core code cannot infer a byte count from an unchanged buffer. Null opaque user context is valid for a stateless callback. Define clock requirements for timed operations separately from optional timestamps on latest-value reads.
- Freeze a per-operation state-gating table: bound-only diagnostics can report untrusted observations; managed latest reads require initialization but may report dirty/untrusted data without freshness. Try-fresh, blocking fresh/one-shot and their jobs require initialized, clean, verified owned settings and reject otherwise before I2C. Recovery can repair dirty/invalidated state but still requires a valid binding, a free operation/result slot and the shared startup/EEPROM guard. The preflight CONFIG comparison is before **any** CONFIG write, including SD, using the exact sequence above; mismatch latches dirty and refuses mutation.
- Define health independently of initialization: UNINIT means no tracked outcome since accepted bind/teardown, and tracked physical outcomes drive READY/DEGRADED/OFFLINE even before begin completes. Device-state invalidation does not itself erase transport history or count a failure. The bus-silent `isOnline()` convenience predicate is `initialized && (state == READY || state == DEGRADED)`; it is not a live probe or freshness/config-trust guarantee. Keep initialized and trust separate; document the health-transition deviation from TMP1x2.
- Freeze `StartupOrigin` as an initial Config seed: source FIRST_ACK by default (timestamp ignored), or explicit CALLER_TIMESTAMP in the authoritative clock's epoch. It is mutable device-state evidence, not an immutable transport-binding field or a replayable sensor setting. When startup proof is absent, FIRST_ACK anchors the policy wait at completion of the first successful compatible read in the current initialization/recovery operation, hence no earlier than operation start. A read proves the selected responder had power by then, not that POR finished. CALLER_TIMESTAMP must be nonfuture and unambiguous in the bounded wrap-safe time domain when consumed. **Both** sources require continuously stable power/no outside reset from the chosen origin through the operation. Stage 6 owns the finite wait policy, not a guaranteed POR maximum; retain established startup evidence until invalidated rather than re-waiting on every setter.
- Extend existing `invalidateDeviceState(StartupOrigin nextOrigin = FIRST_ACK)` at stage 19: bus-silently clear startup proof/old origin, initialization/config trust, freshness baseline and owned-trigger provenance; install the new seed, retain desired settings, transport history and historical alert evidence. Reject invalid seed or active/retained operation ownership without changing state. After reported power loss, stop using old trust, cancel/take any job first, invalidate with FIRST_ACK or a new caller timestamp, then recover under stable power. Accepted bind seeds this state, teardown clears it, and reset discards old proof; no separate origin setter is needed. Stage 14 validates seed structure and stage 18 validates clock/timestamp use.
- Settings/configure requests cannot replace the startup seed. Prefer settings-only input; if an existing full Config input is used, reject a changed startup seed before admission rather than silently ignoring it. This is a separate input-scope check, not proof that origin belongs to immutable transport binding. Initialization/recovery jobs consume the established seed and evidence.
- Specify acquisition kind, profile generation, configuration trust at acquisition and offset provenance in Sample alongside raw/counts/Celsius, timestamp validity and freshness. Observation timestamps do not claim conversion time; a later profile change cannot relabel a cached sample. The offset control and its populated provenance arrive at stage 23.
- Freeze explicit read semantics: readLatest, tryReadFresh/readFreshBlocking, and readOneShotBlocking (final spelling in the API contract), with a ReadKind selector for cooperative reads. Do not make a generic blocking read silently choose a write-producing one-shot from the current mode. Manual trigger/readiness helpers remain private to the shared acquisition engine; add public aliases only for a named consumer with identical semantics.
- Add explicit side-by-side `TMP1x2` and `TMP11x` consumer sketches sharing one I2C owner without any runtime inheritance.

- Freeze the final replayable profile: stable CONFIG fields, both signed limits and explicitly managed supported hardware offset. Offset management is opt-in and disabled by default; unmanaged TMP117/TMP119 initialization/recovery observes 0x07 as provenance but never writes or adopts it as a replay target. Managed zero is an explicit request, not the default. TMP116 rejects offset management before I2C and profile replay never touches its 0x07. Specify desired/observed/trusted representation, validation, write ordering, intermediate-alert limitations and recovery/readback of every owned field. Stages 20/22 add typed wrappers to the stage 18/19 engine; stage 23 extends it with offset.
- Make staged API availability explicit: the public configurable offset member/control is introduced in stage 23, not as an accepted default-zero field in stage 11. Before 23, ordinary Config supports binding plus stable CONFIG/limits, and successful initialization trusts only those owned fields; offset provenance stays unknown. This keeps stages 18–21 usable without silently ignoring an offset request. The stage 8 document describes the final API; source declarations follow their named feature producers.
- Map every included R/F row in [FULL_COVERAGE](FULL_COVERAGE.md) to a typed public operation or explicit integration/documentation responsibility. Map appropriate operations to jobs and CLI routes, with a reason for purely local/getter operations that need no job. Include capability refusals and excluded X rows; do not leave a register function reachable only through unsafe raw writes.

## Verification and exit criteria

Independent API/ownership review: no model-inappropriate functionality is exposed as success, no duplicate scheduler, no runtime SDK coupling, no hidden EEPROM write.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_8.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
