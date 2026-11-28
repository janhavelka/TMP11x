# TMP11x

A new, framework-neutral C++17 driver project for TI TMP116 / TMP116N,
TMP117 / TMP117M / TMP117N, and TMP119. TMP117 is the primary intended sensor.

Development version `0.1.0` provides portable C++17 status, binding, settings,
model capability and observation types through `<TMP11x/TMP11x.h>`.
`TMP11x::getCapabilities()` reports features of an explicit BOM selection
without accessing hardware. `<TMP11x/CommandTable.h>` exposes the register
pointers, field masks and per-model access classes. The private transport
has native tests for one-word reads and guarded writes. The public device
class supports bus-silent binding, explicit family checks, verified volatile
initialization and recovery, latest and fresh temperature reads, read-only
register dumps and passive transport health. Physical qualification is pending.
`<TMP11x/Version.h>` exposes `TMP11x::version()` for the linked library.
[Value and binding contracts](docs/api-contracts.md).

Use `<TMP11x/Temperature.h>` to convert a TEMP or limit word without a
device. For example, `temperature::decodeCounts(0xFFFF)` in namespace
`TMP11x` returns −1 count, or −0.0078125 °C. A stored TEMP value alone
cannot establish when the sensor completed a conversion.

Use `<TMP11x/Configuration.h>` to validate a desired profile, encode a CONFIG
word, or interpret one already read. For example, `decodeConfiguration(0xB620)`
in namespace `TMP11x` reports shutdown with HIGH, DATA_READY and EEPROM_BUSY
set. The corresponding steady settings encode as `0x0620`.

## Read the device ID

Supply the board's synchronous write and combined write/read callbacks and
the address for the selected route. This example assumes a TMP117 at `0x48`.

```cpp
#include <TMP11x/TMP11x.h>

TMP11x::Status identifyTmp117(TMP11x::TMP11x& sensor, TMP11x::I2cWriteFn write,
                              TMP11x::I2cWriteReadFn writeRead, void* route,
                              TMP11x::IdentityInfo& id) {
  TMP11x::Config cfg{};
  cfg.model = TMP11x::Model::TMP117;
  cfg.i2cAddress = 0x48;
  cfg.i2cWrite = write;
  cfg.i2cWriteRead = writeRead;
  cfg.i2cUser = route;
  const auto bound = sensor.bind(cfg);
  if (!bound.ok()) return bound;
  return sensor.readIdentity(id);
}
```

`bind` copies the configuration locally. The owner keeps the borrowed route
and bus alive and serializes access. `readIdentity` needs no clock and makes
one combined read of register `0x0F`. On `IDENTITY_MISMATCH`, inspect
`id.raw` for the complete word. A transfer error preserves the caller's
previous output. Use `probe()` for the same family check when only the status
is needed. `end()` clears observations and retains the binding for another
read. `unbind()` also releases the binding. Both use zero bus callbacks.

## Inspect registers after binding

Use `readRegister(pointer, observation)` for one mapped word or
`readRegisterSnapshot(dump)` for the whole map. The dump checks the ID on
every call and stops immediately if the transfer fails or the family differs.
It reads up to ten words separately at the bound address. Both operations
work without a clock or initialization and leave tracked bus health alone.

An application can render a clockless, freshly bound TMP117 dump as follows.
The rows are in callback order. TEMP acknowledges DATA_READY, so the zero
ready bit in the subsequent CONFIG row cannot establish its earlier value.

```text
model=TMP117 address=0x48 id=0x2117 family=compatible revision=2 status=OK
startup=unknown profile=UNKNOWN sample_freshness=unknown time_ms=unavailable
ptr   register   raw     decoded                                 read clears
0x0F  DEVICE_ID  0x2117  DID=0x117 REV=2                         -
0x00  TEMP       0xFF80  -128 counts (-1 °C)                     DATA_READY
0x01  CONFIG     0x8220  ALERT HIGH=1 LOW=0 READY=0 BUSY=0       HIGH,LOW,DATA_READY
0x02  THIGH      0x0C80  +3200 counts (+25 °C)                   -
0x03  TLOW       0xFD80  -640 counts (-5 °C)                     -
0x04  EEPROM_UL  0x0000  EUN=0 BUSY=0                            -
0x05  EEPROM1    0xCAFE  raw shadow                              -
0x06  EEPROM2    0x0000  raw shadow                              -
0x07  OFFSET     0x0040  +64 counts (+0.5 °C)                    -
0x08  EEPROM3    0x1234  raw shadow                              -
```

On TMP116/N the ID must be `0x1116`, revision is unavailable, and `0x07`
and `0x08` are EEPROM3 and EEPROM4. A shadow's raw word has no temperature
units or guaranteed factory provenance. On any model, CONFIG in THERM mode
acknowledges DATA_READY alone. HIGH is the therm level and LOW reads zero.
In this example `0x8220` also encodes continuous MOD=00, CONV=4, AVG=1
(eight conversions), active-low threshold routing and clear bits 1:0.

On an admitted failure, display the returned status and only the first
`dump.wordCount` rows. For example, a short CONFIG read in the scan above
would leave DEVICE_ID and TEMP available. Report `failedPointer=0x01`,
`status=I2C_SHORT_TRANSFER`, CONFIG and later words unavailable, and
possible loss of HIGH, LOW and DATA_READY. A complete incompatible ID is
itself a valid raw row. [Diagnostic output contract](docs/api-contracts.md#register-diagnostics).

## Read EEPROM traceability metadata

Bind the sensor on its known route and call `readEepromSnapshot(snapshot)`.
This works before initialization and with no clock. The driver checks DEVICE_ID,
reads EEPROM_UL once, then reads each data shadow in a separate combined
transfer. No data writes, resets, CONFIG reads or TEMP reads occur.

| BOM model | One-based shadow indices, in read order |
| --- | --- |
| TMP116, TMP116N | 1 → `0x05`, 2 → `0x06`, 3 → `0x07`, 4 → `0x08` |
| TMP117, TMP117M, TMP117N, TMP119 | 1 → `0x05`, 2 → `0x06`, 3 → `0x08` |

Check the returned status. `NOT_BOUND` leaves the previous snapshot intact.
Once a scan starts, the first `snapshot.wordCount` entries of `snapshot.words`
hold the completed shadow reads. Each entry records its pointer, raw 16-bit
word (high byte first on the wire) and optional completion time. If the scan
stops early, `failedPointer` identifies the first failed read or ID check. A complete incompatible ID
remains in `snapshot.identity`. An absent `snapshot.lock` means lock and BUSY
are unknown. A present lock describes only that read. An observed unlock
blocks guarded operations until the owner corrects the device and calls
`invalidateDeviceState()`.

For one word, `readEepromWord(index, out)` uses the BOM selection even before
an ID read. An unbound device, out-of-range index or cached identity mismatch
returns an error before I²C. An admitted read takes one transfer.
All failures preserve `out`. Keep the shipment-label lot code with
the model, address, route and ordered readings. Every shadow value, including
zero, is valid. An external writer can change a shadow even with EEPROM
locked, so the non-atomic snapshot alone cannot authenticate a factory ID
or establish nonvolatile cell contents. [TI sources and traceability policy](docs/eeprom-and-reset-policy.md#protected-data-and-traceability).

## Initialize and read a temperature

For managed measurements, provide a monotonic floor clock in `cfg.nowMs` and
set `cfg.clockResolutionMs` to its tick in whole milliseconds. Select the
desired mode, alert settings and limits in `cfg.settings`. Keep power stable
and hold exclusive ownership of the target throughout initialization and
subsequent use of its startup evidence.

```cpp
TMP11x::Status initializeAndRead(TMP11x::TMP11x& sensor, const TMP11x::Config& cfg,
                                 TMP11x::Sample& sample) {
  const auto initialized = sensor.begin(cfg);
  if (!initialized.ok()) return initialized;
  return sensor.readLatest(sample);
}
```

`begin(cfg)` checks the candidate before replacing the binding. On an
existing binding, `begin()` and `recover()` replay its retained settings.
Each call checks the ID and EEPROM lock/busy state, enters shutdown, verifies
the limit words and reads back the desired CONFIG. Separate writes can
transiently change the ALERT pin. A failed write can leave the target dirty
and partially applied. Use the returned status to decide when to recover.
An observed EEPROM unlock requires external correction.

Initialization and recovery verify CONFIG and both limits. On TMP117/M/N and
TMP119 they also read the live calibration in shutdown and preserve it by default. The final CONFIG
read consumes DATA_READY. A subsequent `readLatest` reports the stored word
with `freshConversion == false`, including immediately after initialization.
[Startup, read effects and recovery](docs/api-contracts.md#startup-apply-and-reset).

After initialization, `readTemperatureOffset(counts)` reads the hardware
calibration on TMP117/M/N and TMP119 in units of 1/128 °C. A complete read
updates `getSettingsSnapshot().offset`. If unmanaged calibration changes between
reads, the call returns the new value and invalidates initialization and
pending readiness. Restore exclusive access and stable power before calling
`recover()`. On TMP116/N the call returns `UNSUPPORTED` without a transfer.

Set `config.settings.managedOffsetCounts` before initialization to own the
volatile calibration, or call `setTemperatureOffset(celsius)` on a verified
device. The setter rounds to the nearest 1/128 °C, with ties away from zero.
An explicit zero clears the live offset and makes zero the recovery target.
Guarded replay writes and checks the selected value in shutdown. If readback
differs, the snapshot retains the observed word and marks the profile dirty.
The write leaves EEPROM intact. Accuracy qualification requires an independent
temperature reference.

On a confirmed fresh conversion, `sample.offset` records the calibration in
effect and whether the driver manages it. TMP116/N reports `UNAVAILABLE`.
On offset-capable parts, `readLatest` reports `UNKNOWN` because a stored
TEMP word can predate the current calibration. Use `sample.celsius` directly.
The sensor has already applied its offset and saturated the result at the
register limits.

## Read a new conversion

In verified continuous mode, `tryReadFresh(sample)` checks once and returns
`MEASUREMENT_NOT_READY` if a full new block is unavailable. Two successive
blocks may have the same temperature. `readFreshBlocking(sample, timeoutMs)`
paces its CONFIG polls at the configured block period. In alert mode, reading
CONFIG acknowledges HIGH and LOW and can deassert a comparator-routed ALERT
pin. In therm mode, HIGH retains its level across the read.

In verified shutdown, `readOneShotBlocking(sample, timeoutMs)` establishes
a shutdown boundary, triggers one block and waits for new DATA_READY. On a
TMP117 averaging eight conversions, allow 150 ms for the conversion window,
plus time for setup, clock quantization and bus transfers. Both blocking calls
use one whole-operation deadline. Each successful fresh read records the
verified settings generation. Failed reads preserve the previous sample.
[Polling, deadlines and precision](docs/device-behavior.md#polling-and-deadlines).

## Set conversion timing

After successful initialization, use `setMode`, `setCycle` and `setAveraging`
to change one selector. Each call verifies the full profile under the bound
operation deadline. For example, `sensor.setAveraging(TMP11x::Averaging::SAMPLES_8)`
retains the requested cycle and mode. Check the returned status. If a write
was attempted and verification failed, inspect the snapshot and call `recover()`
under stable power and exclusive ownership.

Use `<TMP11x/Timing.h>` to estimate timing before accessing a sensor. On
TMP117, cycle code 4 with eight conversions gives one result per second
nominally, 875 ms nominal standby and 124 ms estimated typical active time.
The helper reports durations in microseconds and continuous update rate in Hz.
[Timing conditions and examples](docs/device-behavior.md#conversion-modes-and-intervals).

## Set limits and observe ALERT

After initialization, `sensor.setLimits({-1344, -832})` sets TLOW to −10.5 °C
and THIGH to −6.5 °C. Each count is 1/128 °C. Use `setAlert(mode, polarity,
pinFunction)` to select the comparator and pin routing. Both setters enter
guarded shutdown and verify the separate register writes before resuming the
desired mode. Conversions can be interrupted and the pin can change between
writes. Therm mode requires TLOW ≤ THIGH. On a partial change, inspect the
desired profile and call `recover()` under stable power and exclusive access.
`readLimits(pair)` returns both observed counts. If the second transfer fails,
the settings snapshot still contains the completed TLOW observation.

In alert mode, a CONFIG read acknowledges latched HIGH and LOW, even after
temperature has returned inside the window. `getAlertStatus()` inspects the
captured events. `takeAlertEvents(out)` delivers and clears the pending bits
and loss indication in memory. Repeated excursions of the same kind coalesce.
In therm mode, HIGH sets above THIGH and clears below TLOW at a completed
comparison. Equality retains its previous level. LOW stays zero. A CONFIG
read leaves therm HIGH intact. Both CONFIG and TEMP reads acknowledge
DATA_READY. TEMP leaves the comparator flags intact.

`readAlertPinLevel(high)` samples the board's optional GPIO hook in owner
context. Under threshold routing the pin signals the comparator. Under
DATA_READY routing it signals a completed conversion. Active-low assertion
sinks the open-drain line and active-high assertion releases it to the pullup.
[Comparator and pin behavior](docs/device-behavior.md#comparator-and-pin-evidence).

## Inspect configuration trust

After initialization, `readConfiguration(out)` reads CONFIG once and returns
the observed fields. `out.raw & out.readClearMask` gives the asserted flags
acknowledged by that read.
A complete read returns success even when a changed stable field latches
`DIRTY`. To check the entire owned profile, call `verifyConfiguration()`.
It reads CONFIG, TLOW and THIGH in order, then OFFSET on capable models.
Drift returns `CONFIG_MISMATCH`. A transfer error stops the scan.

`getSettingsSnapshot()` uses no bus or clock callbacks. It shows the desired
profile, the last complete observation of each register, and the generation
and time of the last full verification. After a failed scan, `verificationReads`
identifies the registers read before the failure. While dirty, a latest
temperature read carries that trust in its sample. Call `recover()` to
replay and verify the retained target.

The owner must report a power interruption, external reset or loss of exclusive
access with `invalidateDeviceState()`. Cached words cannot detect every such
change. This call costs zero callbacks. Restore stable power and exclusive
access before calling `recover()`. Recovery checks identity, waits from the
new compatible read, and checks EEPROM lock and busy before its first write.

On an initialized TMP117, TMP117M, TMP117N or TMP119, call `reset()` to
issue one command to the bound address. It waits for the 2 ms reset interval,
checks identity, waits the separate 3 ms startup interval, then checks the
EEPROM guard and reads CONFIG, both limits and OFFSET. TMP116/N returns
`UNSUPPORTED` before I²C. After a command attempt, inspect the returned
status and `getSettingsSnapshot()`. Even a matching readback leaves the
device uninitialized. Call `recover()` when you want the desired settings
applied and verified again. [Reset and recovery](docs/api-contracts.md#startup-apply-and-reset).

## Inspect transport health

`healthSnapshot()` copies the instance's tracked I2C history without calling
the bus or the clock. To report the most recent tracked fault, including one
followed by a successful transfer, inspect its saved status.

```cpp
#include <cstdio>
#include <TMP11x/TMP11x.h>

void printLastTransportFault(const TMP11x::TMP11x& sensor) {
  const auto health = sensor.healthSnapshot();
  if (health.totalFailures != 0) {
    std::puts(TMP11x::errorName(health.lastError.code));
  }
}
```

An accepted bind, `end()` or `unbind()` starts a new history at `UNINIT`.
Each tracked callback counts once, even during initialization. A success
sets `READY` and clears the failure streak. Failures set `DEGRADED` until
the streak reaches `Config::offlineThreshold` (default 5), then `OFFLINE`.
The owner can still issue an authorized transfer and controls any backoff.
The totals and streak saturate. Check `lastSuccessMsValid` or
`lastErrorMsValid` before using the corresponding completion time. Zero is a
valid clock tick.

`probe()` and `readIdentity()` return their own status and leave tracked
transport history intact. A diagnostic-only session therefore stays at
`UNINIT`. The memory-only `isOnline()` requires initialization and a
`READY` or `DEGRADED` state. Configuration trust and conversion freshness
require their own evidence.

## Build and test

Use the `TMP11x` CMake target via `add_subdirectory`. The standalone
[native consumer](examples/native_consumer/CMakeLists.txt) checks initial
transport health and the linked version. See the [testing guide](docs/testing.md)
for host builds, CTest and version synchronization.

## Start here

- [Agent and execution guide](docs/prompts/START_HERE.md)
- [Current progress](docs/prompts/PROGRESS.md) and [preparation report](docs/reports/preparation.md)
- [Audit of the original 44 prompts](docs/reports/promptset-audit.md)
- [Review and responses to the 68-finding audit](docs/prompts/audit.md#response--2026-11-17)
- [Datasheet/functionality gap audit](docs/reports/feature-gap-audit.md) and [complete feature requirements](docs/prompts/FULL_COVERAGE.md)
- [Prompt handoff audit](docs/reports/handoff-audit.md) and [producer/consumer inventory](docs/prompts/HANDOFFS.md)
- [Library comparison](docs/library-comparison.md) and [source adoption](docs/source-adoption.md)
- [Toolchain and package plan](docs/toolchain-and-package-plan.md)
- [TI reference archive and provenance](docs/reference/README.md)
- [Ordered stages](docs/prompts/README.md) and [dependency manifest](docs/prompts/SEQUENCE.json)

## Planned product boundaries

- Repository/package: `TMP11x`; namespace/class: `TMP11x::TMP11x`;
  main header: `<TMP11x/TMP11x.h>`.
- Explicit BOM model selection for TMP116, TMP116N, TMP117, TMP117M, TMP117N,
  and TMP119. Device revision bits do not prove exact model or accuracy grade.
- TMP114, TMP118, TMP102, TMP112 and other similarly named devices are excluded.
- Portable C++17 core with application-owned I2C callbacks; thin Arduino and
  native ESP-IDF examples for ESP32-S2/S3, plus P4 where tooling permits.
- Volatile settings and read-only EEPROM diagnostics in the first release.
  No EEPROM programming, general-call reset, bus ownership or hidden retries.
- Explicit destructive-read effects, evidence of fresh conversions, bounded
  owner operations, retained results, and visible failure/recovery state.

## Local preparation checks

```sh
python scripts/check_promptset.py
python scripts/test_check_promptset.py
python scripts/check_references.py
python scripts/test_check_references.py
git diff --check
```

On Windows, `scripts\pio.cmd --version` uses the existing VS Code-managed
PlatformIO installation.

## Sources and licenses

Original TI PDFs are pinned by revision, URL and SHA-256 in
[the reference manifest](docs/reference/manifest.json). Use the original PDF
and its page references when interpreting registers; the promptset's seed
notes are not a replacement. See [SOURCE_MAP](docs/prompts/SOURCE_MAP.md) for
sibling repositories and [EVIDENCE](docs/prompts/EVIDENCE.md) for source rules.

Repository-authored code and adapted MIT support files use [LICENSE](LICENSE).
Vendor documents and external reference code retain their own copyrights
and terms; the root MIT license does not relicense them. Reference artifacts
must be excluded from library package exports.
