# CLI and HIL contract

The diagnostic processor is example-only and framework-neutral, shared between
Arduino and native ESP-IDF. Adapt TMP1x2's bounded line buffer and result
correlation; do not introduce an independent direct-I2C path that bypasses
managed driver state. CLI and Python runners are not dependencies of the core.

## Read-only commands

`help`, `identity`, `model`, `addr`, `health`, `settings`, `read`, `latest`,
`config`, `dump`, `eeprom` (read-only), `status`, `jobs`, `version`, `bus`.
Every command reports whether it reads CONFIG or TEMP and may clear a flag.
`discover` visits only valid candidate addresses with read-only identity
queries; it must not silently assume TMP119 from a revision nibble.

## Volatile mutating commands (explicit)

`mode`, `cycle`, `avg`, `limits`, `alert`, `offset` (only TMP117/TMP119),
`oneshot`, `recover`, `invalidate`, `shutdown`, finite `watch`, `job start/step/
cancel/result`. Reject commands that can program EEPROM or broadcast reset.
A command requiring settings mutation snapshots state first and verifies
restoration or records failure. Do not run such commands automatically when
attached to a production board without the owner's chosen safe conditions.

## Tooling and evidence

Use strict bounded UTF-8/ASCII line parsing and finite output buffers; no
hidden device access on help. JSONL record or equivalent must include
session/nonce, firmware version and device selected model, reported ID,
command/request ID, operation token, timestamp, status and captured raw words.
Python must reject stale, out-of-order, mixed-session, truncated and malformed
responses and record exact source/firmware/fixture identity, raw transcripts
and terminal verdicts. No absent board is a passing HIL run.
