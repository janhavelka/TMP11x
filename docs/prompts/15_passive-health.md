# 15 — Implement tracked transport health and error observation

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [11](11_status-values.md), [12](12_i2c-register-transport.md), [14](14_binding-lifecycle.md).

## Objective and boundaries

Make failure telemetry useful for unattended devices without letting it silently control retry, bus recovery or sensor freshness.

## Source audit

Read TMP1x2 `src/TMP1x2.cpp`, `Status.h`, `HealthSnapshot` tests; compare OPT4001 offline behavior and ADS1115 transport status distinction, choosing one documented TMP11x contract.

## Complete implementation work

- Implement `UNINIT/READY/DEGRADED/OFFLINE`, `HealthSnapshot`, last error/time validity, consecutive failure, saturating total failure/success and optional offlineThreshold.
- Count only actual tracked physical I2C outcomes as transport failures/successes. Invalid parameters, unsupported model, not-ready, configuration mismatch and owner deadline are not fabricated physical transfers.
- Keep `OFFLINE` passive, allowing future explicit probe/recovery; application owns backoff. Raw health-neutral diagnostic path must still surface transfer failure directly.
- Test no counted preconditions, multi-callback transactions, device disappearance, first success, recovery from OFFLINE, count saturation, timestamp 0 validity, clock wrap and repeated errors.

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
Do not run unrelated sibling suites or remote CI (except stage 43).

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_15.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
