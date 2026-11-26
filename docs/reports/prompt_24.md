# Prompt 24 — Read-only EEPROM and traceability metadata

## Phase a — Requirements and baseline, 2026-11-27

**PARTIAL.** Prompt 24 is the first unfinished stage in sequence order. All
eight inputs are present on clean `main` at
`365677be60a38d60a4c40857494c56ccdab993c5`. Prompt 23's closing commit
matches its requested subject and four paths, with parent `1cde78c`. The
wrapper recorded equality with `origin/main` at 00:31:09 Prague time and
both refs matched at entry. The public EEPROM methods and snapshot type are
pending. [Stage requirements](../prompts/24_readonly-eeprom.md) and
[sequence](../prompts/SEQUENCE.json).

### Inputs verified

| Artifact (producer) | Evidence at the baseline |
| --- | --- |
| `variant-contract` (4) | The [variant guide](../variants.md#eeprom-shadow-and-traceability) identifies all six BOM selections. The [model descriptor](../../src/ModelDescriptor.h) holds the ordered pointers and word counts. |
| `register-ledger` (5) | The [ledger](../registers/tmp11x.json) records the two physical maps, source conflicts and access policy. Its validator and regression suite passed. |
| `safety-policy` (7) | The [policy](../eeprom-and-reset-policy.md) defines diagnostic reads, persistent unsafe evidence after an external unlock, and prohibited data writes. |
| `register-transport` (12) | [readWord and writeFrame](../../src/RegisterTransport.cpp) enforce one explicit pointer per transfer, decode MSB first, and check writes against the [command table](../../src/CommandTable.cpp). |
| `public-identity-api` (14) | [readIdentity and probe](../../src/TMP11x.cpp) use the [stage 13 engine](../../src/Identity.cpp). A complete mismatch retains the observed ID and invalidates live state. The binding and identity suites passed. |
| `config-trust` (19) | [DeviceEvidence](../../src/DeviceEvidence.cpp) latches EUN and records interrupted guards. [Trust tests](../../test/test_trust.cpp) exercise recovery after external correction. |
| `register-fake` (12) | The [fixture](../../test/support/FakeTmp11xDevice.h) exposes register words, scripted transfer outcomes and callback traces. The [transport suite](../../test/test_register_transport.cpp) already sweeps all pointer bytes and four write purposes on six selections. |
| `audit-20-21` (49) | [Audit acceptance](prompt_49.md#phase-d-acceptance) and the [acquisition suite](../../test/test_acquisition.cpp) cover guard renewal, single-use readiness and late callbacks. [Stage 23 acceptance](prompt_23.md#phase-d-acceptance) verifies typed offset access at the shared pointer. |

### Datasheet and traceability evidence

Each index selects one 16-bit shadow. On the wire the pointer is followed
by a repeated START and two received bytes, high byte first. The snapshot
reads the following pointers individually, in order, after ID and EEPROM_UL.

| Selected BOM model | Shadow pointers for one-based indices | Map and conflicting reset descriptions |
| --- | --- | --- |
| TMP116, TMP116N | 1 → `05`, 2 → `06`, 3 → `07`, 4 → `08` (hex) | Table 3 lists zero for all four words. The detailed entries give unknown, unknown, zero and unknown respectively. [SBOS740A pp. 19–20, 24, 29–31](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24). |
| TMP117, TMP117M, TMP117N | 1 → `05`, 2 → `06`, 3 → `08` | Table 7-3 assigns `07` to Temp_Offset. The overview and EEPROM1 text imply four words. The EEPROM2 heading says zero while the map and field table say unknown. [SNOSD82D pp. 18, 20–21, 25, 30–32](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25). |
| TMP119 | 1 → `05`, 2 → `06`, 3 → `08` | Table 8-2 assigns `07` to Temp_Offset. The four-word prose and EEPROM2 reset heading have the same discrepancies. [SNIS236 pp. 20, 22–23, 32, 37–39](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32). |

EEPROM_UL at `0x04` reports EUN in bit 15 and BUSY in bit 14, which
mirrors CONFIG bit 12. Bits 13:0 are unused. TI documents read-clear
effects on CONFIG and TEMP and specifies none for EEPROM_UL or the data
shadows. A locked data write can still change a volatile shadow. TMP116's
power-up description on p. 17 conflicts with its BUSY tables on pp. 26
and 29, so an idle observation cannot establish startup readiness.
[TMP116 p. 29](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=29),
[TMP117 p. 30](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=30),
[TMP119 p. 37](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=37).

TI describes unique data programmed during manufacture. Its Tier 1B
traceability procedure also relies on the lot code from the shipment label.
An application can bind the known route and BOM model, read the snapshot,
and retain the lot code alongside the ordered observations. Prior external
writes can change what it reads. Every bit pattern, including an all-zero
set, is valid observation data. Factory provenance, physical cell contents
and a calibration certificate require independent evidence.
[TI SBAT024 pp. 2–4, Table 3](../reference/source/NIST-Traceability-for-Temperature-and-Humidity-Sensors-Overview-SBAT024.pdf#page=2).

The 24Cxx `readByte`/`readOnce` path checks range, performs a bounded
addressed read and publishes its scalar after success. Those conventions
apply here. TMP11x has its own pointer map, health-neutral diagnostic
transport and write policy. The sibling's clean HEAD `139e76cd` and the
stage 7 pin match for the public header, implementation and integration guide.
[24Cxx source at the pin](https://github.com/janhavelka/24Cxx/blob/0bb27f46beb4fbe907beb11d9134de4e1797347d/src/EEPROM24Cxx.cpp).

### Phase-b implementation and acceptance

1. Start with `readEepromWord(uint8_t index, uint16_t& out)` on the
   [public driver](../../include/TMP11x/TMP11x.h). Use the existing model
   descriptor and health-neutral transport. A bound, clockless caller may
   read one word with unknown identity. A cached complete mismatch blocks
   the scalar locally. Test all six selections, unbound access, invalid
   indices including zero and count + 1, repeated reads after an outside
   change, asymmetric bytes, all-zero data and a short read. Check the exact
   pointer, one callback on admission, and preserved output on failure.
2. Implement `readEepromSnapshot(EepromSnapshot&)` for a bus-silent bind
   followed by explicit observation. Read `0x0F` even with a cached ID, then
   `0x04`, then the mapped words. A full pass takes six callbacks on TMP116/N
   and five on TMP117/M/N or TMP119. Preserve the selected model, address,
   actual ID, ordered completed words, lock/busy validity and per-read
   optional completion times. The observations span separate transfers.
   On a failed callback return the completed prefix and first failed pointer.
   A complete incompatible ID ends the snapshot after one callback and
   remains available in the result. Local rejection preserves the caller's
   prior snapshot. Test failures at every position, including a short lock
   read, without leaking old or unobserved fields into the new result.
3. Exercise lock words `0x0000`, `0x4000`, `0x8000`, `0xC000` and
   reserved-bit noise. A diagnostic reports BUSY once and continues observing
   shadows. A complete EUN observation latches the existing unsafe condition
   immediately, even if a later word fails. A later diagnostic zero leaves
   guarded mutation and freshness blocked until owner correction and
   invalidation. Diagnostic reads confer no write authority. Check neutral
   health on both success and failure, clockless timestamps and a valid tick
   of zero. The neutral transport currently omits completion time. Preserve
   READY/ALERT evidence by avoiding CONFIG and TEMP throughout the snapshot.
4. Extend the private write-policy tests with varied payloads including
   relock, unlock and all bits set. Check **zero callbacks** to EEPROM_UL
   or any mapped shadow on denied write attempts, including TMP116/N
   `0x07`. On the other four selections only guarded managed-offset intent
   may write `0x07`. Trace existing begin, temperature read and EEPROM
   routes at the bound seven-bit address. Check shadow contents across each
   trace, zero protected data writes, zero general calls and zero diagnostic
   reset attempts. Use the callback trace to detect even a failed attempt.

Add the focused public cases to the [host test entrypoint](../testing.md),
document both methods and snapshot failure semantics in public Doxygen, and
update the user guide and [feature coverage](../feature-coverage.md) for
R13–R15, F13, the EEPROM portion of F15, and X01/X02/X04/X06. Stage 26
integrates diagnostic dumps, stage 28 adds occupied-job admission, and
stages 37/38 exercise CLI observation and mutation boundaries. Physical
observation belongs to stage 40. [Public contracts](../api-contracts.md),
[owner limits](../owner-operations.md) and [handoffs](../prompts/HANDOFFS.md).

### Phase-a checks

- `python scripts/check_references.py` and `python scripts/check_promptset.py`
  passed with Python 3.12.10. All 14 pinned PDFs and the 55-stage manifest
  passed their respective checks.
- `python tools/check_register_ledger.py` and
  `python tools/test_check_register_ledger.py` passed on six BOM selections
  and all 31 ledger regressions.
- `cmake --build build/host --config Release --parallel` and
  `cmake -E chdir build/host ctest -C Release --output-on-failure -R '^(value_types_cpp|register_transport_cpp|identity_cpp|binding_cpp|health_cpp|apply_cpp|trust_cpp|acquisition_cpp)$'`
  passed with CMake/CTest 4.0.1 in the existing MinGW Release build. All
  eight prerequisite suites passed. Public EEPROM tests await phase b.

Two parallel read-only assessments checked the pinned silicon sources and
the transport, tests and sibling precedent. Phase b has no software blocker.
Physical and remote CI checks were not run. Stage 43 owns the first CI
checkpoint.
