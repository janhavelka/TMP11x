# 9 — Freeze cooperative ownership, timing and result contract

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Architecture and package foundation**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md).
Produces: `owner-contract`.
Consumes: `api-contract`, `behavior-contract`, `safety-policy`.

## Objective and boundaries

Select exact semantics for bounded multi-turn operations, freshness, cancellation and ambiguous effects before implementation.

## Source audit

Inspect TMP1x2 `src/TMP1x2Operations.cpp`, `docs/owner-operations.md`, timeout tests; OPT4001 and ADS1115 owner interfaces. Study actual callback timeout/error mapping from peers.

## Complete implementation work

- Add `docs/owner-operations.md`: `startInitialize`, `startConfigure`, `startRead`, `startRecover`, `poll(nowMs,maxTransfers)`, `cancel`, `takeResult` with single active slot and retained terminal identity.
- Freeze callback timeout budget versus whole-job deadline, 32-bit wrap-safe timestamps, zero-transfer polls, unsupported operations and exactly-once result retention.
- Require an authoritative monotonic clock callback for deadline-bearing operations so the owner can re-sample time after a blocking transfer. An entry-only `poll(nowMs)` value cannot detect a callback that finishes late. Reject such operations without the required clock before I2C; any caller-supplied poll time must share the clock's epoch. Clip each transfer timeout to remaining budget, test a successful physical transfer completed after the job deadline, and keep that success separate from the timed-out operation result.
- Define wrap-safe duration bounds (at most INT32_MAX), expiry equality, deadline admission and token wrap/lifetime behavior. Use portable unsigned arithmetic rather than relying on out-of-range unsigned-to-signed conversion. Retained terminal results must not be silently lost through a subsequent admission or rebind; explicitly specify teardown's result policy.
- Distinguish conversion trigger, ready observation, cached latest and confirmed fresh result; retain read-to-clear status in an owner-visible observation rather than throwing it away.
- Define cancellation/timeout failure trust: no automatic hardware rollback, no duplicate job completion, no overwriting pending result, no reentrancy and no callback buffered past return.
- Carry stage 8's immutable-binding and shutdown-only one-shot admission rules into every operation request. Reject changed model/address/callback/clock binding before slot/token commitment or I2C. INITIALIZE operates on a bound instance. Specify in-flight MOD=11 ownership and the uncertain conversion evidence retained after cancellation/timeout; later reconciliation must not invent a mode change or retrigger.
- Specify rules for external multiplexer route selection between polls and temporary I2C owner lock (one sensor callback/transaction, never across sensor conversion wait). Route selection plus the sensor transaction must be serialized as one application-owned critical section. State whether transfer budgets count sensor callbacks only; extra mux transactions belong to the application's bus budget and cannot be advertised as sensor-core work.

- State that the sensor's 20–40 ms serial-interface timeout concerns its documented line-held/no-clock condition, not the application clock or job deadline. A long operation can span many short transfers; an adapter must neither rely on the sensor timeout as its software bound nor deliberately stretch a transaction to wait for conversion. Timing/electrical enforcement belongs to the bus owner.

## Verification and exit criteria

Use small native pseudocode/fake transfer traces to demonstrate one-read/one-write budget and no I2C on admission/cancel. Independent review checks ambiguous partial writes and operation-result lifetime.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_9.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
