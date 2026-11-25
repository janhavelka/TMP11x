# Prompt 21 — Fresh conversion and owned one-shot acquisition

## Phase a — Requirements and baseline, 2026-11-25

**PARTIAL.** All eight inputs are available for implementation. The clean
baseline was `1b1c8ff2f349792b11a2d6667e0dab08881f3b0b` on `main`.
Its subject, parent `f27c82a293538cd886d8b8bbf953ad2c1b554547` and four
changed paths match the prompt 20 closure request. The wrapper recorded
equality with `origin/main` at 01:29:50 Prague time. The local refs matched
at entry. Prompt 21 follows 20 and precedes audit 49. Phases b–d still own
the implementation, review and acceptance of `sync-acquisition`.
[Prompt](../prompts/21_fresh-one-shot.md), [sequence](../prompts/SEQUENCE.json),
[progress](../prompts/PROGRESS.md).

### Verified inputs

| Consumed artifact | Evidence at the baseline |
| --- | --- |
| `behavior-contract` | The [behavior guide](../device-behavior.md) distinguishes a completed block, an observed ready event and a TEMP value. It specifies the polling intervals and limits of the silicon evidence. [Stage 6](prompt_6.md). |
| `owner-contract` | The [owner contract](../owner-operations.md) defines admission, floor-clock waits, work limits, callback accounting and expected wire sequences. [Stage 9](prompt_9.md). |
| `latest-read-engine` | [LatestRead](../../src/LatestRead.cpp) performs one combined TEMP read, records its physical effect before checking expiry, then publishes the decoded sample to caller and cache. Fresh paths need the acquisition kind and evidence in that same publication. [Latest tests](../../test/test_latest.cpp), [stage 16](prompt_16.md). |
| `sync-initialize-recover` | [Apply](../../src/Apply.cpp) establishes startup proof and checks the operation-local EEPROM guard. Its timely final matching CONFIG verifies the generation, consumes ready and timestamps the baseline. Reuse the verified state at admission. [Stage 18](prompt_18.md). |
| `config-trust` | [DeviceEvidence](../../src/DeviceEvidence.cpp) retains alert-mode HIGH/LOW, tracks THERM HIGH as a level, retains eligible continuous readiness, and records destructive-read loss. [CONFIG comparison](../../src/Configuration.cpp) already accepts owned MOD=11 against desired shutdown. [Stage 19](prompt_19.md). |
| `conversion-settings` | [Setters](../../src/TMP11x.cpp) use guarded SD, apply, baseline, desired mode and final readback. [Timing](../../src/Timing.cpp) supplies model-specific active estimates, continuous periods and conversion windows in microseconds. [Stage 20](prompt_20.md). |
| `behavior-vectors` | The [literal traces](../registers/behavior-vectors.json) specify full-block updates, SD abort, guard renewal, generation changes, equal results, poll/return races and OS provenance. The acceptance cases below use those independent words. |
| `register-fake` | The [fake](../../test/support/FakeTmp11xDevice.cpp) traces callbacks, scripts partial/ambiguous effects and models continuous averaging. Its CONFIG write filter currently rejects the OS command. [Stage 12](prompt_12.md), [block tests](../../test/test_averaging_blocks.cpp). |

### Source evidence

| Exact models | One-based pinned PDF pages | Active upper estimate in ms at AVG 1 / 8 / 32 / 64 |
| --- | --- | --- |
| TMP116, TMP116N | p.5 electrical timing, pp.12–14 modes and averaging, pp.25–27 TEMP/CONFIG, p.29 guard. Page 14 explicitly starts OS from SD. [SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=14). | 17 / 136 / 544 / 1088 |
| TMP117, TMP117M, TMP117N | p.6 one-shot electrical timing, pp.12, 14–15 conversion, pp.26–28 TEMP/CONFIG, p.30 guard. [SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=15). | 17.5 / 140 / 560 / 1120 |
| TMP119 | p.7 electrical timing without internal averaging, pp.14, 16–17 conversion, pp.33–35 TEMP/CONFIG, p.37 guard. Page 23 recommends avoiding I²C during active conversion for best precision. [SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23). | 17.5 / 140 / 560 / 1120 |

The estimates scale one conversion by AVG under the electrical row's conditions.
OS ignores CONV. All three datasheets describe SD abort, physical return to SD
after OS, and CONFIG/TEMP acknowledgement of DATA_READY. The precise clear
edge, MOD readback during/after OS, and byte latching across a continuous
update remain open. Use one combined word read. A completed owned shot in
exclusive shutdown has a stable result because no next conversion is running.
The TMP117 race report and inaccessible TMP116 discussion leave missed ready
possible. The software outcome is a timeout or not-ready result. [Read effects and evidence](../device-behavior.md).

The inspected sibling sources matched their project pins. Clean local
OPT4001 HEAD was `f17d4944640145773d3331e155a5bc728fcc4c66`. Its latest
and try-fresh APIs distinguish a register observation from retained readiness.
Clean TMP1x2 HEAD was `0791429f8fce1170e2082f1b011456fc461318b8`. Its
manual one-shot checks desired shutdown. Its OS-bit completion rule belongs
to that device. TMP11x keeps one explicit acquisition call and requires new
DATA_READY. [OPT4001 implementation at pin](https://github.com/janhavelka/OPT4001/blob/441ef579d7fa3f7bca7665f52632fdaddd306300/src/OPT4001.cpp),
[TMP1x2 implementation at pin](https://github.com/janhavelka/TMP1x2/blob/afd50889312f6e218a6a34eca687e3e8547d8b40/src/TMP1x2Measurement.cpp).

### Phase-b implementation and acceptance cases

Implement `tryReadFresh(Sample&)`, `readFreshBlocking(Sample&, uint32_t timeoutMs)`
and `readOneShotBlocking(Sample&, uint32_t timeoutMs)` on the [public device](../../include/TMP11x/TMP11x.h).
Drive one private resumable engine with at most one physical callback per step.
Keep sample decoding and publication in the latest-read path. A timely fresh
sample carries its acquisition kind, verified CONFIG/limit generation and
unknown offset provenance until stage 23. Register the cases in
[CTest](../../CMakeLists.txt) and document timing, CONFIG acknowledgement of
comparator ALERT and the F08 coherence boundary in public Doxygen and the
affected guides. Stage 22 owns public alert consumption, 26 connects raw
diagnostics, 27 owns cache inspection and 30 wraps these steps for jobs.
[Public contract](../api-contracts.md), [handoffs](../prompts/HANDOFFS.md).

1. **Admission and guard.** Require initialization, verified clean settings, retained startup proof and the matching desired mode before I²C. On each fresh acquisition read `0x04`. EUN, including `0xC000`, latches unsafe. Untimed BUSY `0x4000` returns `MEASUREMENT_NOT_READY` after one read. Timed BUSY permits only paced guard reads until safe or deadline/work exhaustion. A transport error ends the attempt. Matching SD CONFIG `0xB620` also revokes a safe guard. Record flags, compare stable fields and require renewed guard before proceeding. Drift wins over a simultaneous BUSY for a timely CONFIG.
2. **Continuous baseline and single consumption.** Exercise all six selections and the 32 CONV × AVG pairs. A verified apply's final CONFIG consumes any ready seen there. If the current generation lacks a baseline, its first CONFIG/TEMP observation only establishes one. Later diagnostic CONFIG can save ready for a guarded TEMP read. Untimed try uses at most three callbacks and zero writes. Deliver two identical `0x0C80` samples only with two separate ready events. Test `0xB220` (BUSY with HIGH/ready) and `0xA240` (AVG drift against `0x0220`). Inspect retained alerts and suppress TEMP in both. Diagnostic TEMP, ambiguous CONFIG/TEMP reads, invalidation or generation change must prevent reuse of the old evidence.
3. **One owned trigger.** Extend [transport write admission](../../src/RegisterTransport.cpp) and the fake for an explicit guarded OS command. Keep the replayable CONFIG mask `0x07FC` and its rejection tests. The existing [trigger evidence test](../../test/test_apply.cpp) injects a transfer outcome. Exercise the actual seven callbacks for desired SD `0x0620`: `R04=0000 R01=2620 W01=0620 R01=0620 W01=0E20 R01=AE20 R00=0C80`. The first CONFIG compares every stable field before writing, the explicit SD aborts residual work, and the SD read establishes the stale-ready boundary. Poll only after the trigger. Also accept `A620` on the ready poll. Both paths return 25 °C and retain HIGH. Poll `AE40` instead and require dirty, six callbacks and zero TEMP reads. Preflight `0640` or unowned `0E20` must stop before any write.
4. **Abort and uncertain effects.** Script full and partial blocks across all AVG codes, vary CONV during separate OS cases, and preserve the last complete TEMP after SD or a settings change. A diagnostic read may expose startup `0x8000`; a fresh claim requires completion of the full block. Keep durable trigger provenance separate from eligibility to publish this shot. A physically successful late trigger, timeout, or abandoning the private operation after trigger leaves the next explicit request able to abort with guarded SD. Successful explicit SD or invalidation clears ownership. Ambiguous SD/trigger writes dirty trust and stop. Cover missed ready, intervening diagnostics and bus failures without automatic retry, rollback or retrigger.
5. **Deadlines and polling.** Use the owner's monotonic clock and a whole-operation budget. Clip each callback, check expiry at equality on return and preserve physical outcomes even on timeout. From trigger completion, wait the active upper estimate before the first OS poll, then at least 5 ms from a clear poll. Earlier sparse polling can overlap active conversion. Pace timed continuous polls from the baseline or last poll at the nominal period, allowing one overdue poll. Budget the OS window at active upper + 10 ms and the continuous window at max(1.25 × nominal period, active upper) + 10 ms, plus transfer/setup time. At TMP117 AVG 8 and 1 ms resolution, a trigger completing exactly at 104 ms permits a poll at 244 ms. A floor reading of 104 ms with unknown phase permits 245 ms. In `os64-short-remaining-budget`, TMP119's first due poll exceeds the deadline, so zero CONFIG/TEMP reads follow the trigger. Test wrap, sub-tick budgets, stopped clocks and normal repeated ticks. Bound timed work at 256 dispatches and 1,000,000 unsuccessful same-tick wait checks before a further check returns `WORK_LIMIT`. Release the bus between callbacks.
6. **Publication and destructive reads.** Feed all physically successful CONFIG reads, including late ones, to the existing evidence owner with their source. Preserve sticky HIGH/LOW through a later zero, timeout or failed TEMP and preserve THERM HIGH as a level. An ambiguous CONFIG can lose alert and readiness history. An ambiguous TEMP can lose readiness alone. Exercise this at the current private seam, including missed OS readiness. A late successful TEMP consumes ready and counts one physical success while preserving caller output and the prior cache. Neither timing, GPIO, MOD readback nor a change in TEMP proves a conversion.

### Baseline checks

| Command | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel` then `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 19 existing Release tests. CMake/CTest 4.0.1, GCC 15.1.0. Acquisition-specific tests are phase-b work. |
| `python -B scripts/check_references.py` | PASS, all 14 pinned PDFs matched paths, sizes and SHA-256 hashes. |
| `python -B scripts/check_promptset.py` | PASS, 55 stages, including handoffs and source-page checks. Python 3.12.10. |

Two read-only reviews covered pinned silicon/sibling sources and the driver,
fixture and test seams. Physical acquisition remains unverified (stages
40–41). The first remote CI checkpoint belongs to stage 43.

## Phase b implementation

Started on clean `main` at `6a3c01e7335948b4aa8c81a16a44daa73a84d9e2`.
Its parent, subject and single report path matched the phase-a request. The
wrapper recorded equality with `origin/main` at 01:45:57 Prague time. The
six prerequisite stages had individual COMPLETE rows. The pinned mode and
CONFIG pages and both sibling API sources listed above were rechecked.

The three [public fresh-read APIs](../../include/TMP11x/TMP11x.h) now use
the [resumable acquisition engine](../../src/Acquisition.cpp). A safe
EEPROM_UL admits the next phase. CONFIG drift stops the sequence, and
CONFIG BUSY revokes the guard. One-shot writes explicit shutdown, reads its
baseline and issues the typed MOD=11 command once. A physically successful
trigger retains its readback provenance through timeout or abandonment.
Only new ready evidence after the current boundary can reach the
[TEMP publication path](../../src/LatestRead.cpp). Failed destructive
reads still update the [instance evidence](../../src/DeviceEvidence.cpp).
Apply and acquisition share the [deadline and work limits](../../src/OperationBudget.h).

| Native cases | Observed result |
| --- | --- |
| `allContinuousProfiles`, `continuousGuardAndReadEffects`, `continuousUpdateBetweenReads` | All 192 model/CONV/AVG selections required a complete block. Equal temperatures required distinct ready events. A diagnostic CONFIG supplied single-use readiness. At an update between CONFIG and TEMP the driver returned the word from one combined transfer. |
| `oneShotWireAndTiming`, `oneShotConfigFailures`, `renewedGuards`, `busyOneShotPoll` | The seven-callback trace returned 25 °C with either MOD=01 or 11 and retained HIGH. Preflight drift caused zero writes. Post-trigger AVG drift stopped before TEMP. After BUSY, the driver renewed the guard and required a new eligible observation. |
| `shortOneShotBudget`, `continuousPollingAcrossWrap`, `guardDenialsAndLimits`, `lateContinuousTransfers` | TMP119 AVG 64 issued zero polls when its first due time exceeded the deadline. TMP117 AVG 8 first polled at tick 245 from floor tick 104. The tests observed the 256-dispatch cap, frozen-clock termination, clipped callbacks and output/cache preservation at expiry. |
| `residualOwnedOneShot`, `destructiveReadDuringShot`, `diagnosticCapturesOwnedShot`, `missingReadyAndThermLevel` | The next explicit request aborted an unfinished block after a late trigger or abandonment. Diagnostic CONFIG captured an eligible ready event, and diagnostic TEMP could consume it. A missed event produced no fresh sample or retrigger. THERM HIGH followed the observed level. |
| `oneShotBlocks` and register transport | All 384 model/CONV/AVG/MOD-readback combinations terminated after one complete synthetic block. Explicit SD aborted partial accumulation and preserved the previous TEMP. Ordinary CONFIG writes still reject the OS command and the new typed command requires both guards. |

[Acquisition tests](../../test/test_acquisition.cpp), [block tests](../../test/test_averaging_blocks.cpp),
[register transport tests](../../test/test_register_transport.cpp).

| Local verification | Result |
| --- | --- |
| `cmake --build build/host --config Release --parallel 4` and `cmake -E chdir build/host ctest -C Release --output-on-failure` | PASS, all 20 suites including acquisition, transport, apply, latest, timing, ledger and generator checks. GCC 15.1.0, CMake/CTest 4.0.1. |
| `cmake --build build/consumer --config Release --parallel 4` and `build/consumer/tmp11x_native_consumer.exe` | PASS, the tests-off source consumer linked and printed `0.1.0`. |
| `doxygen -` on public headers with XML and `WARN_AS_ERROR=YES` | PASS, Doxygen 1.13.2 indexed all three entry points. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, local Markdown targets and `git diff --check` | PASS, 14 pinned PDFs, 55 stages and local change checks. |

Phase c owns fresh review of these changes. Audit 49 follows stage 21
acceptance. Physical qualification of MOD readback, read-clear races and
continuous byte coherence remains open for stages 40–41. The first remote
CI checkpoint is stage 43.
