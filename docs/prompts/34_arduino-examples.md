# 34 — Implement Arduino/PlatformIO adapters and reusable example

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [27](27_sync-helpers-cache.md), [30](30_cooperative-measurement.md), [33](33_native-suite.md).

## Objective and boundaries

Demonstrate a real, minimal, application-owned I2C integration of the portable driver with ESP32-S2/S3 while keeping board setup and console code outside production source.

## Source audit

Study TMP1x2 `examples/01_basic_bringup_cli/main.cpp`, `examples/common/{BoardConfig.h,I2cTransport.h}`, its `platformio.ini`, and `scripts/pio.cmd`; inspect TCA9548A for an optional multiple-device I2C multiplexing boundary.

## Complete implementation work

- Implement ESP32-S2 and ESP32-S3 PlatformIO `arduino` example profiles using the existing managed PlatformIO installation. Keep examples self-contained and avoid vendor/factory pin defaults in core.
- Implement `Wire` terminal callbacks for pointer+word reads and complete bounded writes; use ESP32 Arduino repeated START, explicit timeout support/limitations and correct status mapping (do not claim address/data NACK distinction where Wire does not prove it).
- Examples demonstrate reading a TMP117, selecting expected model and address from the BOM, printing revision-aware identity, reading temperature and checking readiness without unsafe EEPROM writes.
- Add an optional second sensor instance on the same shared bus to demonstrate external serialization; if a TCA9548A mux is used, establish route before and hold it for each transfer only, not an entire conversion wait.
- Provide a read-only baseline diagnostic CLI endpoint (help/identity/temperature/status) as a thin harness; prompt 37 will add the shared authoritative CLI engine. Avoid a second production parser.

## Verification and exit criteria

Run real PlatformIO builds for S2 and S3, or record exact unavailable framework versions. Native host CLI tests do not establish full MCU linking; no physical sensor reading is claimed before hardware qualification.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_34.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
