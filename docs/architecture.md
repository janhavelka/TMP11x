# TMP11x architecture and register ownership

One `TMP11x::TMP11x` instance owns the observations, desired profile and
operation slot for one bound sensor. The board owns access to that sensor
and its bus. Synchronous calls drive the same resumable register steps as
cooperative jobs. The application can service another target while a
conversion or startup interval runs. [Public API and operation gates](api-contracts.md).

## Code and package boundaries

| Public header under `include/TMP11x/` | Contents |
| --- | --- |
| `Status.h` | SDK-independent errors, static messages, status predicates and passive transport states. |
| `Config.h` | Value types, model and capability queries, callback types, binding and desired sensor settings including optional managed offset counts. |
| `CommandTable.h` | Register pointers, positioned field masks and `command::accessFor(model, pointer)`. The lookup distinguishes a protected shadow from a managed offset at `0x07`. |
| `Temperature.h` | Pure signed count, Celsius and register-word conversions for TEMP and thresholds. |
| `Configuration.h` | Bus-silent profile validation, CONFIG encoding and observation, and expected-register comparison for CONFIG, THIGH, TLOW and managed OFFSET. |
| `Timing.h` | Bus-silent conversion timing for the selected model, cycle and averaging. |
| `TMP11x.h` | The device class and its sample, identity, health, settings, register and alert evidence. Public operations include binding, initialization and recovery, addressed reset, identity checks, read-only diagnostics, guarded settings changes, CONFIG and limit readback, alert event consumption, GPIO sampling and temperature acquisition. |
| `Version.h` | Checked-in generated package version from `library.json`. Its constants describe the header, and the out-of-line `version()` describes the linked library. |

Implementation belongs in `src/`. Each public header must compile on its
own as standard C++17. Installed consumers need only the public headers and
library. Keep SDK handles, GPIO setup, FreeRTOS, heap allocation, logging,
tasks, sleeps, retry loops and bus recovery outside the core. The Arduino
and ESP-IDF examples have separate adapters and one CLI in `examples/common/`.
Native IDF builds directly against the portable library. Keep examples out of
the library target. The source archive can carry standalone examples alongside
the library. Test doubles, host scripts and vendor PDFs stay in the development
checkout. [Native testing](testing.md), [toolchain and package plan](toolchain-and-package-plan.md).

## Which code decides each register action

The private `src/ModelDescriptor.h` holds the two model profiles and declares
`findModelDescriptor` for transport, identity and shadow access. Use the table
to recognize a complete ID word independently of the selected BOM model.
`src/Config.cpp` implements the lookup and the public capability query.

The private word helpers in `src/RegisterTransport.h` borrow the owner's
`Config` for one call. The caller supplies the remaining budget and receives
a status plus an `attempted` flag. A set flag counts one callback, including
when the adapter reports failure. On a failed read the caller keeps the old
word, but TEMP or CONFIG may have consumed flags. A failed write may have
changed the device. The operation owner uses the pointer, direction and
attempt evidence to update observation history and configuration trust.

Managed engines pass `detail::RegisterTransport::tracked(device)` to the
word or identity helper. The frame helper records the normalized outcome
on callback return, before any register comparison. Callers use the
`attempted` flag for their work budget. Public `readIdentity` and `probe`
leave counters, fault history and clock untouched. Checked register
observations use neutral transport and carry one optional completion tick
for the word and its effects. Both full-map and EEPROM snapshots check
identity first and retain complete words through the first failed pointer.
The full-map scan reads TEMP before CONFIG. The EEPROM scan skips both.
A lock read can latch EUN or invalidate an acquisition guard on BUSY.

`detail::LatestRead` reuses the transport's completion tick for both the
sample timestamp and the timed owner's expiry check. Its result retains a
complete TEMP word even at expiry. A short read retains the physical fault
and possible loss of readiness. Only a timely success replaces the caller's
sample and the instance cache, including the acquisition kind and freshness
of a guarded result. [Latest-read engine](../src/LatestRead.cpp).

`detail::Acquisition` drives latest reads, guarded continuous readiness and
the explicit one-shot sequence. Each step can issue one physical callback.
If a diagnostic CONFIG read observes BUSY between steps, the acquisition
keeps its completion tick even after a later clear CONFIG. Timed acquisition
renews the EEPROM guard before proceeding and repeats the applicable CONFIG
comparison before either one-shot write. An ambiguous diagnostic transfer
ends acquisition with its transport status.
Apply and acquisition share the floor-clock deadline, paced waits, callback
budget and finite work limits. Under exclusive ownership a completed
one-shot has a stable shutdown result. During a continuous update, the
datasheets leave byte latching within the combined TEMP read unspecified.
[Acquisition engine](../src/Acquisition.cpp), [operation budget](../src/OperationBudget.h).

`detail::DeviceEvidence` records each CONFIG, limit, OFFSET and TEMP outcome for
apply, managed observation, diagnostics and verification. It captures flags on complete
CONFIG reads, records possible loss after ambiguous reads and keeps each
register's observation time. The owner resolves its deadline separately,
so a late CONFIG word can record a mismatch and pending alerts even though
the operation returns a timeout. [Read and write bookkeeping](../src/DeviceEvidence.cpp).

The write path checks the selected model, the typed purpose, and the
operation owner's startup and locked-idle evidence before calling the adapter.
Stable CONFIG accepts only bits in `0x07FC`, including canonical continuous
and shutdown modes. The private apply engine reads EEPROM_UL before writing,
renews the guard after an observed BUSY, and verifies the final CONFIG before
publishing a generation. Acquisition uses a separate typed write permission
for MOD=11 after explicit shutdown and its CONFIG baseline read. Guarded
replay issues a managed OFFSET write only for an engaged target on a
capable model, then reads back the full word before the CONFIG baseline.

`detail::Reset` uses the verified settings encoder and its own CONFIG write
purpose. One operation budget spans preflight, the command and all readback.
The reset interval starts at command completion. The startup interval starts
at the subsequent compatible ID completion. Reset records the settings and
alert evidence observed under the new guard. Only an explicit replay can
establish the next verified generation. [Reset implementation](../src/Reset.cpp).

| Owner | Exclusive decision and callers |
| --- | --- |
| Model and transport (11–12) | One pure model/capability table supplies family, ID policy, EEPROM index map, offset/reset support. One checked transport maps the requested function to a permitted single-word transfer and accounts its physical outcome once. Deny holes/aliases, any data write to TEMP/ID/EEPROM_UL/shadows, and command encodings on the ordinary CONFIG path. Private typed commands have distinct permission. No transfer accepts an arbitrary caller-supplied bus address. |
| Identity (13, public at 14) | Managed and diagnostic reads pass each complete ID word through the same family decoder. The private `detail::readIdentity` returns both family status and the tracked transfer result to managed operations. A complete incompatible ID counts as a physical success there. Neutral diagnostics publish the complete ID and stop a dump on mismatch. [Identity interface](../src/Identity.h), [diagnostic implementation](../src/Diagnostics.cpp). |
| Configuration (17–20, 22–23, 25) | One encoder/expected-state comparison for CONFIG. The configuration engine owns `0x04` safety decisions, stable CONFIG fields, THIGH/TLOW and supported managed OFFSET, including ordered apply and final verification. It supplies the guarded SD/command encoder to measurement and the explicit reset command at 25. Stage 19 owns common observation bookkeeping for CONFIG/TEMP and generation/trust invalidation. Every reader calls it. Stage 22 exposes retained alert events. |
| Measurement (16, 21) | One signed-word codec for TEMP, limits and offset. One acquisition engine owns the TEMP-to-Sample publication point, continuous-ready evidence, one-shot preflight/baseline/trigger sequence, and last-good sample. It asks configuration to compare CONFIG and enforce the guard. It never installs a new profile. |
| Diagnostics (24, 26) | Shadow access maps a one-based word index through the selected capability. Diagnostics own model-specific labels, signed count interpretation and ID-first traversal. A ten-entry array holds the full-map observations in wire order. Applications render its completed prefix and the first unsuccessful pointer. The checked read path reuses the managed decoders and evidence bookkeeping. |

Stage 14 owns bus-silent binding and stage 15 owns transport health. Stage 18
supplies real initialize/recover and a resumable full-profile apply with a
single physical callback per step. Stage 19 extends trust, observation and
invalidation. Stages 20 and 22 submit typed settings to that engine. Stage 23
extends it with offset, including unmanaged calibration provenance in the
stage 21 acquisition engine. Stage 27 assembles the synchronous consumer and
cache helpers. Stage 28 supplies one admission/result slot. Stage 29 attaches
initialize/configure/recover to apply, and 30 attaches the three read kinds
to acquisition. Pending waits store the due time in the current operation.
Polling is application-driven. Stage 31 exercises failures in those existing
engines. Declarations become available when a stage can execute and test
them. [Stage handoffs](prompts/HANDOFFS.md).

## Two packages on one bus

These adjacent consumer functions use application-provided adapter functions
over the **same** `BoardBus` object. The example BOM places a TMP112 at
`0x48` and a TMP117 at `0x49`. Both adapters borrow the bus through
`i2cUser` and map controller outcomes to their package's own `Status`.
`boardMillis` reads the board clock. Each sensor object and the bus outlive
the calls. Both bring-up functions bind first and then initialize the sensor.

```cpp
#include <TMP1x2/TMP1x2.h>
#include <TMP11x/TMP11x.h>

TMP1x2::Status bringUpTmp112(BoardBus& bus, TMP1x2::TMP1x2& sensor) {
    TMP1x2::Config cfg{};
    cfg.model = TMP1x2::Model::TMP112;
    cfg.i2cAddress = 0x48;
    cfg.i2cUser = &bus;
    cfg.i2cWrite = tmp1x2Write;
    cfg.i2cWriteRead = tmp1x2WriteRead;
    cfg.nowMs = boardMillis;
    cfg.timeUser = &bus;
    auto bound = sensor.bind(cfg);
    if (!bound.ok()) return bound;
    return sensor.begin();
}

TMP11x::Status bringUpTmp117(BoardBus& bus, TMP11x::TMP11x& sensor) {
    TMP11x::Config cfg{};
    cfg.model = TMP11x::Model::TMP117;
    cfg.i2cAddress = 0x49;
    cfg.i2cUser = &bus;
    cfg.i2cWrite = tmp11xWrite;
    cfg.i2cWriteRead = tmp11xWriteRead;
    cfg.nowMs = boardMillis;
    cfg.timeUser = &bus;
    auto bound = sensor.bind(cfg);
    if (!bound.ok()) return bound;
    return sensor.begin();
}
```

After successful bring-up the same owner calls `tmp112.readSample(a)` with a
`TMP1x2::Sample` and `tmp117.readLatest(b)` with a `TMP11x::Sample` and
handles each returned status. An observation-only TMP117 consumer stops
after bind and invokes `readIdentity` or a checked diagnostic. On a mux,
each target context carries its route and a reference to this bus. Inside
each adapter callback, acquire the application bus lock, select the route
through STOP, perform the sensor transaction and release the lock within
the supplied timeout. Count extra mux transactions separately. Serialize
ownership of the *same sensor* across dependent transfers and waits even
when the bus itself is released. Neither package depends on the other or inherits from
a common driver. [Peer sources and consumers](library-comparison.md).

## Register-to-interface map

Each row names the semantic owner and its public route. The job column gives
the stage 29/30 path. “Sync” denotes a bounded observation or explicit
command with a synchronous API. “Local” denotes memory-only work that needs
no transfer budget or job slot. A job reads the same hardware through the
same owners. CLI commands are the planned shared routes for stages 37/38.
For all rows, unsupported capability, address, pointer or index is rejected
before the callback. [Source pages and test owners](feature-coverage.md),
[CLI grammar and observation rules](prompts/CLI_CONTRACT.md).

| ID | Owner and public route | Job | CLI |
| --- | --- | --- | --- |
| R01 | Measurement decodes TEMP in `readLatest` and the explicit fresh APIs. Before begin, CLI `read` uses a bound TEMP diagnostic with unknown freshness and leaves the sample cache alone. `getLastSample` reads the cache. | READ (all kinds), cache local | `read`, `read --fresh`, `oneshot`, cached `latest` |
| R02 | Configuration captures HIGH/LOW on every live CONFIG read. `getAlertStatus` inspects the retained events and therm level. Only `takeAlertEvents` drains the event record. | Shared read effect in jobs, inspect/take local | `config` and `status` each read CONFIG and disclose its effects |
| R03 | Measurement consumes eligible DATA_READY once after the generation baseline. CONFIG/TEMP observation bookkeeping includes diagnostic reads. | READ continuous fresh/one-shot | `read --fresh`, `config`, `dump` |
| R04 | Configuration tests BUSY at `0x04` and in any CONFIG read. BUSY ends an untimed fresh attempt. Only a timed path can wait for a new safe `0x04` within budget. A transfer failure ends the attempt. | All apply and fresh jobs | `begin`, `recover`, `read --fresh`, `status` |
| R05 | Configuration owns the two replayable modes and canonical CC alias. Measurement owns the single explicit OS trigger. | CONFIGURE or READ one-shot | `mode`, `shutdown`, `oneshot` |
| R06 | Configuration `setCycle` selects one of eight CONV codes. Timing estimates combine cycle and averaging. | CONFIGURE, estimates local | `cycle`, `config` |
| R07 | Configuration `setAveraging` selects the block size. Measurement schedules the OS poll from the count and model-specific active-time bound. | CONFIGURE, READ | `avg`, `oneshot` |
| R08 | Configuration `setAlert` selects alert/therm. CONFIG observation decodes flags in their observed mode. | CONFIGURE, alert inspection local | `alert`, `config`, `status` |
| R09 | Configuration `setAlert` selects polarity. Optional GPIO reader reports the physical level of the open-drain pin. | CONFIGURE, GPIO sampled by owner | `alert`, `config` |
| R10 | Configuration `setAlert` routes threshold or DATA_READY to ALERT. Compare routing independently of the returned flags. | CONFIGURE | `alert`, `config` |
| R11 | Configuration `reset` issues addressed bit 1 on TMP117/M/N or TMP119. TMP116/N refuses. Ordinary CONFIG writes zero the command. | Sync explicit command, excluded from replay | `reset`, `config` |
| R12 | Configuration `setLimits` and `readLimits` write/verify and observe both signed limits in THIGH/TLOW. | CONFIGURE, full replay | `limits`, `dump`, `recover` |
| R13 | Configuration owns EUN/BUSY policy. Diagnostics report the raw `0x04` word. Every data write to this pointer is denied. | Guard in apply/fresh jobs, diagnostic sync | `dump`, guarded setters |
| R14 | Diagnostics `readEepromWord`, `readEepromSnapshot` on TMP116/N map indices 1–4 to `05/06/07/08` hex. | Sync bounded shadow reads | `eeprom`, `dump` |
| R15 | Same API maps 1–3 to `05/06/08` hex on TMP117/M/N and TMP119. Index 4 rejects. | Sync bounded shadow reads | `eeprom`, `dump` |
| R16 | Configuration `readTemperatureOffset`/`setTemperatureOffset` access `0x07` on supporting models. Diagnostic dump labels it by selected model. TMP116/N refuses typed offset even for zero. | CONFIGURE and full replay, observe-only when unmanaged | `offset`, `settings`, `dump` |
| R17 | Identity `readIdentity`/`probe` check a complete `0x0F` word. A failed identity-first composite stops there. | In apply jobs, explicit probe sync | `identity`, authorized `discover`, `dump` |
| R18 | Transport validates each mapped selector before framing. Refuse `0x09–0x0E` or upper pointer bits. Dump enumerates documented pointers only. | All jobs inherit, no extra operation | `dump` |

## Behavior and integration map

| ID | API or application responsibility | Job and CLI mapping |
| --- | --- | --- |
| F01 | `Config::model` selects the exact ordered part. `IdentityInfo` reports observed compatibility. Packaging and accuracy grade require BOM evidence. | Bind/capabilities local, identity sync. `bind`, `model`, `caps`, `identity`. |
| F02 | `i2cAddress` comes from ADD0 tied to GND/V+/SDA/SCL for `0x48/49/4A/4B`. Board owner resolves collisions and selects a known compatible route. | Binding local, discovery explicitly application-scoped. `addr`, `bind`, `discover`. |
| F03 | Transport emits one explicit pointer per word even though the pointer is retained. Controller ACKs the MSB, NACKs the LSB, then STOP. Data writes are pointer/MSB/LSB. | Every transfer uses the same framing. `xfer` reports adapter attempts, including neutral traffic. |
| F04 | Board controls 1–400 kHz bus. Sensor does not drive SCL. The 20–40 ms held-line/no-clock timeout is independent of callback and job budgets. STOP/interface reset details differ for TMP116. | Job deadline clips each adapter call. `bus` and `xfer` report board state. |
| F05 | Begin/recover establishes missing FIRST_ACK proof under stable power, waits the 3 ms policy interval, checks lock/busy, then verifies actual writes. Initial TEMP validity is separate. | INITIALIZE/RECOVER. `begin`, `recover`, `invalidate`. |
| F06 | `setMode`/`setCycle`/`setAveraging` cover 32 CC combinations. Managed OS from desired SD uses one trigger and AVG-dependent timing. Aborted partial blocks supply no result. | CONFIGURE, READ one-shot. `mode`, `cycle`, `avg`, `oneshot`. |
| F07 | Apply establishes SD, an observed baseline and a verified mode for every profile change, including limit-only. Same-value CONFIG writes have undocumented conversion effects. | CONFIGURE/RECOVER. `mode`, `cycle`, `avg`, `recover`. |
| F08 | A single combined TEMP transfer preserves framing. Byte coherence at a concurrent CC update is unresolved. Completed owned OS under exclusive ownership yields stable TEMP. | All READ kinds. `read`, `read --fresh`, `oneshot`. |
| F09 | Alert-mode thresholds are strict excursions. CONFIG acknowledges the flags, and a later excursion can reassert. Therm HIGH holds at equality and in the band, LOW is zero. GPIO observation is separate. TMP116 block-end comparison is inferred. | CONFIGURE, retained status local. `limits`, `alert`, `status`. |
| F10 | Supported chips saturate TEMP after offset. Observe raw counts without double correction. Offset/comparator ordering remains open. TMP116/N lacks hardware offset. | CONFIGURE and READ. `offset`, `limits`, `read`. |
| F11 | Desired, observed, verified and dirty settings cover CONFIG, both limits and opt-in supported offset. Recover verifies every owned field and observes supported unmanaged calibration. | INITIALIZE/CONFIGURE/RECOVER. `settings`, `config`, `recover`. |
| F12 | After power loss or outside reset/write/ARA, invalidate affected assumptions and preserve historical evidence. Addressed reset has its own device gate, encoding, wait and observations. | Invalidation local, reset sync, RECOVER for explicit replay. `invalidate`, `reset`, `recover`. |
| F13 | Read current EEPROM shadows in index and byte order, accepting zero or arbitrary words. Owner retains the shipment lot and independently establishes provenance. | Sync bounded snapshot. `eeprom`, `dump`. |
| F14 | One slot for INITIALIZE, CONFIGURE, RECOVER, READ. Each start returns a token. Stage 28 exposes stored slot state through `getOperationSnapshot()` for the passive `jobs` query. A request acknowledgement and a terminal job result have separate records. Observation firmware starts with bind only. | `job start init/configure/recover/read/tryfresh/oneshot`, `job step`, `job auto`, `jobs`, `job result`. |
| F15 | ID-first dumps record order, incomplete prefixes and destructive read effects. A TMP116/N or TMP117-family mismatch stops after one ID callback. | Explicit bounded diagnostics sync. `identity`, `status`, `config`, `dump`. |
| F16 | Board integrator owns logic levels, pulls, local bypass, package mounting, thermal paths, strain, leakage and bus activity during conversion. | No sensor job. `bus` reports adapter configuration. [Board guide](board-integration.md). |
| F17 | Public value gives raw word, signed counts and °C. Accuracy depends on exact grade, supply, temperature range, averaging, timing and assembly. | READ. `read` reports observation. Physical accuracy needs an independent reference. |
| F18 | Each sensor callback has a finite transport timeout. Timed operations also clip it to the remaining whole-operation budget and recheck the clock on return. The exclusive owner retains physical health, uncertain effects and terminal job identity across cancellation. | `health`, `xfer`, `job cancel`, `job result`. Health/counter queries and result handling are local. |
| F19 | Check all six BOM selections and TMP114/TMP118 negative responders in stages 13/32–33. Mismatch ends the attempt with no alternate-family scan. | Identity in apply jobs, sync observation. `bind`, `identity`, authorized `discover`. |
| F20 | Stage 33 tests native consumers, 34–36 link S2/S3/P4 targets, 42 exercises the installed package, 43 verifies exact-SHA CI, and 40–41 qualify the actual fixture. | Application/build evidence, no sensor job. `version`, `bus`, `xfer` identify the run. |

## Deliberate release limits

| ID | Boundary across typed calls, jobs and shared CLI |
| --- | --- |
| X01 | No EEPROM unlock, relock, programming, persistent startup change or shadow data writes, including with EUN clear. `eeprom` reads only. |
| X02 | No general-call address `0x00` or broadcast synchronization. TEMP pointer `0x00` always goes to the bound unicast address. |
| X03 | No SMBus alert-response transactions at `0x0C`. Outside ARA may change the pin and makes live evidence uncertain. `config` uses addressed CONFIG acknowledgement. |
| X04 | No arbitrary data writes, bare current-pointer reads or multiword bursts. A checked read or typed guarded mutation is required. |
| X05 | The application owns bus rate, pin pulls, mux, interrupt handling, controller recovery and retries. `bus` and `xfer` expose information. |
| X06 | EEPROM shadows cannot authenticate a factory ID or certificate. No API/job/CLI certifies calibrated system accuracy. |
| X07 | TMP114/N, TMP118, TMP102/112/1075/110 and speculative TMP119M are outside this map. Reject unsupported model selections and incompatible responses. No foreign UID procedure or alternate ID probing. |
| X08 | No CRC/PEC, fault queue, extended mode or conversion sample counter. `caps` reports supported capabilities only. |

The [register ledger](registers/tmp11x.json) and [literal behavior vectors](registers/behavior-vectors.json)
supply independent inputs to the later implementation tests. This mapping
assigns delivery and test ownership. Evidence for an installed consumer,
a firmware link, CI and a physical fixture belongs to its respective stage.
