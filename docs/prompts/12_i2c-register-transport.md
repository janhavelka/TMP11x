# 12 — Implement bounded 16-bit I2C register transactions

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [10](10_package-foundation.md), [11](11_status-values.md).

## Objective and boundaries

Deliver one portable, endian-correct, externally serialized I2C transport boundary with explicit failure semantics.

## Source audit

Adapt TMP1x2 callback signatures and physical read/write helpers, TMP1x2 `test/bus/test_bus.cpp` and ADS1115 transfer guards; verify datasheet word pointer MSB first and combined repeated START.

## Complete implementation work

- Implement one register pointer write+two-byte read and one pointer+two-byte write with borrowed terminal callbacks; reject unexpected size/invalid address/unbound context.
- Define checked read/write access against the per-model register ledger, including read-only registers and EEPROM/0x07 alias; keep safety gates separate from the low-level wire serializer.
- Track exact physical callback counts; callback always bounded by timeout including lock acquisition. No retry, pin init, implicit bus reset or cross-device address changes.
- Normalize callback `IN_PROGRESS`/incoherent return without mislabeling NACK phase. Preserve diagnostic `detail`, and do not publish output arguments after failed/incomplete reads.
- Register native fake-bus tests for byte ordering, repeated START, short response, address vs data NACK, timeout, stale caller buffer and shared-bus ownership.

## Verification and exit criteria

Run focused register transport suite and framework-free source consumer. Inspect captured expected I2C bytes, not merely converted temperature values.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_12.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
