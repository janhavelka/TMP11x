# CLI and HIL contract

The diagnostic processor is example-only and framework-neutral, shared between
Arduino and native ESP-IDF. Adapt TMP1x2's bounded line buffer and result
correlation; do not introduce an independent direct-I2C path that bypasses
managed driver state. CLI and Python runners are not dependencies of the core.

## Canonical command names and effects

Use these names in help, parser, documentation and runner tests. Stage 37
freezes arguments and output fields; do not create competing `average`,
`threshold`, `temp` or `fresh` spellings without an explicit alias contract.

| Command | Required semantics |
|---|---|
| `help`, `version`, `model`, `addr`, `caps`, `health`, `settings`, `jobs`, `bus` | Query cached/software state without I2C. `settings` reports desired/trusted state; observed CONFIG uses `config`. Any model/address setter is a separate explicit binding change, rejected while a job/result owns the instance. |
| `latest` | Return the cached sample with validity and acquisition provenance; no I2C and no new-conversion claim. |
| `bind <model> <addr>`, `unbind`, `end` | Bus-silent lifecycle actions; bind validates the full available profile and preserves the instance on rejection. Stage 37 freezes omitted/default arguments; stage 38 wires final routes. |
| `begin` | Explicit guarded initialization; never run implicitly when connecting or issuing observation commands. |
| `read` | Read the latest TEMP register through the driver; clears DATA_READY and does not by itself establish freshness. |
| `read --fresh` | Bounded continuous-mode try-fresh acquisition using owned hardware evidence; no conversion trigger or mode change. Return not-ready when unproven. |
| `identity`, `config`, `status`, `dump`, `eeprom` | Explicit register reads. Report CONFIG/TEMP flag consumption and retained alert evidence; multi-register dumps first verify family identity, stopping on error/mismatch. Sequential observations are not atomic snapshots; EEPROM reads observe shadows. |
| `discover` | Read identity only at application-authorized candidates within 0x48–0x4B. Those addresses may contain unrelated devices; require fixture/bus ownership, never run discovery implicitly. Do not infer TMP119 or grade from revision bits. |
| `mode`, `cycle`, `avg`, `limits`, `alert`, `offset` | Explicit volatile-register changes that remain the desired profile, never NVM writes. `mode` accepts continuous/shutdown; one-shot is separate. Offset management is opt-in on TMP117/TMP119 and defaults off, preserving observed calibration. No implicit restoration after an intentional setter. |
| `oneshot`, `recover`, `reset` | Explicit typed guarded mutation. `reset` is TMP117/TMP119 only, with unknown reset effects followed by safe-state/full readback; no general-call fallback. Report resulting trust. |
| `shutdown` | Exact alias for `mode shutdown`, including desired-mode update and guards, owned by stage 20 and integrated in 38. |
| `invalidate`, `job cancel`, `job result` | Bus-silent state/result actions under the owner/result contract. Cancellation cannot undo an earlier hardware effect. |
| `job start init/configure/recover/read/tryfresh/oneshot`, `job step` | Explicit operation kind and bounded timeout; read/try-fresh/one-shot are distinct. Stage 37 freezes profile arguments and framing; stage 38 implements all kinds. No low-level I2C bypass. |
| `job auto [0/1]` | Default automatic owner-loop polling with a fixed per-turn transfer budget. Manual steps require a finite explicit deadline that includes host delay. |
| `watch` | Finite register-read acquisition by default. One-shot or configuration changes require an explicit option/campaign with declared effects. |
| `stop` | Bus-silent stop/cancel of watch or campaign scheduling; record partial hardware effects. Any restoration uses a separately declared guarded operation, never hidden inside stop. |
| `xfer` | Bus-silent adapter attempt counters since boot: pointer-write, sensor data-write, read and authorized mux-route attempts, session ID and overflow/saturation. Includes health-neutral traffic; never reset invisibly. |

Register-read-only means no data-register writes, not no side effects: pointer
writes select the register and CONFIG/TEMP reads can consume flags. Bind a
read-only session without `begin`/recovery/configuration writes, including at
firmware boot. Never program EEPROM, mutate its shadows, or broadcast reset.
Use the bound-only diagnostic route when managed initialization is absent;
return unknown startup/configuration/freshness rather than auto-initializing.
Continuous `read --fresh` requires the known mode/evidence preconditions from
the API contract; observation-only sessions may refuse it while allowing raw
`read`, `config` and `identity`.
`oneshot` and one-shot watch obey the same shutdown-only API precondition:
desired continuous mode rejects without I2C, and the caller explicitly invokes
shutdown first. CLI commands must not silently change or restore desired mode.
The explicit one-shot engine then establishes its guarded shutdown/baseline
boundary; it does not infer shutdown from undocumented MOD readback. All
unrelated I2C commands return BUSY while a job/result owns the instance.
Stage 37 freezes a bounded request-ID envelope; responses echo that ID and
session. No additional marker/alias command is needed solely for correlation.

Temporary test campaigns snapshot restorable settings and verify restoration
or report failure. Restore only writable stable fields, with reset commands
masked and mode normalization handled; flags, conversion phase, consumed
readiness and prior temperature cannot be restored. Do not replay a captured
one-shot command or raw CONFIG word. The user must select safe fixture
conditions for mutating campaigns; no such campaign runs on connection alone.
Snapshot offset management separately from observed offset so restoration never
silently acquires ownership of an unmanaged calibration. MOD=11 snapshots
cannot be replayed; restorable shutdown uses the explicit owned mode policy.
An offset-changing campaign on an unmanaged sensor must capture a valid original
raw offset first. Restore that exact raw value through an authorized guarded
write/readback, then restore unmanaged ownership; merely clearing manageOffset
would leave the temporary calibration in hardware. If the original value is
unknown, refuse the campaign; if restoration cannot complete, report failure
and remaining state instead of reporting a clean restore.

Physical selection uses concrete authorization already supplied in the session
or a fixture record: board/port, exact part/address, bus participants and mux
route, upload permission and allowed observe/volatile/power-cycle actions.
Do not infer authorization from enumerating ports. A mandatory user-authored
JSON file is not required; stage 39 validates concrete resolved inputs. Default
campaigns omit discovery; each candidate must be an authorized pointer-safe
device. Named mux routing is permitted and counted separately from sensor writes.
Observation campaigns retain boot-to-end data-write counters (including attempts
that failed). These are software instrumentation evidence, not independent
proof of wire traffic; a logic trace is a separate evidence layer.

## Tooling and evidence

Stage 37 produces `docs/cli-protocol.md` and golden transcripts under `test/cli/`;
stage 38 completes commands/help and firmware integration, and stage 39 consumes
that same protocol/compiled fixture. Freeze the full grammar in 37 while only
advertising implemented capabilities as operational. All three stages register
their executing tests in the standard host entrypoint; stage 43 runs them.

The protocol distinguishes library version, application firmware source/build
identity, protocol version and per-boot session identity. Application adapters
from 34/35 supply firmware/session metadata without sensor access; 38 wires it
into final images and records them in `docs/firmware-cli-handoff.md`. Unknown
device ID/observations are explicit null/unknown fields, never a reason for an
implicit probe during `version`, `help` or other bus-silent commands.
Stages 34/35 initially provide fixed demos and byte-I/O hooks, without a second
parser. Stage 37 defines the shared processor/host protocol, stage 38 attaches
it. If a backend resumes later, that backend owner integrates the current CLI
and refreshes its build record and firmware handoff.

Define restart/session detection and request-versus-operation terminal records
before the Python runner is written. An accepted `job start` response is not a
completed operation; request IDs, tokens, progress and terminal operation results
have distinct roles. The runner must detect incompatible schema, old boot/session
records and unavailable firmware identity instead of mistaking a matching core
library version for a matching image.

Use strict bounded UTF-8/ASCII line parsing and finite output buffers; no
hidden device access on help. JSONL record or equivalent must include
session/nonce, firmware version and device selected model, reported ID,
command/request ID, operation token where applicable, observation timestamp,
status, side effects and captured raw words. Async progress is not completion;
every accepted request must have a correlated terminal result.
Immutable firmware/model metadata may live in a session header, referenced by
session ID on each record. Mutable observations, request IDs, tokens and status
remain on the relevant records; no undocumented omission of required provenance.
Python must reject stale, out-of-order, mixed-session, truncated and malformed
responses and record exact source/firmware/fixture identity, raw transcripts
and terminal verdicts. No absent board is a passing HIL run.
Count host requests/results and observed ready indications, not unknowable
missed or duplicate physical conversions. Repeated temperatures are valid.
Stage 39's `tools/hil_tmp11x_runner.py` needs pyserial only in physical mode;
host parser/subprocess tests remain standard-library-only. Stage 40/41 evidence
belongs in `docs/hil/<date>-<fixture>/`, or a committed summary/hash inventory
of externally retained raw logs. Ignored local logs alone are not durable evidence.
