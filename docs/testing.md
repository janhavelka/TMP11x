# Native builds and testing

## Host suite

From a development checkout, use CMake 3.16 or newer, a C++17 compiler,
the selected CMake generator's build tool and Python 3.10 or newer.
The Python checks use the standard library.
Keep generated output under the ignored `build/` directory.

```sh
cmake -S . -B build/host -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/host --config Release --parallel
cmake -E chdir build/host ctest -C Release --output-on-failure
```

On Windows with MinGW, add `-G "MinGW Makefiles"` to the configure command.
CTest registers these cases when `TMP11X_BUILD_TESTS=ON`.

| Test | Check |
| --- | --- |
| `version_cpp` | Rebuild the version string from its numeric constants and compare it with the header string and the linked library's version. The executable checks its results in Release builds. |
| `value_types_cpp` | Compile the new public headers in separate translation units and exercise Status, all model and address values, ID and capability literals, selector bounds, settings defaults and observation evidence. Release checks stay active and guard against ordinary C++ heap allocation during the cases. |
| `register_transport_cpp` | Check the command table against independent literals for each model, exercise all address and pointer byte values, inspect shared-bus frames and timeout grants, and script partial reads and ambiguous writes. |
| `identity_cpp` | Validate all six model selections, observed groups and revision fields. Check shared-bus TMP114 and TMP118 responses, coincidental matches, transfer failures and local refusals against the callback trace and output evidence. |
| `binding_cpp` | Exercise the public device lifecycle with the register fake. Check structural validation, rejected and accepted rebinds, callback-free teardown, retained ID evidence after errors, and two independent routes to address `0x48`. |
| `health_cpp` | Compare tracked outcomes with the callback trace, including mixed reads and writes, local refusals and an ordinary success from OFFLINE. Check neutral identity diagnostics, lifecycle resets, saturating counters, optional and wrapping timestamps, and the online predicate. |
| `temperature_cpp` | Check independent signed TEMP and threshold vectors, half-count ties, invalid inputs and round trips for all 65,536 register words. |
| `configuration_cpp` | Check 512 steady profiles per BOM selection against literal field encodings, invalid selectors, raw flags and read effects, a nonfactory reset word, MOD compatibility with and without trigger ownership, and both signed limit words. |
| `latest_cpp` | Exercise public admission and the one-read engine with the register fake. Check all six model selections, both desired modes, optional timing, caller and cache preservation, and physical read effects at the deadline. |
| `apply_cpp` | Exercise synchronous initialization and recovery and drive the same private steps individually. Inspect startup timing, EEPROM guards, comparator transitions, dirty intent, CONFIG and limit readback, and public latest reads. |
| `trust_cpp` | Exercise public configuration observation and verification on six models. Check per-field timestamps, partial scans, ambiguous writes, retained dirty causes, and recovery after external correction of an EEPROM unlock. |
| `timing_cpp` | Check all 32 CONV × AVG settings on each selected model against literal periods, standby budgets and count-scaled active estimates. Check the nominal update rate, software windows and output preservation on invalid input. |
| `conversion_settings_cpp` | Exercise 384 public steady profiles. Trace a change through shutdown and final verification, including preflight drift, ambiguous writes, late readback, retained alerts and sample provenance. |
| `averaging_blocks_cpp` | Feed synthetic counts on six models. Check TEMP and READY at block completion, acknowledgement, shutdown abort and the first full block after resuming. |
| `version_generator_check` | Compare `Version.h` and the ESP-IDF manifest's version with `library.json` using the generator's check command. |
| `version_sync` | Run the delegated version check through the checker script. |
| `version_generator_regressions` | Exercise IDF version drift and an unwritable mirror. |
| `register_ledger` | Validate the six-model register ledger and its references. |
| `register_ledger_regressions` | Exercise the ledger validator against valid and deliberately inconsistent inputs. |

Use `cmake -E chdir build/host ctest -C Release -R "^register_transport_cpp$" --output-on-failure`
to run just the transport cases. The fake records each callback's bytes and
result. Use the filters `^identity_cpp$` for the private identity cases and
`^binding_cpp$` for the public lifecycle cases.
Adapter timing, repeated START and ACK/NACK on the wires require separate
adapter and fixture evidence. [Bus framing](board-integration.md#register-transfers).

## Source consumer

CMake provides the `TMP11x` target with its public include directory and C++17
requirement. `TMP11X_BUILD_TESTS` defaults to `OFF`. With tests off, the library
uses a C++ compiler and CMake build tools. Python and Git package discovery can
be disabled without affecting the build. The example sets
`TMP11X_BUILD_TESTS=OFF` and accepts `TMP11X_SOURCE_DIR` to select a separate
library source tree. From the checkout root, configure the consumer separately.

```sh
cmake -S examples/native_consumer -B build/consumer -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE
cmake --build build/consumer --config Release --parallel
./build/consumer/tmp11x_native_consumer
```

On Windows with MinGW, add `-G "MinGW Makefiles"` and use the executable's
`.exe` suffix. With a multi-config generator, run the executable under
`build/consumer/Release/`. It checks the public register policy for TMP117
offset and reads a new device's health snapshot and online state. It prints
the linked version when these checks pass and the header version matches.

## Version and package checks

From the checkout root, edit the `version` in `library.json` and run `sync`
to update the generated header and the top-level version in
`idf_component.yml`. Keep the double-quoted IDF version on the first content
line. Use three decimal components in `X.Y.Z` form, each in the range 0
through 65535. Both check commands are read-only and exit nonzero on drift.

```sh
python scripts/generate_version.py sync
python scripts/generate_version.py check
python tools/check_version_sync.py
```

On Windows, use the managed PlatformIO launcher and inspect the archive
locally. Create the ignored output directory before packing.

```powershell
New-Item -ItemType Directory -Force build/package | Out-Null
scripts/pio.cmd package pack --output build/package/TMP11x.tar.gz
tar -tf build/package/TMP11x.tar.gz
```

The archive contains the portable source and headers, build and package
metadata, MIT license, changelog, README and native consumer. The host suite
uses the development checkout. Firmware links, installed CMake package
qualification, remote CI and physical fixtures have their own
[verification requirements](prompts/VERIFICATION.md).
