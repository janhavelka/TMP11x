# 15 — Implement tracked transport health and error observation

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [11](11_status-values.md), [12](12_i2c-register-transport.md), [14](14_binding-lifecycle.md).
Produces: `transport-health`.
Consumes: `api-contract`, `value-types`, `register-transport`, `binding-api`, `register-fake`.

## Objective and boundaries

Make failure telemetry useful for unattended devices without letting it silently control retry, bus recovery or sensor freshness.

## Source audit

Read TMP1x2 `src/TMP1x2.cpp`, `Status.h`, `HealthSnapshot` tests; compare OPT4001 offline behavior and ADS1115 transport status distinction, choosing one documented TMP11x contract.

## Complete implementation work

- Implement `UNINIT/READY/DEGRADED/OFFLINE`, `HealthSnapshot`, last error/time validity, consecutive failure, saturating total failure/success and optional offlineThreshold.
- Count only actual tracked physical I2C outcomes as transport failures/successes. Invalid parameters, unsupported model, not-ready, configuration mismatch and owner deadline are not fabricated physical transfers.
- Apply tracked-outcome state transitions independently of initialized state, including startup/apply attempts. UNINIT is the initial state after accepted bind/teardown; invalidating device/configuration trust alone preserves physical health history. Document this deliberate difference from TMP1x2's initialization-gated state updates, and keep isOnline/initialized semantics distinct.
- Keep `OFFLINE` passive: it does not block an otherwise authorized normal tracked operation, probe or recovery. The application owns backoff; only tracked successes affect tracked health. A raw health-neutral diagnostic path must still surface transfer failure directly and must not repair health merely because it succeeds.
- Test no counted preconditions, multi-transfer operations, device disappearance, first tracked success, an ordinary tracked success from OFFLINE, raw success leaving health unchanged, count saturation, timestamp 0 validity, clock wrap and repeated errors. Keep binding/initialized state separate; prompt 18 completes initialization-health integration without a fake successful begin here.
- Use the private tracked transport seam and stage 14's public health-neutral probe for these tests now. Stage 16/18 integrate managed sample/initialization paths; stage 26 repeats health-neutral assertions for its broader raw diagnostics when those routes exist.

## Verification and exit criteria

Maintain health documentation and external consumer examples. A successful read does not assert desired configuration or new conversion freshness.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_15.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
