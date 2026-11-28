# Prompt 26 — Read-only register diagnostics

## Phase a — Requirements and baseline, 2026-11-28

Stage 26 is open and ready for implementation. Its deliverable is one checked
single-register read and an ordered full dump usable immediately after binding.
On entry, clean `main` and its tracking ref `origin/main` both pointed to
`65dcacdbefb7d92f7a7ddf0cc82759af9b397e2e`. The stage 25 closure commit
matched the subject and four paths in `commit_20261128-004203.json`, with
parent `7d77d2af3d08e246a16ad6cbb5201e268d49293f`. The wrapper recorded
upstream equality at 00:45:32 Prague time. The manifest places 26 between 25
and 27. All nine direct prerequisite reports have completion evidence.
[Stage requirements](../prompts/26_diagnostic-access.md), [progress](../prompts/PROGRESS.md).

### Inputs verified at the baseline

| Producer reports and inputs | Existing behavior to use |
| --- | --- |
| [5](prompt_5.md) `register-ledger`, [7](prompt_7.md) `safety-policy` | The [ledger](../registers/tmp11x.json) identifies each model's mapped pointers and read effects. The [safety policy](../eeprom-and-reset-policy.md) defines identity admission, EUN retention and write denials. |
| [12](prompt_12.md) `register-fake` and checked transport | [RegisterTransport](../../src/RegisterTransport.cpp) checks the complete pointer byte, reads one MSB-first word and preserves caller data on a short read. The [fake](../../test/support/FakeTmp11xDevice.cpp) records frames and can inject partial transfers. |
| [13](prompt_13.md) identity engine, [14](prompt_14.md) `binding-api` | [Identity](../../src/Identity.cpp) retains a complete incompatible word. Public `probe` invalidates live state on mismatch. [Binding and EEPROM entry points](../../src/TMP11x.cpp) already distinguish local refusals from attempted transfers. |
| [19](prompt_19.md) `config-trust`, [21](prompt_21.md) `sync-acquisition`, [22](prompt_22.md) `limits-alert` | [DeviceEvidence](../../src/DeviceEvidence.cpp) owns CONFIG and TEMP read effects, limit comparison, sticky alerts and uncertain history. [Acquisition](../../src/Acquisition.cpp) consumes eligible readiness once. |
| [23](prompt_23.md) `offset-feature`, [24](prompt_24.md) `readonly-eeprom` | The [offset drift regression](../../test/test_trust.cpp) preserves the newly observed word and invalidates live calibration continuity. [EEPROM tests](../../test/test_eeprom.cpp) exercise ID-first partial scans, the sparse pointer map, sticky EUN and neutral health. |

`readRegister`, `RegisterObservation`, `readRegisterSnapshot` and
`RegisterSnapshot` are specified in the [API contract](../api-contracts.md)
and still absent from the [public header](../../include/TMP11x/TMP11x.h).
The existing private `readDiagnosticWord` returns a scalar. Its neutral
transport supplies no completion tick, and the EEPROM snapshot obtains a
tick separately. Phase b must carry one optional completion time through each
published observation and its evidence bookkeeping. Preserve tick zero and
clockless operation.

### Silicon and source decisions

| Selected model | ID, word map and read effects |
| --- | --- |
| TMP116, TMP116N | Require the full ID word `0x1116`. Revision is unavailable. Pointers `05/06/07/08` are EEPROM1/2/3/4. CONFIG bits 1:0 are unused. [SBOS740A pp. 13–16](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=13), [register map and fields pp. 24–31](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24). |
| TMP117, TMP117M, TMP117N | Require DID `0x117` in bits 11:0 and retain the upper revision nibble. Pointers `05/06/08` are EEPROM1/2/3. Pointer `07` is signed OFFSET. [SNOSD82D pp. 14–17](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=14), [register map and fields pp. 25–32](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25). |
| TMP119 | Same ID rule and sparse EEPROM map as TMP117. The map gives `0x0117` and the ID table gives `0x2117`. Both pass the family check. The BOM supplies the exact part and grade. [SNIS236 pp. 16–19](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=16), [register map p. 32](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32), [ID table p. 39](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=39). |

The clean TMP1x2 checkout at `0791429f8fce1170e2082f1b011456fc461318b8`
provides neutral scalar reads, formatted hex and CLI callback-count tests.
Its snapshot requires initialization and reads CONFIG twice. TMP11x needs
one explicit read per pointer. Its private write policy also excludes the
sibling's raw write and bus-wide operations. Reviewed
[raw read](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/src/TMP1x2.cpp#L230),
[snapshot](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/src/TMP1x2Configuration.cpp#L384),
[BusOperations](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/src/BusOperations.cpp),
[CLI dump](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/examples/common/Tmp1x2Cli.cpp#L444)
and [CLI tests](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/test/cli/test_cli.cpp#L347).

The clean OPT4001 checkout at `f17d4944640145773d3331e155a5bc728fcc4c66`
provides a repeated-read test for destructive FLAGS. Use a second read to
expose consumption, with the mode-specific TMP11x masks below. Both sibling
HEADs matched their local tracking refs. Live sibling remote refs were not
queried in this wrapper-managed run.
[FLAGS read](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/src/OPT4001.cpp#L1140),
[regression](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/test/test_basic.cpp#L2301).

### Phase-b implementation and acceptance cases

1. **Single word and identity.** Implement the contracted
   `readRegister(uint8_t, RegisterObservation&)` using the bound address and
   [command table](../../src/CommandTable.cpp). Across six BOM selections,
   exercise all 256 pointer bytes. A mapped word costs one neutral combined
   read. Holes `09–0E` and nonzero upper nibbles cost zero callbacks. An
   unknown identity permits a labelled scalar observation. A known mismatch
   blocks non-ID reads until an explicit ID check establishes compatibility.
   Preserve caller output on refusal or failed scalar transfer. For a complete
   incompatible ID publish the raw word with `IDENTITY_MISMATCH` and invalidate
   live state. Preserve the previous transport counters and last fault.
2. **Ordered snapshots and early stop.** Full dump order in hex is
   `0F 00 01 02 03 04 05 06 07 08`, ten separate reads. Recheck ID on each
   invocation, even after earlier success. The EEPROM snapshot retains its
   `0F 04 05 06 07 08` or `0F 04 05 06 08` order. Exercise precondition
   rejection with a seeded output and inject a short read at every position.
   An admitted failure returns its completed prefix, first failed pointer
   and possible read effects. Preserve completed state even if a later
   word fails. Reuse the stage 13 `sharedBusNeighbors` fixtures at the public
   dump boundary. A CRC-disabled TMP114 or a TMP118 can yield the inferred
   unmapped word zero at `0F`. Exercise CRC-enabled TMP114 with a transport
   error and, separately, a complete `0xFFFF`. Each trace ends at the first
   ID read, before TMP114 ALERT_STATUS or TMP118 TLOW at `02`. TI documents
   a NACK for bad CRC and FF bytes with continued clocks. The response to
   an omitted CRC byte remains unknown.
   [TMP114 pp. 24–29](../reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=24),
   [TMP118 pp. 23–26](../reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=23).
3. **Destructive reads and trust.** Capture every complete CONFIG immediately
   with its diagnostic source, generation and optional tick. In alert mode
   the acknowledgement mask is `0xE000`. Retain HIGH/LOW until consumed by
   the owner. In therm mode it is `0x2000`, HIGH is a level and LOW is zero.
   TEMP acknowledges DATA_READY alone. In the prescribed dump, TEMP precedes
   CONFIG and can explain a later zero ready bit. Test repeated reads,
   same-generation single-use readiness, failure after a successful CONFIG
   and ambiguous CONFIG/TEMP attempts. A failed CONFIG can lose both alert
   and readiness history. A failed TEMP can lose readiness. A diagnostic
   supplies no fresh sample. A fresh bind with nondefault register contents
   stays uninitialized with unknown profile trust. On an owned profile,
   observed CONFIG, limits and managed offset drift must preserve the target
   and dirty reason. A later matching diagnostic cannot clear dirty.
4. **Calibration, EEPROM and protection.** Exercise stage 23's unmanaged
   `0x0000` to `0x0040` change through both scalar and full dump. Retain
   +64 counts (+0.5 °C), invalidate prior readiness and live initialization,
   and preserve historical sample provenance. On TMP116/N the identical word
   is EEPROM3 only. An EEPROM_UL observation records BUSY and latches EUN
   immediately, even if a later pointer fails. A later zero cannot remove
   that safety latch. With `bind` alone trace ID, CONFIG, TEMP and shadows
   with zero data writes, retries or fallback probes. Keep protected-write
   and ordinary CONFIG negative tests at the [private transport seam](../../test/test_register_transport.cpp).
5. **Dump format and consumers.** Supply an ordered, bounded representation
   and document a concise rendered example. Include selected model/address,
   family evidence, pointer, register name, four-digit unsigned hex word,
   signed units where defined and read effects. Mark unavailable values and
   partial or uncertain status explicitly. For example, a TMP117 TEMP word
   `0xFF80` decodes to −1 °C and acknowledges DATA_READY even when startup
   validity is unknown. EEPROM shadows carry raw words without a decoded
   temperature or guaranteed factory provenance. Cover the remaining CONFIG
   fields with the existing [R01–R18 definitions](../feature-coverage.md).
   Register the diagnostic cases in [CMake](../../CMakeLists.txt) and update
   public Doxygen, README, CHANGELOG, API/architecture, testing and feature
   coverage alongside the implementation. Stage 27 and audit 50 consume the
   public API. Stage 28 adds zero-I/O `BUSY` for active and retained-result
   slots. Stages 37/38 own CLI framing and command integration.

### Baseline checks and next slice

| Command | Result on the phase-a baseline |
| --- | --- |
| `python scripts/check_references.py` | PASS, 14 pinned PDF sizes and hashes. Python 3.12.10. |
| `python tools/check_register_ledger.py` and `python tools/test_check_register_ledger.py` | PASS, six models and 31 regression cases. |
| `cmake --build build/host --config Release --parallel` | PASS, existing Release host build. CMake 4.0.1. |
| `cmake -E chdir build/host ctest -C Release -R '^(register_transport\|identity\|binding\|eeprom\|health\|trust\|apply\|latest\|limits\|alert_comparator\|acquisition)_cpp$' --output-on-failure` | PASS, all 11 prerequisite suites. CTest 4.0.1. |

Phase b starts with the public single-word result and checked neutral path,
including identity admission, timestamps and read effects. The ordered full
dump and its acceptance cases follow on that path. Stage 26 has no software
prerequisite blocker. Stage 40 owns physical observation. Exact flag-clear
timing and TEMP coherence across a concurrent conversion remain open.
Stage 43 owns the first remote CI checkpoint.
