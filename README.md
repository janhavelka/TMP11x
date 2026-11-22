# TMP11x

A new, framework-neutral C++17 driver project for TI TMP116 / TMP116N,
TMP117 / TMP117M / TMP117N, and TMP119. TMP117 is the primary intended sensor.

Development version `0.1.0` provides portable C++17 status, binding, settings,
model capability and observation types through `<TMP11x/TMP11x.h>`.
`TMP11x::getCapabilities()` reports features of an explicit BOM selection
without accessing hardware. `<TMP11x/CommandTable.h>` exposes the register
pointers, field masks and per-model access classes. The private transport
has native tests for one-word reads and guarded writes. The public device
class supports bus-silent binding, teardown, explicit family checks and
passive transport health. The managed latest-read API requires verified
initialization, which arrives in stage 18. Physical qualification is pending.
`<TMP11x/Version.h>` exposes `TMP11x::version()` for the linked library.
[Value and binding contracts](docs/api-contracts.md).

Use `<TMP11x/Temperature.h>` to convert a TEMP or limit word without a
device. For example, `temperature::decodeCounts(0xFFFF)` in namespace
`TMP11x` returns −1 count, or −0.0078125 °C. A stored TEMP value alone
cannot establish when the sensor completed a conversion.

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
