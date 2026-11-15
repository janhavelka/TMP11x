# 36 — Qualify clean ESP32-P4 integration and shared-bus consumer

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [33](33_native-suite.md), [35](35_idf-examples.md).

## Objective and boundaries

Provide a practical optional native ESP32-P4 integration example, while preserving the independent reusable driver package. An example target does not establish the actual PCB processor or wiring.

## Source audit

Discover relevant current `FieldCore-node` I2C owner/consumer code read-only; inspect a `p4_mainboard` source only if its actual repository/path is identified. Do not assume that name exists. Record local/remote revision differences before treating a local consumer as current GitHub evidence. Inspect pinned ESP-IDF P4 master support and TMP1x2's callback contract without changing either reference repository.

## Complete implementation work

- Add an ESP32-P4 ESP-IDF example or compile consumer with application-supplied pins, ADD0 address, bus initialization, mutex/ownership and optional TCA9548A routing. Do not invent the user's PCB pin mapping.
- Demonstrate selected `TMP117` and optional `TMP1x2` driver instances using the same external bus transport, without making TMP1x2 a mandatory dependency of TMP11x.
- Verify lifetime/cancel/mux behavior for interleaved measurement jobs and multiple sensors. Keep clock timing and temperature sample confidence explicit at the application boundary. External mux-selection traffic has its own application budget; cancellation cannot undo a completed route or sensor write.
- A physical P4 board is not required for a full firmware link. If the SDK/toolchain is unavailable, run the portable consumer and record blocked firmware commands/requirements separately from missing hardware. Avoid adding P4-only source files to mandatory core targets.
- Document the board-supplied inputs needed for reuse, without inventing pin mapping, shared-address routing, processor identity or prototype availability.

- Verify the shared-bus owner preserves each selected sensor's1–400 kHz and electrical/timeout constraints across mux routes and other device transfers. Document address/strap collisions and per-device limits; sequential one-shots do not prove broadcast-synchronized sampling, whose general-call/EEPROM recipe remains excluded.

## Verification and exit criteria

Run actual P4 firmware full link when supported; compare build artifacts and exact target. Independently run portable shared-I2C consumer. Hardware remains NOT RUN unless an actual authorized P4 fixture is present.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_36.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
