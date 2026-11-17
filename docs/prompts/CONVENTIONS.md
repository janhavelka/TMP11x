# Selected TMP11x public conventions

Use the real TMP1x2 implementation as the principal precedent, not the
protocol authority. Stage 8 freezes any needed exact signatures in
`docs/api-contracts.md`; shipped public headers then own the live declaration.

| Concern | Selected convention | Peer evidence |
|---|---|---|
| Repository/package | `TMP11x` | `TMP1x2/library.json` |
| C++ namespace, class, header | `TMP11x::TMP11x`, `<TMP11x/TMP11x.h>` | `TMP1x2/include/TMP1x2/TMP1x2.h` |
| Include/src | `include/TMP11x/{Config,Status,CommandTable,TMP11x,Version}.h`, `src/` | TMP1x2 layout |
| Type/function names | PascalCase types, camelCase methods/fields, uppercase scoped enum values, `_private` | TMP1x2 style |
| Status | `Err`, `Status{code,detail,msg}`, `.ok()`, `.is()`, `.inProgress()`, `Status::Ok()`, `Status::Error()`, `errorName()` | TMP1x2/Status.h |
| Lifecycle | bus-silent `bind`, `unbind`, `end`; verified `begin`; explicit `recover` | TMP1x2; distinguish from other sibling `begin` meanings |
| States | `UNINIT`, `READY`, `DEGRADED`, `OFFLINE` passive transport health, independent of initialization | TMP1x2 vocabulary; deliberate state-transition difference, see ENGINEERING |
| Mode and capability | `Model::{TMP116,TMP116N,TMP117,TMP117M,TMP117N,TMP119}`; explicit variant capabilities | TMP1x2 Model style, revised for correct silicon |
| Sample | Raw/counts/Celsius, observation time/validity, fresh flag, acquisition kind, profile generation, trust and offset provenance; exact fields frozen at stage 8 | TMP1x2 Sample, OPT4001 freshness; extended provenance |
| Operations | `OperationKind`, token, `startInitialize`, `startConfigure`, `startRecover`, `startRead`, `poll`, `cancel`, `takeResult` | TMP1x2 jobs; do not duplicate scheduler |
| Tests | CMake/CTest native independent register device model; PlatformIO native; Arduino & native ESP-IDF link gates | TMP1x2 tests and examples |
| Packaging | `library.json` version authority; generated checked-in Version.h; standalone CMake and ESP-IDF component | TMP1x2, OPT4001 |
| CLI | one framework-neutral parser, thin Arduino/ESP-IDF transport adapters; bounded output | TMP1x2 examples/common |

## Architecture guidance

- Use one main class and a small **verified** model capability table; no
  unnecessary inheritance hierarchy, external shared-core dependency, code
  generator or speculative generic temperature framework.
- `Config` owns desired settings, selected BOM model, seven-bit address
  (0x48–0x4B), callbacks, timeout, clock required by deadline-bearing operations, optional
  ALERT GPIO callback. Core never configures pins or I2C.
- Offset management is opt-in and off by default. Unmanaged TMP117/TMP119
  offset is observed, not replayed; TMP116 rejects managed offset before I2C.
- A synchronous helper may use an explicitly bounded application yield hook
  only if stage 8 defines it; cooperative `poll` always returns during waits.
  The required clock hook governs deadlines/ages; legacy `nowMs` arguments
  do not override it. Stage 9 freezes per-API clock requirements.
- A per-instance user callback context and selected I2C address may coexist
  with other TMP11x/TMP1x2 instances on a shared bus. Bus route selection,
  mutexes and TCA9548A channel selection belong to application owner.
- Synchronous getters vs owner operations must have precise semantics;
  reading TEMP never by itself guarantees a fresh conversion.
- Stable desired mode is continuous or shutdown. One-shot/reset are explicit
  commands and cannot be replayed by initialization or recovery. Bind-only
  diagnostics can observe permitted registers without managed initialization;
  report unknown startup/configuration/sample trust instead of writing settings.
- Managed one-shot first checks current stable CONFIG before mutation, then
  requires desired shutdown and a guarded shutdown write
  followed by a cleared readiness baseline before triggering;
  a continuous desired mode rejects before I2C. The caller explicitly selects
  shutdown first. Desired shutdown remains unchanged across the owned
  one-shot and its completion; undocumented MOD readback is not completion
  evidence. Cancellation does not restore a prior mode.
- Acquisition kinds are explicit: latest register read, continuous try-fresh
  (and its bounded wait), or owned one-shot. No mode-dependent read silently
  triggers a conversion. Dirty/uninitialized profiles reject fresh requests;
  latest diagnostics can return untrusted observations.
- Binding fields are immutable during settings/owner operations. Changing
  model/address/callbacks/contexts/clock epoch requires explicit rebind; INITIALIZE,
  CONFIGURE and RECOVER jobs reuse the established binding.
- `Err::OK` / `Status::Ok()` report a successful **call**, not a new conversion.
  Return `MEASUREMENT_NOT_READY` or explicit `freshConversion=false` as
  contract requires. `I2C_NACK_ADDR` only when transport knows the phase.
- Health counts actual tracked physical transport attempts, not failed
  preconditions/unsupported feature/late logical operation deadline.
- When nonvolatile lock uncertainty exists, mark `EEPROM_UNSAFE` (or an
  equivalently precise error) and refuse all mutating register writes.
- External reset and diagnostic reads can alter knowledge. Reset invalidates
  configuration and freshness; status reads consume/record flag evidence but
  do not automatically mean that persistent configuration became dirty.
- CONFIG-read alert evidence survives later zero reads and composite failure;
  stage 19 retains it and stage 22 exposes explicit bus-silent consumption.
  Active jobs and retained results block unrelated I2C calls, including probes.
- Reserved fields and command-only bits are not ordinary settings to preserve
  or replay. CONFIG bit 1 is a reset command only on TMP117/TMP119; use a
  dedicated guarded operation. MOD=10 is a legal continuous alias, not reserved.
- Public headers Doxygen describe units, read effects, deadlines, validity,
  model applicability and output-argument behavior on failure.

## Intentional exclusions

Do not add TMP1x2's 12/13-bit extended-mode codec, its conversion-rate enum,
TMP112D address model, pointer persistence assumptions or general-call bus
helpers. Do not silently accept TMP114/TMP118 based on a shared 0x48 address
or equal numeric temperature LSB. Do not add EEPROM writes to satisfy a
feature-coverage percentage; record them as an intentional safety boundary.
