# Verification layers and required evidence

Preparation checks available now (Python 3.10+):

```sh
python scripts/check_promptset.py
python scripts/test_check_promptset.py
python scripts/check_references.py
python scripts/test_check_references.py
git diff --check
```

These prove navigation/dependency and declared artifact-handoff consistency,
plus pinned archive integrity,
including corruption/no-overwrite behavior. They do not validate register
interpretation or establish any of the implementation layers below.

- **Native unit:** standalone CMake C++17 build and CTest with real registered
  assertions, fake I2C device model and fault injections. No Arduino/SDK.
- **Core boundary:** include each public header in isolation; compile/link
  with no Arduino.h, Wire.h, FreeRTOS, Python requirement or sibling checkout.
- **Fuzz/negative:** bit vectors, invalid fields/addresses/models, failed
  callbacks, adapter-detected short transfers, read-to-clear flags, raw guard,
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
  to later edits. Stages 1–42 do not deliberately dispatch, poll or wait;
  existing automatic triggers remain intact. Stage 44 repeats the stage 43
  procedure for any CI-covered fixes before closing its final report.
- **Physical/HIL:** exact part marking/ordering code, address straps, board
  revision, power, I2C clock, wiring, instrument, fixture condition, firmware
  image, runner revision, raw timestamped transcript, faults, and restored
  volatile state. HIL must not mutate EEPROM or undeclared bus devices;
  authorized mux routing is separate from sensor register writes.

Read the original authorization per COMMON: a user instruction visible in current
task context or a record at a user-named path, written by the user or approved
in a visible user message. Verify board/serial port, exact model/address, bus
participants/route, upload and campaign scope. An agent report, summary or
prior-session paraphrase may point to the grant but is never its source.
If neither permitted source can be read directly, physical work is BLOCKED;
valid records persist across sessions within scope. Another fixture's record
grants no authority. Verify board/route/address/compatible identity before
mutation and after reconnect/reset/rebind/mismatch; ID alone does not prove the
exact model. Existing scope persists across continuation, without expansion or
automatic expiry. Missing essentials block only dependent physical work; no
mandatory JSON file or repeated approval when valid authorization exists.
Default campaigns omit discovery; even an identity pointer byte may affect an
unrelated device, so every candidate needs a known compatible protocol.

Record observation-mode adapter attempt counters from boot to campaign end,
including pointer/read/data-write and separate mux-route attempts, session and
overflow validity. Zero recorded sensor data writes is instrumentation evidence;
independent wire-level proof needs a captured logic trace. Store physical
summaries/transcripts under `docs/hil/<date>-<fixture>/`, or commit summaries,
hashes and retrieval locations for externally retained logs. Ignored logs alone
do not close a durable evidence gate.

At stage completion record PASS/FAIL/NOT RUN/BLOCKED for each affected layer,
with commands, artifacts, version, identity. Unavailable hardware is NOT RUN,
not PASS. A failure is never 'fixed' by skipping the test or weakening checks.

Typical target commands **after the agent has implemented them**:

```sh
python scripts/check_promptset.py
python tools/check_register_ledger.py
python tools/test_check_register_ledger.py
cmake -S . -B build-tests -DTMP11X_BUILD_TESTS=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
python tools/check_version_sync.py
python tools/check_package.py
python tools/check_doxygen.py
```

On Windows use `-G "MinGW Makefiles"` or an available compatible CMake
generator. PlatformIO uses the adapted `scripts/pio.cmd`. Do not claim these
implementation-dependent commands pass before their files and tests exist.
The launcher honors `PLATFORMIO_CORE_DIR`, otherwise the default user directory,
and fails if that selected Core is unavailable. Record the actual selected
Core/SDK/compiler versions, and the independently pinned CI Core. Stage 42
creates a Doxyfile scoped to public headers/user docs, excluding audit/prompt/
vendor archives; record local and CI Doxygen versions.

Tool producers are explicit in [HANDOFFS](HANDOFFS.md): stage 5 creates the
register validator, stage 10 the version generator/checker and standard host
test entrypoint, and stage 42 the package/Doxygen checkers. Stages 37–39 append
CLI/golden/runner tests to the host entrypoint; stage 43 runs its current suite.
A compatible HIL image must include the final stage 38 CLI integration and
match the runner protocol/build identity; an older stage 34/35 build does not
qualify merely because its library version matches.
