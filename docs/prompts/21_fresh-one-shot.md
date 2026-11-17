# 21 — Implement fresh-conversion acquisition and one-shot behavior

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [6](6_timing-status.md), [9](9_owner-deadline-contract.md), [16](16_temperature-codec.md), [18](18_safe-initialize.md), [19](19_config-trust.md), [20](20_conversion-settings.md).
Produces: `sync-acquisition`.
Consumes: `behavior-contract`, `owner-contract`, `latest-read-engine`, `sync-initialize-recover`, `config-trust`, `conversion-settings`, `behavior-vectors`, `register-fake`.

## Objective and boundaries

Establish a trustworthy `freshConversion` contract despite clear-on-read DATA_READY and the initial -256°C placeholder.

## Source audit

Read TI data-ready clearing, one-shot trigger and timing sequences. Check OPT4001 `fresh` vs `latest` APIs and TMP1x2 manual one-shot logic only for API inspiration.

## Complete implementation work

- Implement internal one-shot trigger/readiness steps, public `tryReadFresh`, `readFreshBlocking` and `readOneShotBlocking` around stage 16's latest-read engine; do not add an ambiguous mode-dependent `readBlocking` or a separate public manual-trigger protocol. Use a private resumable engine with at most one physical callback per step; synchronous helpers drive those same steps, and stage 30 supplies the cooperative wrapper. Managed freshness requires clean verified owned settings and startup readiness. Managed one-shot requires desired shutdown; reject desired continuous locally with zero I2C.
- At the start of each mutating or freshness-producing acquisition, read 0x04 and require EUN=0/busy=0 under the shared safety policy. Before any OS CONFIG write, read current CONFIG and compare stable fields against the verified desired profile. A mismatch latches dirty state and returns with zero CONFIG writes; do not silently repair drift. An observed MOD=11 is compatible only with retained provenance of this instance's own prior trigger, including uncertain residual work after cancellation; unknown/unowned MOD=11 cannot be normalized. Then explicitly write stable desired CONFIG with MOD=01, read CONFIG to establish the stale-ready baseline, and write MOD=11 exactly once. These writes share the guard in the same exclusive operation. Any ambiguous failure stops the sequence; no automatic retry. The explicit SD write aborts owned residual work and does not change desired mode. TMP116 documents OS from SD (p.14); every family documents the SD abort.
- MOD readback during/after OS is undocumented. Track an owned trigger independently of MOD; after an owned DATA_READY-confirmed completion, observed 01/11 may match desired SD by provenance, never by itself. An unowned 11 is not exempt from drift. Cancel/timeout performs no restore/retrigger; the next explicit acquisition reestablishes the guarded SD baseline.
- Continuous try-fresh needs a current-generation baseline plus trusted profile. The first owned CONFIG/TEMP baseline read cannot publish fresh data; later ready evidence can be consumed once. Profile-changing writes use stage 20's explicit SD/apply/baseline/CC/final-CONFIG-readback sequence. Final readback must match stable fields before trust; its consumed readiness establishes a new baseline, while alert evidence is retained. Read-only try-fresh never starts or reconfigures conversions to obtain a baseline.
- Before stage 23 extends the profile, report a fresh conversion under the known CONFIG/limits while retaining unknown hardware-offset provenance. Freshness does not assert a zero offset or calibrated reading. Stage 23 connects offset trust/generation to this same acquisition engine.
- Preserve a successfully captured ready event only while it still belongs to an unconsumed result of the same generation. Diagnostic TEMP reads, outside access, reset or ambiguous failed CONFIG/TEMP reads can consume evidence; invalidate it conservatively rather than duplicating a fresh sample. A lost one-shot ready event must not cause an automatic retrigger.
- Every successful CONFIG read feeds stage 19's sticky alert-evidence bookkeeping before any later failure, timeout or output suppression. A later zero flag must not erase an unconsumed event; successful diagnostic/alert reads likewise feed same-generation readiness bookkeeping. Failed destructive reads mark possible loss. Stage 22 owns explicit event consumption. CONFIG polling can release comparator ALERT; document this interaction instead of promising an interrupt stays asserted until a separate alert call.
- Disallow prematurely returning initial `0x8000` as fresh before completed conversion including averaging; support a read of raw reset value for diagnostics.
- Blocking helpers require a usable monotonic clock; a yield hook alone is not a clock. Clip each callback to the remaining operation deadline, check time again after it, and bound non-advancing-clock behavior. Yield is optional application cooperation and must not re-enter this instance. Do not hold an I2C mutex while waiting.
- Distinguish a documented finite work/poll cap from the clock deadline; repeated reads of one millisecond tick are normal, not proof of a stalled clock. Exhausting the work cap is an explicit failure, never fabricated conversion completion.
- Explicitly model simultaneous diagnostics, one-shot trigger, shutdown, averaging change and bus access failures.

- State the TEMP-word coherence evidence boundary from F08: combined word transfer is required, but continuous-update byte latching is not explicitly documented. Assess a completed owned one-shot's stable shutdown result separately. Tests may explore update/read interleavings, but must not certify an invented latch or silently add rereads/retries.
- Cover aborted partial averages and stale retained TEMP after shutdown or profile change. Do not assume shutdown clears all old status; the owned acquisition sequence must establish its own baseline.
- Use stage 6's sparse, maximum-based default OS poll schedule and explicit latency alternatives, especially TMP119's bus-activity precision caveat. Missing DATA_READY ends with an explicit timeout/not-ready outcome and no fresh result; an optional later latest read remains unconfirmed. An inaccessible E2E thread does not authorize a time-only freshness fallback, new GPIO completion semantics or automatic retrigger.

## Verification and exit criteria

Native fake-sensor tests exercise pre-trigger stale ready, ready cleared by
successful or ambiguous failed reads, repeated equal temperatures with new
conversions, no duplicate consumption, missed one-shot events without retrigger,
initial reset value, averaging completion, stopped clocks and late callback
completion. Timing estimates alone must never manufacture freshness.
Test every AVG setting and CONV independence, both post-OS MOD readback
outcomes, continuous first-baseline suppression, and alert events retained
exactly once across all acquisition reads. EUN=1, busy=1 or unknown startup
must yield zero CONFIG writes and no fresh result. Late successful TEMP reads
must not update outputs/cache, while their actual transport/read effects remain.
Preflight CONFIG drift or unowned MOD=11 must also yield zero CONFIG writes;
retained owned-trigger provenance permits explicit SD abortion of residual work.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_21.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
