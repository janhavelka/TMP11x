# Verification layers and required evidence

Preparation checks available now (Python 3.10+):

```sh
python scripts/check_promptset.py
python scripts/test_check_promptset.py
python scripts/check_references.py
python scripts/test_check_references.py
git diff --check
```

These prove navigation/dependency consistency and pinned archive integrity,
including corruption/no-overwrite behavior. They do not validate register
interpretation or establish any of the implementation layers below.

- **Native unit:** standalone CMake C++17 build and CTest with real registered
  assertions, fake I2C device model and fault injections. No Arduino/SDK.
- **Core boundary:** include each public header in isolation; compile/link
  with no Arduino.h, Wire.h, FreeRTOS, Python requirement or sibling checkout.
- **Fuzz/negative:** bit vectors, invalid fields/addresses/models, failed
  callbacks, unexpected short transfers, read-to-clear flags, raw guard,
  EEPROM unlocked/busy, clock wrap/deadline, cancellation and retained tokens.
- **Firmware build:** full linked Arduino-ESP32 examples for supported boards;
  native ESP-IDF S2/S3, plus ESP32-P4 when matching SDK/toolchain is installed.
  A header-only compile is not a linked firmware build.
- **Application consumer:** TMP11x and TMP1x2 coexist using one application
  I2C owner; demonstrate no dependency between sensor driver packages.
- **Package:** separate extracted source and installed/relocated CMake
  `find_package` consumer; no accidental source-tree paths, SDK, Python or
  Git needed for clean consumer build; version and license correct.
- **CLI/Python:** compiled CLI fixture and parser/runner tests; finite line
  limits, noise/drop handling, exact command/result correlation, safe device
  selection and real status.
- **Remote CI:** first complete checkpoint only in stage 43; record exact SHA,
  workflow/job URLs, tool versions, final state. Do not equate old green SHA
  to later edits.
- **Physical/HIL:** exact part marking/ordering code, address straps, board
  revision, power, I2C clock, wiring, instrument, fixture condition, firmware
  image, runner revision, raw timestamped transcript, faults, and restored
  volatile state. HIL must not mutate EEPROM or other devices on the bus.

At stage completion record PASS/FAIL/NOT RUN/BLOCKED for each affected layer,
with commands, artifacts, version, identity. Unavailable hardware is NOT RUN,
not PASS. A failure is never 'fixed' by skipping the test or weakening checks.

Typical target commands **after the agent has implemented them**:

```sh
python scripts/check_promptset.py
cmake -S . -B build-tests -DTMP11X_BUILD_TESTS=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
python tools/check_version_sync.py
python tools/check_package.py
```

On Windows use `-G "MinGW Makefiles"` or an available compatible CMake
generator. PlatformIO uses the adapted `scripts/pio.cmd`. Do not claim these
implementation-dependent commands pass before their files and tests exist.
