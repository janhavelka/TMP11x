# 12 — Implement bounded 16-bit I2C register transactions

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [10](10_package-foundation.md), [11](11_status-values.md).
Produces: `register-transport`, `command-table`, `register-fake`.
Consumes: `register-ledger`, `api-contract`, `owner-contract`, `value-types`, `build-foundation`.

## Objective and boundaries

Deliver one portable, endian-correct, externally serialized I2C transport boundary with explicit failure semantics.

## Source audit

Adapt TMP1x2 callback signatures and physical read/write helpers, its sensor transport tests in `test/test_core.cpp`, and ADS1115 transfer guards. `test/bus/test_bus.cpp` mainly covers excluded bus-wide helpers and is not the sensor-transport authority. Verify the one-byte register pointer and MSB-first two-byte data word against the TI serial-interface diagrams; use one combined repeated-START callback for reads.

## Complete implementation work

- Implement private register transport over a validated borrowed binding view: one pointer write+two-byte read and one pointer+two-byte write. Reject invalid lengths/address or missing callbacks; a stateless callback may use a null opaque context. Stage 14 adds public unbound-instance guards once lifecycle exists; do not create a fake binding API here.
- Produce `include/TMP11x/CommandTable.h` with reviewed literal register addresses, field/access metadata and variant distinctions from stage 5. Keep it framework-free; stage 17 adds CONFIG codecs, not a second register table. Test this private transport seam directly before public lifecycle integration.
- Define one shared checked-access/write-policy seam against the per-model ledger and stage 11 capability rows, including read-only registers and model-dependent 0x07 (TMP116 EEPROM3 / TMP117–TMP119 Temp_Offset); keep safety gates separate from the low-level wire serializer. Later features use or extend this seam rather than introducing competing allowlists. Cross-check CommandTable metadata and capability rows against the stage 5 ledger in native tests using independent literal expectations.
- Track exact physical callback counts. The core passes the clipped remaining timeout; the adapter must enforce that bound across lock acquisition and controller work, tested in stages 34/35. No retry, pin init, implicit bus reset or cross-device address changes.
- Normalize callback `IN_PROGRESS`/incoherent return without mislabeling NACK phase. Preserve diagnostic `detail`, and do not publish output arguments after failed/incomplete reads. If the frozen callback returns only Status, the adapter owns byte-count validation and must return an error on a short transfer; core cannot detect a dishonest OK with a short buffer fill. Do not use sentinel bytes as a substitute for transferred-length reporting.
- Register native fake-bus tests for byte ordering, one combined-read callback, adapter-reported short-transfer failure, address vs data NACK, timeout, stale caller buffer and shared-bus ownership. Real repeated-START/electrical behavior is an adapter/HIL gate; a core fake cannot prove it on hardware.
- Produce the initial reusable `test/support/FakeTmp11xDevice.h` and `.cpp` register-device fixture here, with independent literal register/access facts, callback trace/counts and scripted terminal failures. Later stages extend this same fixture as their features land; stage 32 consolidates variant/timing coverage, not the first fake. Keep unknown silicon outcomes parameterized and production codecs out of expected-value generation.

- Implement F02–F04/R18 from [FULL_COVERAGE](FULL_COVERAGE.md): validate all four 7-bit addresses and reject shifted 8-bit forms, holes and nonzero pointer upper bits without aliasing. Supply an explicit pointer on every access even though the chip retains it; do not assume auto-increment or burst across EEPROM words.
- Document physical read framing: ACK the first data byte, NACK the last, then STOP; the chip never drives SCL. All selected parts specify 1–400 kHz and a 20–40 ms held-line/no-clock interface timeout (35 ms typical). STOP-reset and ignore-until-START behavior are explicit for TMP117 p.20 and TMP119 p.23; TMP116 p.21 documents timeout reset, not that stronger STOP claim. Never infer that STOP resets the retained register pointer. Core byte tests and adapter/electrical proof remain distinct.
- Test all address/pointer boundary cases and terminal adapter errors after partial transfer. Unknown partial-word commit/byte-latching behavior must not turn an ambiguous error into a known unchanged sensor state.

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
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_12.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
