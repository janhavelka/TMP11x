# 12 — Implement bounded 16-bit I2C register transactions

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [10](10_package-foundation.md), [11](11_status-values.md).

## Objective and boundaries

Deliver one portable, endian-correct, externally serialized I2C transport boundary with explicit failure semantics.

## Source audit

Adapt TMP1x2 callback signatures and physical read/write helpers, its sensor transport tests in `test/test_core.cpp`, and ADS1115 transfer guards. `test/bus/test_bus.cpp` mainly covers excluded bus-wide helpers and is not the sensor-transport authority. Verify the one-byte register pointer and MSB-first two-byte data word against the TI serial-interface diagrams; use one combined repeated-START callback for reads.

## Complete implementation work

- Implement one register pointer write+two-byte read and one pointer+two-byte write with borrowed terminal callbacks; reject invalid lengths/address or an unbound instance/missing callback. Do not reject a null opaque user context when callbacks are stateless.
- Define checked read/write access against the per-model register ledger, including read-only registers and EEPROM/0x07 alias; keep safety gates separate from the low-level wire serializer.
- Track exact physical callback counts; callback always bounded by timeout including lock acquisition. No retry, pin init, implicit bus reset or cross-device address changes.
- Normalize callback `IN_PROGRESS`/incoherent return without mislabeling NACK phase. Preserve diagnostic `detail`, and do not publish output arguments after failed/incomplete reads. If the frozen callback returns only Status, the adapter owns byte-count validation and must return an error on a short transfer; core cannot detect a dishonest OK with a short buffer fill. Do not use sentinel bytes as a substitute for transferred-length reporting.
- Register native fake-bus tests for byte ordering, one combined-read callback, adapter-reported short-transfer failure, address vs data NACK, timeout, stale caller buffer and shared-bus ownership. Real repeated-START/electrical behavior is an adapter/HIL gate; a core fake cannot prove it on hardware.

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
