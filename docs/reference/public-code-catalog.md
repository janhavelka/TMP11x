# Public code and vendor tooling catalog

Inspected on 2026-11-15. These sources support independent comparisons; TI's
exact-part datasheets govern silicon behavior. No code from this catalog has
been copied into the production library. GitHub file contents were re-read at
the listed commits, not inferred from an unpinned default branch.

## Linux IIO TMP11x driver — factual comparison only

- Repository revision: `23c62304dde29abe92013d85910d03a77dada35d`.
- [drivers/iio/temperature/tmp117.c](https://github.com/torvalds/linux/blob/23c62304dde29abe92013d85910d03a77dada35d/drivers/iio/temperature/tmp117.c),
  blob `2d0e79416434bec0c03469ce71094882952098df`.
- License: file SPDX **GPL-2.0-only**. Do not copy into the MIT driver.
- Useful comparisons: swapped SMBus word reads, signed temperature/offset
  interpretation and separate TMP116 channels without calibration offset.
- Differences to reject here: `tmp117_probe` selects a model using full
  `0x0117`/`0x2117` words; our BOM-selected capabilities must retain revision
  separately. `tmp117_write_raw` directly writes offset without checking EEPROM
  lock/busy, and caches bias initialized to zero. Those assumptions do not meet
  this project's configuration-trust and nonvolatile-safety contract.
- The driver includes TMP119 support at this revision. An older source map
  claiming only TMP116/TMP117 would be stale. No Linux build or runtime test ran.

## Adafruit Arduino TMP117 — comparison, not portable core

- Repository revision: `6876d913b529ce408060f38013037e5ed0c1abad` (1.0.5).
- [Adafruit_TMP117.cpp](https://github.com/adafruit/Adafruit_TMP117/blob/6876d913b529ce408060f38013037e5ed0c1abad/Adafruit_TMP117.cpp),
  blob `b172e939b3cfbe688e1694bbf06f1a0c6c5f7849`.
- [license.txt](https://github.com/adafruit/Adafruit_TMP117/blob/6876d913b529ce408060f38013037e5ed0c1abad/license.txt):
  BSD 3-clause terms; copyright 2020 Bryan Siepert for Adafruit Industries.
  Any later adaptation must retain that notice and terms; it is not MIT code.
- Useful review targets: explicit big-endian registers, readiness/alert cache,
  threshold APIs and averaging vocabulary.
- Observed mismatch: `interruptsActiveLow(true)` writes `1` into POL, whereas
  TMP117 Table 7-6 defines `1` as active high. Do not inherit the naming/bit
  relationship. `waitForData` loops without a timeout; reset invokes it.
  `getEvent` returns true without a checked transport-result path. These differ
  from the required terminal callbacks, bounded operations and explicit errors.
- Arduino dependencies, heap ownership, delays and implicit reset are excluded
  from the portable core. No Adafruit build or hardware test ran.

## TI official tooling and examples

| Official source | Version/evidence | Availability and boundary |
|---|---|---|
| [TMP117 ASC Studio](https://dev.ti.com/sysconfig/index.html?module=%2Fti%2Fsensors%2Ftempsensor%2FTMP117&product=ascstudio) | Linked by the [TI product page](https://www.ti.com/product/TMP117); no immutable generated-code revision acquired | The app request timed out. Generated code and its license were not inspected; no adoption claim. |
| [TMP117EVM software](https://www.ti.com/tool/TMP117EVM) | Product page lists GUI release August 23, 2024; [SNOU161](source/snou161-tmp117evm.pdf) documents evaluation | GUI is an evaluation reference; firmware source/commit/license not acquired. Downloaded the guide and quick start, not executable installers. |
| [TMP116EVM TIDCDE4](https://www.ti.com/tool/download/TIDCDE4) | Product page lists `01.00.00.0B`, August 1, 2017; [SNOU145A](source/snou145a-tmp116evm.pdf) | TI download is export-approval gated. Not downloaded; license/source inspection remains open. |
| [TMP119EVM](https://www.ti.com/tool/TMP119EVM) | [SNOU199](source/snou199-tmp119evm.pdf), December 2023 | Guide acquired. Firmware source/commit/license not acquired. |

Availability of a TI GUI or code generator is not permission to relicense its
output. If later research needs TI software, record the exact release, original
license and inspected files first. E2E forum posts also remain secondary support
evidence; see the unresolved access item in [intake notes](extracted/intake-notes.md).
