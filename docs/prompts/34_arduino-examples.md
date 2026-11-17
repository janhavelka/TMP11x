# 34 — Implement Arduino/PlatformIO adapters and reusable example

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [27](27_sync-helpers-cache.md), [30](30_cooperative-measurement.md), [33](33_native-suite.md).
Produces: `arduino-adapter-source`, `arduino-build-record`.
Consumes: `tooling-plan`, `board-guide`, `version-tools`, `sync-api`, `measurement-jobs`, `native-qualification`.

## Objective and boundaries

Demonstrate a real, minimal, application-owned I2C integration of the portable driver with ESP32-S2/S3 while keeping board setup and console code outside production source.

## Source audit

Study TMP1x2 `examples/01_basic_bringup_cli/main.cpp` (including its inline Wire callbacks), `examples/common/BoardConfig.h`, `platformio.ini`, and `scripts/pio.cmd`; the audited TMP1x2 revision has no `examples/common/I2cTransport.h`. Inspect TCA9548A for an optional multiple-device I2C multiplexing boundary. Verify the pinned Arduino-ESP32 Wire implementation, not only its API names.

## Complete implementation work

- Implement ESP32-S2 and ESP32-S3 PlatformIO `arduino` example profiles using the existing managed PlatformIO installation. Keep examples self-contained and avoid vendor/factory pin defaults in core.
- Implement `Wire` terminal callbacks for pointer+two-byte reads and pointer+two-byte writes. Verify repeated START and completion in the pinned Wire version; validate its `uint16_t` timeout range before narrowing. Budget application lock/route selection and transfer time explicitly, and never extend an exhausted caller deadline. Map short reads and errors without claiming an address/data NACK distinction that the backend cannot prove. Check TX-buffer acceptance before committing a write; an error cleanup must not transmit an unintended partial register value.
- Examples demonstrate a BOM-selected TMP117/address, revision-aware identity and temperature/readiness provenance. Provide an observation startup path using bus-silent binding and approved diagnostics; no automatic `begin`, reset, one-shot or configuration write may precede a claimed read-only session. An explicit initialization demonstration may write safe volatile settings and must identify that behavior.
- Add an optional second sensor instance on the same shared bus to demonstrate external serialization; if a TCA9548A mux is used, establish route before and hold it for each transfer only, not an entire conversion wait. Count mux-selection transfers separately in application scheduling; the driver's sensor-transfer budget does not bound external route traffic.
- Deliver a fixed observation demo plus bounded byte-I/O hooks; stage 37's shared processor owns line assembly, overflow handling and command parsing. Stage 38 owns attaching that processor. Reading CONFIG/TEMP can acknowledge flags even when no data register is written. A fresh one-shot demonstration is an explicit mutating operation, not a read-only temperature query.
- Supply application-owned firmware build identity (source revision or explicit unknown/dirty state, target, build identifier) and a per-boot session identity without sensor I2C. Library `Version.h` is a separate value. Stage 37 freezes the wire fields; stage 38 connects this metadata and the final CLI, then refreshes the linked image. This initial example image alone is not the later HIL firmware handoff.
- Count every adapter transfer attempt from boot, including health-neutral diagnostics: pointer-only writes, sensor data writes, reads and separate mux/other-device transfers. Expose a passive snapshot for the stage 37 `xfer` command. Lost/saturated counter continuity must be explicit; library health counters alone cannot establish an observation session's write history.
- Inspect the pinned Wire/IDF timeout implementation and actual RTOS tick rate. Apply the computable admission policy below; reject insufficient budgets before sensor I2C, without rounding beyond the caller's remaining time. Verify terminal cleanup and post-callback timing on failure. Record backend limits and SDK-owned bus-clear behavior separately from driver retries.
- If this stage resumes after stage 38, attach the already completed CLI, rebuild the target and update `docs/firmware-cli-handoff.md`; stage 38 need not rerun merely to recover a previously unavailable SDK.

- Enforce the example-owned bus configuration within 1–400 kHz; document hardware 20–40 ms line-held/no-clock timeout separately from Wire's timeout. Show actual ADD0 mapping, package pins, supply/logic compatibility, external SDA/ALERT pullups and 100 nF bypass; active-high ALERT remains open-drain.
- Extend stage 4's `docs/board-integration.md` with actual Arduino setup, covering exact-grade accuracy conditions, self-heating from I2C/averaging, thermal coupling/settling and package-specific pad/strain guidance. Default example setup is not proof of calibrated accuracy. Preserve the TMP116 ALERT-to-ground wording conflict; do not ground a used ALERT signal.

## Verification and exit criteria

For the selected truncating tick backend, let `P = 1000/configTICK_RATE_HZ`
milliseconds and `t = pdMS_TO_TICKS(sdkTimeoutMs)` using its actual conversion.
Require `t >= 2` and `(t - 1) * P >= Ttransfer`: one tick has no positive
minimum wait independent of tick phase. Derive `Ttransfer` from the bounded
configured SCL period (36 clock periods for a register write, 45 for a combined
register read), START/STOP timing and explicit controller/interrupt-service
allowances. These are example policy assumptions, not a datasheet deadline.
Name the numerical/configured allowance inputs and use overflow-safe arithmetic;
do not silently use nominal SCL as a guaranteed maximum transfer time.

After lock/route time, also require `t * P <= remainingMs` and the stronger
`Bbackend(t) + cleanupReserve <= remainingMs`, where the pinned source's
whole-path bound includes every potentially sequential SDK wait, recovery and
scheduling allowance. Reused full timeouts cannot be counted only once. Perform
the bound calculation as the maximum over reachable backend paths of
`sum(maxCalls(j) * waitBound(j, t)) + setupReserve + recoveryReserve + scheduleReserve`.
The source audit must supply finite counts and numerical/configured reserves;
normal tick waits contribute up to `t * P` before scheduling delay, while the
pinned IDF strict-`>` NACK loop needs an additional tick. These are not unnamed
"sufficient margin" constants. Perform
an initial admission check before route traffic and recheck remaining time after
the owner lock/route; a later rejection performs no sensor transfer, though an
already completed mux selection remains separately counted. Test 0/1/2 ticks,
tick-phase edges, slow configured SCL and deadline exhaustion during lock/route.
If a finite supported bound or terminal borrowed-buffer cleanup cannot be
established, record that adapter qualification BLOCKED; a two-tick floor alone
does not qualify it.

Run real PlatformIO builds for S2 and S3, or record exact unavailable framework versions. Native host CLI tests do not establish full MCU linking; no physical sensor reading is claimed before hardware qualification.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_34.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
