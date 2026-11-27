# Prompt 25 — Per-device reset and invalidation

## Phase a — Requirements and baseline, 2026-11-27

Stage 25 is open. Clean `main` and its local upstream `origin/main` both
pointed to `d80da0faf4ffeb39a65ca2092026b8b964ebe2f8`. The parent,
subject and three changed paths matched the stage 24 closure request. The
wrapper recorded upstream equality at 01:29:54 Prague time. In manifest
order, 25 follows 24. All eight direct prerequisites have COMPLETE rows
and acceptance reports. The deliverable for phase b is the explicit,
synchronous `reset()` operation. [Prompt](../prompts/25_per-device-reset.md),
[progress ledger](../prompts/PROGRESS.md).

| Input | Evidence at the baseline |
| --- | --- |
| Stages 5 and 7, register ledger and `safety-policy` | The [device ledger](../registers/tmp11x.json), [six-model matrix](../variants.md#model-selection-and-capabilities) and [reset policy](../eeprom-and-reset-policy.md#addressed-reset-and-recovery) already record the model split, encoding and unknown effects. |
| Stages 9 and 14, `owner-contract` and `binding-api` | The [owner contract](../owner-operations.md) defines the single deadline and clock resolution. The [driver](../../src/TMP11x.cpp) owns the selected binding, live state and retained history. |
| Stages 18 and 19, `sync-initialize-recover` and `config-trust` | [Apply](../../src/Apply.cpp) establishes FIRST_ACK, waits, guards and verifies replay. [DeviceEvidence](../../src/DeviceEvidence.cpp) records complete and uncertain read effects. External invalidation is already bus-silent. |
| Stages 21 and 23, `sync-acquisition` and `offset-feature` | [Acquisition](../../src/Acquisition.cpp) tracks readiness and owned triggers. [Offset recovery cases](../../test/test_apply.cpp) exercise managed replay and preservation of unmanaged calibration. |
| Stage 12, `register-fake` | The [fixture](../../test/support/FakeTmp11xDevice.cpp) records frames and injects faults. The [transport tests](../../test/test_register_transport.cpp) already exercise protected pointers. The [checked transport](../../src/RegisterTransport.cpp) currently admits stable CONFIG and one-shot CONFIG writes only at the CONFIG pointer. |

### Silicon decision

| Selected BOM model | Addressed reset evidence |
| --- | --- |
| TMP116, TMP116N | CONFIG bits 1:0 are unused. Return `UNSUPPORTED` before the first callback. The POR description says BUSY clears during load, while the register tables say it sets. An idle word alone cannot prove startup. [SBOS740A pp. 17, 26, 29](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26). |
| TMP117, TMP117M, TMP117N | Table 7-6 defines bit 1 as R/W. Writing one starts a 2 ms reset and the bit always reads zero. Figure 7-14 labels it R-0. Use the detailed field definition. [SNOSD82D pp. 27–28](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27). |
| TMP119 | Table 8-5 gives the same command, duration and readback. Figure 8-7 has the same R-0 discrepancy. [SNIS236 pp. 34–35](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34). |

EEPROM reload, EUN after the command, the effect of companion CONFIG bits
and bus availability during addressed reset remain unknown. For the separate
POR/general-call fixture, load only MOD0 and bits 9:2 from persistent CONFIG
(`0x07FC`), along with the programmed limits, supported offset and mapped
shadows. Seed those words with nonfactory values. [Source pages by reset domain](../eeprom-and-reset-policy.md#protected-data-and-traceability).

The clean TMP1x2 sibling at `0791429f8fce1170e2082f1b011456fc461318b8`
provides the external-reset precedent. Its argument-free invalidation issues
no transfer and leaves recovery to the owner. TMP11x retains historical sample
and alert evidence and excludes the sibling's bus-wide reset helper.
[TMP1x2 implementation](https://github.com/janhavelka/TMP1x2/blob/0791429f8fce1170e2082f1b011456fc461318b8/src/TMP1x2.cpp#L187).

### Phase-b implementation and acceptance cases

1. **Command admission.** Start with a private reset write purpose in
   `src/RegisterTransport.{h,cpp}` and literal six-model tests in
   `test/test_register_transport.cpp`. Require a supported model, CONFIG
   pointer, startup proof and locked-idle guard. Accept only bit 1 plus
   canonical stable bits from the last verified settings. Check frames
   `01 02 22` (continuous) and `01 06 22` (shutdown). Reject missing bit 1,
   MOD=10/11 payloads, status/reserved bits, other pointers and ordinary
   write purposes used with the command. Exercise protected words for the
   added purpose. Count callbacks and target addresses.
2. **Public preflight and invalidation.** Require an initialized, verified
   device, valid clock/budget and retained startup proof. Read a new
   `0x04` guard before issuing the single addressed write. EUN refuses
   immediately, even with BUSY set. Only successful BUSY reads may be
   repeated at paced intervals within budget. Immediately before the command
   callback, invalidate initialization, startup origin/proof, live profile,
   guard, ready/shot and live alert evidence. Keep the desired profile,
   health history, last sample and pending historical alerts with their old
   generation and reset/unknown provenance. Test an applied write returning
   an error and a late completion. Neither permits a retry or reuse of old
   proof. Assert zero general-call traffic.
3. **Post-command observations.** From successful callback completion wait
   at least 2 ms, rounded for the clock. Read compatible ID, take a new
   FIRST_ACK at completion, then wait the separate 3 ms startup policy under
   stable power. Read a new locked-idle guard and the full profile (CONFIG,
   both limits and supported OFFSET). Use one operation deadline throughout.
   Count preflight and post-command reads separately. Preserve partial read
   evidence on failure. Exercise changed and unchanged settings, EUN, BUSY,
   ID mismatch, deadline and transport failure. A zero command bit never
   establishes completion. Matching observations leave initialization clear.
4. **Explicit recovery.** Compare addressed-reset outcomes using nonfactory
   THIGH `0x1280`, TLOW `0xFB00`, OFFSET `0x0040` and distinctive EEPROM
   shadows. A later explicit recovery restores owned settings, including a
   managed offset. When offset is unmanaged, observe the post-reset value
   and assert zero offset writes. Check the reset trace for zero replay and
   the begin/recover traces for zero reset commands. Reported external reset
   uses the existing argument-free invalidation followed by explicit recovery.
   [Public operation contract](../api-contracts.md#startup-apply-and-reset).

### Checks and handoff

| Baseline check | Result |
| --- | --- |
| `python scripts/check_references.py` and `python scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages. Python 3.12.10. |
| `python tools/check_register_ledger.py` and `python tools/test_check_register_ledger.py` | PASS, six model rows and 31 validator regressions. |
| `cmake -E chdir build/host ctest -C Release -R '^(register_transport\|binding\|apply\|trust\|acquisition\|eeprom)_cpp$' --output-on-failure` | PASS, six existing Release suites in the stage 24 build, CTest 4.0.1. These exercise the inputs to stage 25. |

Phase b starts with command admission (case 1). The public operation,
addressed-reset fixtures and their acceptance tests remain open. Stage 27
consumes the synchronous API and stage 38 owns CLI reset integration.
Physical reset behavior remains unverified. Remote CI belongs to stage 43.

## Phase b — Private reset command admission, 2026-11-27

Case 1 passed on a working tree based on `b253326808bb88686e0ebcb2830934fd2d4d6d5d`.
The phase-a commit matched the wrapper request and local upstream. The
wrapper recorded upstream equality at 01:45:09 Prague time.

The [private transport](../../src/RegisterTransport.cpp) admits an addressed
reset only at CONFIG on TMP117/M/N and TMP119. The command must contain bit 1,
with all remaining set bits inside the canonical stable mask (`0x07FC`).
The typed caller supplies the startup and locked-idle proofs. The public
operation still owns deriving the word from the last verified profile and
acquiring a new guard. The [fixture](../../test/support/FakeTmp11xDevice.cpp)
accepts the frame and models one possible outcome in which the register image
stays unchanged.

In [addressedResetAdmission](../../test/test_register_transport.cpp), each
accepted word produced one write to the selected address `0x4B`. The literal
frames included `01 02 22` and `01 06 22`. TMP116/N returned `UNSUPPORTED`
before I²C. All six models refused the other 255 pointers and incorrect write
purposes at CONFIG. On supported models each missing proof, missing command,
noncanonical MOD, status bit and reserved bit produced zero callbacks.
`protectedWritesDenyEveryPayload` also swept all 65,536 payloads at each
protected pointer with the new purpose.

| Local check | Result |
| --- | --- |
| `cmake --build build/host --config Release --target tmp11x_register_transport_test --parallel` | PASS with CMake 4.0.1 and GCC 15.1.0. |
| `cmake -E chdir build/host ctest -C Release -R '^register_transport_cpp$' --output-on-failure` | PASS, 1/1. |
| `python scripts/check_references.py` and `python scripts/check_promptset.py` | PASS, 14 pinned PDFs and 55 stages, Python 3.12.10. |
| `python tools/check_register_ledger.py` and `python tools/test_check_register_ledger.py` | PASS, six models and 31 regression cases. |
| `git diff --check` and `git diff --cached --check` | PASS. The wrapper owns staging, commit and push. |

Continue phase b with case 2, timed preflight and invalidation immediately
before the reset write, including ambiguous and late completion. The full
public operation, post-reset observations and explicit recovery tests remain open.
