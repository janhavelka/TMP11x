# 35 — Implement native ESP-IDF examples and framework adapter

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [27](27_sync-helpers-cache.md), [30](30_cooperative-measurement.md), [33](33_native-suite.md).

## Objective and boundaries

Prove the portable driver is usable with native ESP-IDF without Arduino shims or sensor-specific bus ownership.

## Source audit

Inspect TMP1x2 `examples/esp_idf/basic` CMake/PlatformIO, modern ESP-IDF `driver/i2c_master.h`, OPT4001 or LDC1614 native transport examples, and read-only current SDK APIs. Record pinned IDF version before adapting callbacks.

## Complete implementation work

- Create native ESP-IDF CMake example for ESP32-S3 and S2 using app-owned `i2c_master_bus_handle_t` and per-device handles; driver only sees I2C callback table, never SDK headers or handles.
- Ensure an atomic write-then-read action with clear timeout/error mapping. Validate actual API semantics and end-of-transfer lifetime against SDK implementation; do not pretend unsupported combined transaction or short-transfer status exists.
- Implement nonblocking caller scheduling, monotonic time and a CLI transport shell that can share prompt-37's framework-neutral processor without including Arduino code.
- Demonstrate one device/one bus owner and coexistence with a second I2C device. Add an explicit rule that GPIO and bus recovery/reset remain app-owned.
- Register clean native ESP-IDF app linking and component-only builds, not only header syntax tests. Keep IDF-only CMake source/target dependencies outside portable native `find_package` consumers.

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
