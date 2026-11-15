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
| `read` | Read the latest TEMP register through the driver; clears DATA_READY and does not by itself establish freshness. |
| `read --fresh` | Bounded continuous-mode try-fresh acquisition using owned hardware evidence; no conversion trigger or mode change. Return not-ready when unproven. |
| `identity`, `config`, `status`, `dump`, `eeprom` | Explicit register reads. Report CONFIG/TEMP flag consumption; dumps are sequential observations, not atomic snapshots. EEPROM reads observe shadows. |
| `discover` | Read identity only at application-authorized candidates within 0x48–0x4B. Those addresses may contain unrelated devices; require fixture/bus ownership, never run discovery implicitly. Do not infer TMP119 or grade from revision bits. |
| `mode`, `cycle`, `avg`, `limits`, `alert`, `offset` | Explicit persistent volatile-setting changes, subject to model and EEPROM safety gates. `mode` accepts continuous/shutdown; one-shot is a separate operation. `offset` exists only for TMP117/TMP119. No automatic restoration that would undo an intentional setter. |
| `oneshot`, `recover`, `shutdown`, `reset` | Explicit device mutation through typed guarded operations. `reset` is TMP117/TMP119 only; no general-call fallback for TMP116. Report resulting mode/configuration trust. |
| `invalidate`, `job cancel`, `job result` | Bus-silent state/result actions under the owner/result contract. Cancellation cannot undo an earlier hardware effect. |
| `job start`, `job step` | Use the selected operation's declared effects; read, continuous try-fresh and one-shot are distinct requests. No low-level I2C bypass. |
| `watch` | Finite register-read acquisition by default. One-shot or configuration changes require an explicit option/campaign with declared effects. |

Register-read-only means no data-register writes, not no side effects: pointer
writes select the register and CONFIG/TEMP reads can consume flags. Bind a
read-only session without `begin`/recovery/configuration writes, including at
firmware boot. Never program EEPROM, mutate its shadows, or broadcast reset.
Use the bound-only diagnostic route when managed initialization is absent;
return unknown startup/configuration/freshness rather than auto-initializing.
Continuous `read --fresh` requires the known mode/evidence preconditions from
the API contract; observation-only sessions may refuse it while allowing raw
`read`, `config` and `identity`.

Temporary test campaigns snapshot restorable settings and verify restoration
or report failure. Restore only writable stable fields, with reset commands
masked and mode normalization handled; flags, conversion phase, consumed
readiness and prior temperature cannot be restored. Do not replay a captured
one-shot command or raw CONFIG word. The user must select safe fixture
conditions for mutating campaigns; no such campaign runs on connection alone.

## Tooling and evidence

Use strict bounded UTF-8/ASCII line parsing and finite output buffers; no
hidden device access on help. JSONL record or equivalent must include
session/nonce, firmware version and device selected model, reported ID,
command/request ID, operation token where applicable, observation timestamp,
status, side effects and captured raw words. Async progress is not completion;
every accepted request must have a correlated terminal result.
Python must reject stale, out-of-order, mixed-session, truncated and malformed
responses and record exact source/firmware/fixture identity, raw transcripts
and terminal verdicts. No absent board is a passing HIL run.
Count host requests/results and observed ready indications, not unknowable
missed or duplicate physical conversions. Repeated temperatures are valid.
