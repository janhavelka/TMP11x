# Prompt 18 — EEPROM-safe initialization and configuration writes

## Phase a — requirements and inputs, 2026-11-23

**PARTIAL.** All 13 inputs are available for phase b. The clean baseline was
`fc1d1de8f1dbd8844842ae921efaf43eb00ad3b1` on `main`, equal to local
`origin/main`. The parent `2a1a19b`, subject and four changed paths match
the prompt 47 closure request. The wrapper recorded upstream equality at
21:16:23 Prague time. The sequence places 18 immediately after audit 47.
[Stage requirements](../prompts/18_safe-initialize.md), [audit acceptance](prompt_47.md#phase-d-acceptance).

### Verified inputs at the baseline

| Consumed artifacts | Evidence and use in phase b |
| --- | --- |
| `safety-policy`, `behavior-contract`, `behavior-vectors` | The guard truth table and the `startup-anchor-and-reset`, `non-atomic-therm-limits` and `continuous-generation-change` vectors give literal timing, pointer, ordering and read-effect expectations. [Safety policy](../eeprom-and-reset-policy.md), [device behavior](../device-behavior.md), [vectors](../registers/behavior-vectors.json). |
| `api-contract`, `owner-contract` | Binding replacement, first-write intent, terminal status, floor-clock waits and work limits are specified for the shared step engine. [Public contract](../api-contracts.md#startup-apply-and-reset), [owner contract](../owner-operations.md#work-limits), [architecture](../architecture.md). |
| `binding-api`, `public-identity-api`, `transport-health` | Bind currently validates the binding before replacing it. Public probe uses a health-neutral ID read. Apply can pass a tracked transport to the same identity engine. Word writes already require startup and locked/idle evidence from their caller. [Lifecycle](../../src/TMP11x.cpp), [identity engine](../../src/Identity.cpp), [transport](../../src/RegisterTransport.h), [health tests](../../test/test_health.cpp). |
| `temperature-codec`, `config-codec`, `latest-read-engine` | Signed counts cover each entire limit word. Existing helpers validate all steady selectors and the THERM limit pair, encode canonical CONFIG, decode flags and compare registers independently. Public latest already requires managed initialization. [Signed codec](../../src/Temperature.cpp), [CONFIG codec](../../src/Configuration.cpp), [latest engine](../../src/LatestRead.cpp). |
| `register-fake`, `audit-10-17` | The fixture records addressed frames and grants, destructive reads and ambiguous writes. A successful legal write currently updates storage. Phase b needs a scripted ACK with the old word retained to exercise ignored writes. The audit closed the package and read-path prerequisites. [Register fixture](../../test/support/FakeTmp11xDevice.cpp), [audit report](prompt_47.md). |

### Silicon evidence and reference implementation

| BOM selections | Pinned TI evidence |
| --- | --- |
| TMP116, TMP116N | SBOS740A p.17 §7.5.1.1 allows reads during reset loading, describes all writes as ignored and describes BUSY as clear. Tables 6 and 10 on pp.26 and 29 instead describe BUSY as asserted at power-up. Exercise the clear-BUSY case. See pp.15–17 for comparator and startup behavior, pp.24, 26–29 for the map, CONFIG, limits and EEPROM_UL. [TMP116 datasheet](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17). |
| TMP117, TMP117M, TMP117N | SNOSD82D pp.16–18 describe comparator behavior, asserted startup BUSY and ignored CONFIG/limit writes during load. See pp.25, 27–30 for the map and fields. The load-time response to an offset write is unspecified. [TMP117 datasheet](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18). |
| TMP119 | SNIS236 pp.18–20 and pp.32, 34–37 document the corresponding behavior and register fields. The same offset uncertainty applies. The device can load programmed values other than the factory example. [TMP119 datasheet](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20). |

TI gives about 1.5 ms for startup without a guaranteed maximum. The 3 ms
wait below is project policy. Readback detects a suppressed write when the
old word differs from the target. A matching factory word alone cannot show
that a write took effect. [Reference manifest](../reference/manifest.json), [register ledger](../registers/tmp11x.json).

The clean local TMP1x2 checkout was at `0791429f8fce1170e2082f1b011456fc461318b8`.
Its `src/TMP1x2Configuration.cpp`, `test/test_core.cpp` and
`test/jobs/test_jobs.cpp` match the project pin `afd50889312f6e218a6a34eca687e3e8547d8b40`.
Reuse the one-transfer structure of `beginApply`/`stepApply` and the mismatch
and partial-write test patterns. TMP11x requires ID-first access, its own
startup clock, guarded shutdown and limit ordering. The peer starts with
CONFIG, settles extended-mode changes and writes TLOW first. [Sibling comparison](../library-comparison.md).

### Phase b deliverables and acceptance traces

`safe-apply-engine` belongs in the private core. `sync-initialize-recover`
adds `begin(const Config&)`, `begin()` and `recover()` to the existing device
class. All three drive the same resumable engine. Register its focused tests
with the host suite and document the delivered API, read effects and scope
of verification. [Public class](../../include/TMP11x/TMP11x.h), [test entrypoint](../../CMakeLists.txt), [coverage rows R04, R13, F05, F11 and X02](../prompts/FULL_COVERAGE.md).

| Planned case | Required observable result |
| --- | --- |
| Admission and target retention | Validate the entire candidate before replacing the binding. Invalid selectors or inverted THERM limits cost zero callbacks and preserve the prior route, target and evidence. Bind-only diagnostics allow a missing clock. Timed begin checks its clock and budget before replacement. An accepted replacement retains its own target even if I/O fails. For a settings candidate on an existing binding, retain the old target until immediately before the first mutating callback, including shutdown. |
| FIRST_ACK and bounded waits | Read ID at `0x0F` first. An incompatible word costs one tracked physical success and stops dependent I/O. With proof absent, anchor only on this operation's compatible ID completion. At floor tick 50 the first eligible startup tick is 54 for 1 ms resolution or 70 for 10 ms resolution. Use the wide interval `q + ceil(d/q) × q`. Round the operation budget down to whole ticks, clip callback grants to remaining time and expire at equality. Test wrap, a frozen clock, delayed power after bind and end/begin. Discard unfinished origins. Established proof requires continuously stable power and exclusive ownership. |
| Guard and forbidden frames | Read EEPROM_UL at `0x04` before mutation. Mask with `0xC000`. EUN set, including `0xC000`, latches unsafe immediately. Ordinary recovery, a zero diagnostic word or rebind cannot legitimize a known unlocked target without external correction. With EUN clear and BUSY set, wait at least 1 ms from completion using the floor-clock rule, then reread only `0x04`. Failures end the attempt. One safe word covers dependent writes only within uninterrupted ownership of this operation. BUSY observed in CONFIG cancels it. Assert zero premature writes for busy timeout and clear-BUSY TMP116 load, zero data writes outside CONFIG/THIGH/TLOW and zero general-call or alert-response traffic. |
| Resume and work limits | Inspect successive private steps and compare their combined transcript with public begin/recover. Each dispatch costs zero or one callback. A parked wait retains a due time and returns pending. Cap the operation at 256 dispatches and the same-tick unsuccessful wait checks at 1,000,000. A late complete callback retains its physical outcome and read effects. Expiry prevents any subsequent transfer or publication. |
| Ordered limits and comparator mode | Observe the current pair as needed. In THERM, moving [0,10] °C to [15,20] writes THIGH `0x0A00` before TLOW `0x0780`. Lower TLOW first if the new THIGH is below the old TLOW. Before changing THERM [0,10] to ALERT [20,10], write and verify SD/ALERT CONFIG `0x0620` under the guard. Failed, mismatched or late verification permits zero inverting writes. In reverse, verify TLOW `0x0000` and THIGH `0x0500` before enabling THERM. Unknown or inverted initial limits require full replay. Independent limit writes can change the pin between transfers. |
| Final verification and read effects | For every profile change, including repeat begin, limit-only and pin-only changes, establish guarded SD, apply and verify stable CONFIG and both limits, consume the SD CONFIG baseline, write the desired mode, then read CONFIG again. Check exact pointer order and both 16-bit limit values. Compare canonical stable fields with the existing MOD-aware comparator. If final CONFIG matches but reports BUSY, retain its flags, pace a new safe `0x04` and repeat final CONFIG. A timely full match clears dirty and advances the verified generation. Every CONFIG read consumes readiness. Accumulate ALERT HIGH/LOW until consumption, track THERM HIGH as a level, and preserve historical events and possible loss on failed reads. Final DATA_READY is the new baseline. |
| Partial failure, recovery and latest | Inject an ambiguous first or second write, an independently mismatched limit, and a late or mismatched final CONFIG. Retain the target, dirty state and partial evidence. A first failed initialization cannot admit latest. Explicit recover replays the target under a fresh guard without reset, rollback or transport retry. On timely success for each of the six models, public latest costs one tracked TEMP read, reports freshness false and keeps offset provenance unknown. Already initialized dirty reads retain untrusted provenance. Compare physical health with the callback count, including at deadline equality. |

Stage 19 owns public invalidation and expanded trust snapshots. Use fresh
binding or private state for proof-renewal tests here. Stage 22 owns public
alert consumption, stage 23 adds offset, stage 29 connects the private steps
to jobs, and stage 31 extends fault coverage. Stage 18 verifies CONFIG and both
limits and leaves `0x07` untouched on every model. [Handoff boundaries](../prompts/HANDOFFS.md).

### Phase a checks

| Command | Result |
| --- | --- |
| `cmake --build build/p47d/host --config Release --parallel 4`, then `cmake -E chdir build/p47d/host ctest -C Release --output-on-failure` | PASS, the existing Release build was current and all 14 prerequisite tests passed with CMake/CTest 4.0.1. |
| `python -B scripts/check_references.py` and `python -B tools/check_register_ledger.py` | PASS with Python 3.12.10, all 14 pinned PDF hashes and six BOM model maps. |
| `python -B scripts/check_promptset.py` | PASS, 55 stages. |

Phase b can begin. The acceptance traces above await implementation. Adapter
links and wire timing, physical qualification, installed-package qualification
and remote CI retain their respective checkpoints in stages 34–36, 40–41, 42
and 43.
