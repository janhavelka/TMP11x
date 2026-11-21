# Native builds and testing

## Host suite

From a development checkout, use CMake 3.16 or newer, a C++17 compiler,
the selected CMake generator's build tool and Python 3.10 or newer.
The Python checks use the standard library.

```sh
cmake -S . -B build-tests -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests --config Release --parallel
cd build-tests
ctest -C Release --output-on-failure
```

On Windows with MinGW, add `-G "MinGW Makefiles"` to the configure command.
CTest registers these cases when `TMP11X_BUILD_TESTS=ON`.

| Test | Check |
| --- | --- |
| `version_cpp` | Rebuild the version string from its numeric constants and compare it with the header string and the linked library's version. The executable checks its results in Release builds. |
| `version_generator_check` | Compare `Version.h` and the ESP-IDF manifest's version with `library.json` using the generator's check command. |
| `version_sync` | Run the delegated version check through the checker script. |
| `register_ledger` | Validate the six-model register ledger and its references. |
| `register_ledger_regressions` | Exercise the ledger validator against valid and deliberately inconsistent inputs. |

## Source consumer

CMake provides the `TMP11x` target with its public include directory and C++17
requirement. `TMP11X_BUILD_TESTS` defaults to `OFF`. With tests off, the library
uses a C++ compiler and CMake build tools. Python and Git package discovery can
be disabled without affecting the build. The example sets
`TMP11X_BUILD_TESTS=OFF` and accepts `TMP11X_SOURCE_DIR` to select a separate
library source tree. From the checkout root, configure the consumer separately.

```sh
cmake -S examples/native_consumer -B build-consumer -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE
cmake --build build-consumer --config Release --parallel
./build-consumer/tmp11x_native_consumer
```

On Windows with MinGW, add `-G "MinGW Makefiles"` and use the executable's
`.exe` suffix. With a multi-config generator, run the executable under
`build-consumer/Release/`. It prints the linked version and exits successfully
when it matches the version in the header.

## Version and package checks

From the checkout root, edit the `version` in `library.json` and run `sync`
to update the generated header and the top-level version in
`idf_component.yml`. Use three decimal components in `X.Y.Z` form, each in
the range 0 through 65535. Both check commands are read-only and exit nonzero
on drift.

```sh
python scripts/generate_version.py sync
python scripts/generate_version.py check
python tools/check_version_sync.py
```

On Windows, use the managed PlatformIO launcher and inspect the archive
locally. Create the ignored output directory before packing.

```powershell
New-Item -ItemType Directory -Force build-package | Out-Null
scripts/pio.cmd package pack --output build-package/TMP11x.tar.gz
tar -tf build-package/TMP11x.tar.gz
```

The archive contains the portable source and headers, build and package
metadata, MIT license, changelog, README and native consumer. The host suite
uses the development checkout. Firmware links, installed CMake package
qualification, remote CI and physical fixtures have their own
[verification requirements](prompts/VERIFICATION.md).
