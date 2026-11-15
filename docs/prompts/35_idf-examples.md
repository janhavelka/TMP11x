# 35 — Implement native ESP-IDF examples and framework adapter

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [27](27_sync-helpers-cache.md), [30](30_cooperative-measurement.md), [33](33_native-suite.md).
Produces: `idf-adapter-source`, `idf-build-record`.
Consumes: `tooling-plan`, `board-guide`, `version-tools`, `sync-api`, `measurement-jobs`, `native-qualification`.

## Objective and boundaries

Prove the portable driver is usable with native ESP-IDF without Arduino shims or sensor-specific bus ownership.

## Source audit

Inspect TMP1x2 `examples/esp_idf/basic` CMake/PlatformIO, modern ESP-IDF `driver/i2c_master.h`, OPT4001 or LDC1614 native transport examples, and read-only current SDK APIs. Record pinned IDF version before adapting callbacks.

## Complete implementation work

- Create native ESP-IDF CMake example for ESP32-S3 and S2 using app-owned `i2c_master_bus_handle_t` and per-device handles; driver only sees I2C callback table, never SDK headers or handles.
- Use `i2c_master_transmit_receive` for the combined pointer-write/read, with terminal timeout/error mapping. Do not register asynchronous transfer callbacks on these handles: the SDK then returns before completion, violating the borrowed-buffer contract. Validate positive finite timeouts before conversion to the SDK's signed `int`; never map overflow/exhaustion to its `-1` infinite wait. Account for owner lock/route time, and document SDK tick quantization and timing limits. Do not invent a short-transfer count or NACK phase the SDK does not return.
- Implement nonblocking caller scheduling, monotonic time and a CLI transport shell that can share prompt-37's framework-neutral processor without including Arduino code. Input/ISR callbacks may queue bounded input or GPIO observations; only the owner calls driver APIs. Preserve a register-write-free observation startup path for stage 40.
- Demonstrate one device/one bus owner and coexistence with a second I2C device. GPIO and bus recovery/reset remain app-owned. Create handles only for intended sensor addresses; do not inherit TMP1x2's general-call/SMBus alert-response handles or commands.
- Register clean native ESP-IDF app linking and component-only builds, not only header syntax tests. Keep IDF-only CMake source/target dependencies outside portable native `find_package` consumers.
- Supply application-owned firmware build identity and per-boot session identity, separate from the core library version and obtained without sensor access. Stage 37 defines the protocol fields; stage 38 integrates the final CLI and refreshes linked images. Preserve precise target/SDK/source identity so stage 40 cannot select an older pre-CLI image.

- Validate the application-owned device clock against1–400 kHz and document the chip's20–40 ms held-line/no-clock timeout separately from IDF software deadlines. Verify explicit pointer, repeated START and terminal ACK/NACK/STOP via backend evidence; no sensor clock-stretch feature is assumed.
- Extend stage 4's `docs/board-integration.md` independently with IDF setup: ADD0 straps, exact package/supply/temp constraints, external pullups, 100 nF decoupling, open-drain active-high ALERT and thermal/strain conditions. This guide does not require an Arduino build. Internal MCU pullups or a successful link alone do not qualify electrical timing or accuracy.

## Verification and exit criteria

Build/verify real IDF firmware images for S2/S3 where toolchain exists, with exact IDF version and command. Record BLOCKED where unavailable, not a fake CI success.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run unrelated sibling suites or remote CI (except stage 43).

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_35.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
