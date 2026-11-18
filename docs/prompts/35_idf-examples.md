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
- Implement nonblocking caller scheduling, monotonic time, a fixed observation demo and bounded byte-I/O hooks for the later shared processor. Stage 37 owns line assembly, overflow handling and command parsing; stage 38 attaches it. Input/ISR callbacks may queue bounded input or GPIO observations; only the owner calls driver APIs. Preserve a register-write-free observation startup path for stage 40.
- Demonstrate one device/one bus owner and coexistence with a second I2C device. GPIO and bus recovery/reset remain app-owned. Create handles only for intended sensor addresses; do not inherit TMP1x2's general-call/SMBus alert-response handles or commands.
- Register clean native ESP-IDF app linking, including a minimal app linking the TMP11x component, not only header syntax tests. An IDF component needs a project to build. Keep IDF-only CMake source/target dependencies outside portable native `find_package` consumers.
- Supply application-owned firmware build identity and per-boot session identity, separate from the core library version and obtained without sensor access. Stage 37 defines the protocol fields; stage 38 integrates the final CLI and refreshes linked images. Preserve precise target/SDK/source identity so stage 40 cannot select an older pre-CLI image.
- Set and record the example's `CONFIG_FREERTOS_HZ` (1000 Hz is the selected example default). Apply the computable tick/budget policy below using the actual SDK conversion. Reject insufficient budgets before sensor I2C, without rounding beyond the caller's deadline. A two-tick floor or 1000 Hz scheduler alone does not prove terminal cleanup or a hard wall-time bound.
- Document the pinned SDK's own timeout recovery/bus-clear behavior. It is adapter/backend behavior, not a TMP11x retry, and must fit the application bus-ownership contract. If the SDK cannot satisfy terminal borrowed-buffer lifetime or the declared deadline bound, report that adapter gate BLOCKED rather than weaken the core contract.
- Count all adapter attempts from boot, separating pointer-only writes, sensor data writes, reads and any mux traffic; provide a bus-silent snapshot for `xfer`. Include health-neutral diagnostics and mark counter saturation/continuity loss.
- If this stage resumes after stage 38, attach the final shared CLI, rebuild and update `docs/firmware-cli-handoff.md` with the new image; do not leave integration ownerless after a late SDK installation.

- Validate the application-owned device clock against 1–400 kHz and document the chip's 20–40 ms held-line/no-clock timeout separately from IDF software deadlines. Verify explicit pointer, repeated START and terminal ACK/NACK/STOP via backend evidence; no sensor clock-stretch feature is assumed.
- Extend stage 4's `docs/board-integration.md` independently with IDF setup: ADD0 straps, exact package/supply/temp constraints, external pullups, 100 nF decoupling, open-drain active-high ALERT and thermal/strain conditions. This guide does not require an Arduino build. Internal MCU pullups or a successful link alone do not qualify electrical timing or accuracy.

## Verification and exit criteria

For this truncating SDK backend, define `P = 1000/configTICK_RATE_HZ` ms and
`t = pdMS_TO_TICKS(sdkTimeoutMs)` using the pinned conversion. Require `t >= 2`
and `(t - 1) * P >= Ttransfer`. Derive `Ttransfer` from the bounded configured
SCL period (36 clocks for a register write, 45 for a combined register read),
START/STOP timing and named controller/interrupt-service allowances. Record
their numerical/configured values and assumptions, using overflow-safe
arithmetic. Nominal SCL and tick conversion alone are not worst-case evidence.

Before passing a finite millisecond argument to the SDK, clip it in wider
checked arithmetic to the remaining budget, signed-int range and the selected
conversion's safe range. The pinned v5.5.5 `FreeRTOS-Kernel` macro
(`CONFIG_FREERTOS_SMP=n`, including the default dual-core S3 kernel) multiplies
32-bit TickType_t operands before division, so also require
`sdkTimeoutMs <= UINT32_MAX / configTICK_RATE_HZ`. The alternate
`FreeRTOS-Kernel-SMP` macro uses a uint64_t product; verify the actual selected
macro/overrides and final tick range. A wide local calculation alone cannot fix
the SDK's later overflow. Test the safe cap and one millisecond above it without
sensor I2C; never round a clipped timeout beyond the remaining deadline.

After subtracting actual owner lock/route time, require `t * P <= remainingMs`
and `Bbackend(t) + cleanupReserve <= remainingMs`. `Bbackend` must include all
potentially sequential SDK semaphore/event/NACK waits, SDK recovery and
scheduling allowance; the SDK can reuse the same timeout for several waits.
Calculate it as the maximum over reachable backend paths of
`sum(maxCalls(j) * waitBound(j, t)) + setupReserve + recoveryReserve + scheduleReserve`,
with finite source-derived counts and numerical/configured reserves. A normal
tick wait contributes up to `t * P` before scheduling delay; the pinned strict-`>`
NACK loop needs another tick. Do not hide missing evidence in an unnamed margin.
On pinned v5.5.5 S2/S3, the hardware-clear recovery loop has a fixed 50-ms
setting independent of the caller timeout. Account for
`(pdMS_TO_TICKS(50) + 1) * P` per reachable clear, plus bounded setup/scheduling
costs and the maximum call count on a path. The extra tick follows its strict
`>` test; 50 ms is not an exact wall-time maximum. Recovery may run before a
new transfer on a previously busy/timed-out bus or during error handling.
Reject budgets that cannot cover the full reachable path, even when the normal
register transaction would fit; a smaller SDK timeout does not clip this loop.
Check admission before route activity and remaining time again after it. An
exhausted budget returns a typed error without sensor I2C; any already completed
mux write remains separately counted. Test 0/1/2 ticks, tick-phase edges,
configured slow-SCL durations, lock/route exhaustion, late return and buffer
lifetime after failure. Unknown finite bounds or cleanup leave the adapter
qualification gate BLOCKED instead of turning the inequalities into a false
end-to-end guarantee.

Build/verify real IDF firmware images for S2/S3 where toolchain exists, with exact IDF version and command. Record BLOCKED where unavailable, not a fake CI success.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_35.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
