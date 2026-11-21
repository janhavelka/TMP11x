# Sibling library comparison

Reviewed on 2026-11-15 for repository preparation and prompt 1. This is an
implementation and registered-test audit, not a test-run report or a frozen
TMP11x API. Exact TMP11x signatures remain owned by prompt 8. TI sources, not
these libraries, determine register behavior.

## Revisions and access

The existing sibling checkouts were read only. Each had a clean working tree;
`git ls-remote https://github.com/janhavelka/<name>.git HEAD` matched its local
HEAD. No sibling fetch, checkout, build, commit, push or CI operation was run.
All five root LICENSE files contain the MIT license and the 2026 THYMOS
Solution s.r.o., Jan Havelka copyright notice.

| Repository | Reviewed local and remote HEAD |
|---|---|
| [TMP1x2](https://github.com/janhavelka/TMP1x2) | `afd50889312f6e218a6a34eca687e3e8547d8b40` |
| [OPT4001](https://github.com/janhavelka/OPT4001) | `441ef579d7fa3f7bca7665f52632fdaddd306300` |
| [ADS1115](https://github.com/janhavelka/ADS1115) | `e07622a99fe9f13c5d6a5a2497a8112a14185b9d` |
| [TCA9548A](https://github.com/janhavelka/TCA9548A) | `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` |
| [MB85RC](https://github.com/janhavelka/MB85RC) | `3ace168eb01d687ab60e9fe88cc72322e5cae260` |

TMP11x began this preparation on `main` at
`6c5e4496d5c896af689228d4bee4382a53c20626`, tracking `origin/main`; its actual
origin is `https://github.com/janhavelka/TMP11x.git`. It is an existing remote
with planning files, not an inferred or newly created repository.

## Primary API and ownership evidence

Paths below are relative to TMP1x2 at the pinned revision. Tests were read,
including their registration; they were not executed. `CMakeLists.txt`
registers `core`, `jobs`, `helpers`, `variants`, `bus`, `cli`, `console`, and
`core_no_arduino` executables with CTest. The following mapping names the
declaration, implementation and a real test or caller for each decision.

| Concern | Declaration and implementation | Registered test/caller | TMP11x decision and destination |
|---|---|---|---|
| Typed status | `include/TMP1x2/Status.h`: `Err`, `Status`, `errorName`; `src/TMP1x2.cpp`: `terminalTransportStatus` | `test/test_core.cpp`: `protocolAndClockGuards` checks a callback returning `IN_PROGRESS` becomes a terminal error while preserving detail | Adapt shape in `include/TMP11x/Status.h`; define TMP11x errors, static message lifetime and output policy in prompt 8/11. |
| Transport callback | `include/TMP1x2/Config.h`: `I2cWriteFn`, `I2cWriteReadFn`; `src/TMP1x2.cpp`: `read`, `write` | `test/test_core.cpp`: `Bus::write`, `Bus::read`, `lifecycle`; `test/jobs/test_jobs.cpp`: `budgetsAndHealth` | Adapt one bounded callback per physical operation in `Config.h` and transport implementation. Preserve repeated START, MSB-first word, user context and remaining deadline; rewrite register validation. |
| No hidden transport retry | Same private `read`/`write` each invoke the callback once; no retry loop | `test/jobs/test_jobs.cpp`: `initializationFailureMatrix`, `configurationFailureMatrix`, `readFailureAndLatePublication` stop on injected faults | Adopt single-attempt transport. Readiness polling is a scheduled new observation, not a transport retry. Application owns retry policy. |
| Passive health | `include/TMP1x2/TMP1x2.h`: `DriverState`, `HealthSnapshot`; `src/TMP1x2.cpp`: `track`, `healthSnapshot`, `guard` | `test/test_core.cpp`: `health` reaches OFFLINE, performs another read, returns READY; `validation` leaves failure counts unchanged | Adopt in TMP11x lifecycle/health. Count tracked physical outcomes; keep protocol mismatch, rejected preconditions and operation deadline separate. Preserve last fault and saturating counters. |
| Binding and lifecycle | `TMP1x2.h`: `bind`, `unbind`, `begin`, `end`, `recover`; `TMP1x2.cpp`: binding/end; `TMP1x2Configuration.cpp`: `recover` | `test/test_core.cpp`: `lifecycle`, `failureMatrix`; `test/jobs/test_jobs.cpp`: `admissionAndResults`, `cancellationAndTeardown` | Adopt bus-silent bind/end, explicit verified initialization/recovery, and retained cancellation result. Rewrite initialization for TMP11x identity and EEPROM safety. |
| Desired state versus trust | `TMP1x2.h`: `hardwareConfigDirty`, `SettingsSnapshot`; `TMP1x2Configuration.cpp`: `update`, `beginApply`, `stepApply`, `verify`; `TMP1x2.cpp`: `markDirty` | `test/test_core.cpp`: `dirtyRecovery`, `thresholdPartialWriteFailureMatrix`, `verificationPreservesMismatchEvidence`; jobs `configurationFailureMatrix` | Adapt dirty evidence before an ambiguous write and verified reapplication. Settle desired-profile commit timing in prompt 8; do not copy TMP1x2 extended-format states. |
| Cooperative operations | `TMP1x2.h`: token/result types, `start*`, `poll`, `cancel`, `takeResult`; `TMP1x2Operations.cpp`: `admit`, `poll`, `finishJob`, `takeResult` | `test/jobs/test_jobs.cpp`: `admissionAndResults`, `budgetsAndHealth`, `deadlinesAndWrap`, `readFailureAndLatePublication` | Adapt one slot, bus-silent admission, transfer budget, clipped callback timeout, late-completion handling and exactly-once result consumption. No internal task/scheduler. |
| Sample provenance | `TMP1x2.h`: `Sample`; `TMP1x2Measurement.cpp`: `readSample`, `startOneShot`, `isConversionReady`, `tryRead`, cache helpers | `test/test_core.cpp`: `oneShot`, `conversionRequiresHardwareCompletion`; jobs `oneShotWaitScheduling` | Retain separate raw value, decoded value, acquisition timestamp and confirmed conversion. Rewrite readiness and timing using TMP11x sources; latest TEMP read alone proves no new conversion. |
| Raw and tracked diagnostics | `TMP1x2.h`: `readRegister`, `readRegisterRaw`, `writeRegisterRaw`, snapshot contracts; `TMP1x2.cpp`: implementations; `TMP1x2Configuration.cpp`: `readSnapshot` | `test/variants/test_variants.cpp` checks that a bound probe leaves health untouched. In `test/helpers/test_helpers.cpp`, `trackedWordObservations` checks that only the tracked read increments successes. The helper suite also covers raw-write cache invalidation and preservation of caller output on snapshot failure. | Adapt explicit tracking and side effects. TMP11x raw APIs must still respect model/write restrictions and destructive reads; they are not an EEPROM or safety bypass. |

All four `src/TMP1x2*.cpp` units and `CommandTable.h` were inspected. The
additional `src/BusOperations.cpp`/`BusOperations.h` and registered bus tests
demonstrate explicit bus-wide helpers. Those helpers are intentionally
excluded from TMP11x: no general-call reset or alert-response utility is
needed to implement this sensor driver.

## Peer differences that matter

| Peer | Verified source and registered test/caller | Decision |
|---|---|---|
| OPT4001 | `include/OPT4001/Config.h`, `OPT4001.h`; `src/OPT4001.cpp`: `_captureReadinessFromFlags`, `_latchReadinessEvidence`, `_clearReadinessEvidence`; `test/test_basic.cpp`: `test_generic_raw_flags_access_synchronizes_readiness_without_dirtying_config`, `test_fresh_one_shot_elapsed_without_flag_is_not_ready` | Adapt destructive-read bookkeeping and independent conversion evidence. Its CRC, FIFO, sample counter, lux scaling and INT modes do not establish TMP11x behavior. |
| OPT4001 | `src/OPT4001.cpp`: `end` attempts POWER_DOWN; normal transport helpers reject OFFLINE; registered `test_offline_blocks_normal_operation_without_bus_io` | Reject these lifecycle semantics for TMP11x. Follow TMP1x2 passive OFFLINE and bus-silent end instead. Shared method names do not mean equivalent contracts. |
| ADS1115 | `Config.h`: synchronous callbacks; `ADS1115.h`: `DriverConfig`, `DeviceProfile`, owner API; `src/ADS1115.cpp`: `bind`, `begin`, `poll`, `_markHardwareConfigDirtyIfClean`; `test/test_basic.cpp`: `test_begin_partial_write_failure_preserves_dirty_address`, `test_owner_safe_initialize_uses_one_transfer_polls_and_commits_generation_after_readback` | Adapt verified readback, partial-write evidence and owner tests. Do not import ADC channel/profile generations or its token lifetime policy without a TMP11x need. |
| ADS1115 | `test/test_basic.cpp`: `test_passive_offline_diagnostic_does_not_gate_authorized_read`, `test_owner_safe_bind_and_unbind_are_zero_i2c`; `examples/02_owner_safe_poll/main.cpp`: `lockBus`, `ownerWrite`, `ownerWriteRead`, application recovery counter | Adopt application-owned serialization and timeout that includes lock acquisition; example keeps recovery retries outside the driver. Board pins, FreeRTOS and Wire remain example-only. |
| TCA9548A | `Config.h`: narrow `TransportStatus`; `src/TCA9548A.cpp`: `begin`, `recover`, `selectChannel`; `test/test_driver/test_driver.cpp`: `test_end_is_bus_silent_and_rebinding_is_repeatable`, `test_offline_is_passive_and_success_recovers_health`, `test_recover_is_one_safe_off_write_even_after_failed_begin` | Adopt ownership discipline, not API equivalence: `recover()` disables all mux routes, whereas TMP11x recovery reapplies its own settings. Route selection belongs to the application callback, never sensor core. |
| TCA9548A | `examples/common/I2cTransport.h` documents STOP completion; `Tca9548aCli.cpp` explicitly selects channels and warns about conflicting addresses | Keep mux write/STOP and following sensor access serialized by the shared owner. Never cache a route inside TMP11x or assume exclusive bus access. |
| MB85RC | `MB85RC.h`: transfer/result contracts; `src/MB85RC.cpp`: `_requestTransfer`, `pollTransfer`, `_finishTransfer`, `takeTransferResult`; `test/test_basic.cpp`: `test_transfer_preflight_busy_cancel_and_exact_end_boundary`, `test_transfer_read_respects_single_instruction_budget`, `test_transfer_timeout_after_possible_write_can_be_verified_afterwards` | Adapt retained terminal result and possible-write evidence concepts. Its caller request IDs, chunked memory transfers, current-address state and write reconciliation are not a TMP11x implementation template. |

OPT4001, ADS1115 and MB85RC tests use Unity `RUN_TEST` registration in
`test/test_basic.cpp` and a PlatformIO native environment. TCA9548A's separate
driver suite registers the named tests in `test/test_driver/test_driver.cpp`.
These registrations were inspected; no claim is made that the tests pass in
this session. In particular, OPT4001's root CMake file is an ESP-IDF component,
not the standalone CMake/CTest precedent used from TMP1x2.

## Handoff

Selected consumer discovery: local FieldCore-node was clean at
`c40ea8341edf8d665460b902cafefbe857a7f73f`. Its
`include/TunnelMonitor/i2c/I2cOwnerTransport.h` and
`src/devices/coil_head/CoilHeadModule.cpp` show application-owned single
transfers, clipped remaining timeout and separate possible-write evidence.
The current remote HEAD observed during this check was
`eccdbdcc0e9ce22beee4f54494c6c179a320d3aa`, a different revision. The GitHub
connector confirmed the owner transport header at that remote commit, but
the local CoilHeadModule path returned 404 there. Treat that module as local
revision evidence only; stage 36 must rediscover the current consumer path.
No FieldCore files, firmware or application dependency were imported.

This audit supplies prompt 1's source comparison and adoption judgments.
The lead owns the stage report, progress, checks and Git synchronization.
Prompt 2 covers other build/promptset peers and consumer discovery. Prompts
3-7 must establish exact silicon evidence before prompt 8 freezes the API.
No driver implementation, firmware build, remote CI or HIL was performed here.
