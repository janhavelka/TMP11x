# Changelog

## 0.1.0 development

- Added the C++17 library target, ESP-IDF component metadata and native
  compile/link example.
- Added the generated public package version and linked version function.
- Added the host CTest entrypoint with version and register-ledger checks.
- Added portable status, binding, stable settings, model capabilities, and
  identity and sample evidence types with native tests for all six BOM models.
- Added model-specific register metadata and a private single-word I2C
  transport with clipped timeouts, guarded writes and native failure tests.
- Added private family validation for all six BOM models. A complete ID
  mismatch reports the observed word, recognized group and available revision.
- Added bus-silent bind, end and unbind on `TMP11x::TMP11x`. Bound callers
  can read the device ID or probe family compatibility before initialization.
- Added `healthSnapshot()` and `isOnline()` for passive transport inspection.
  Each tracked I2C callback updates saturating counters and records its
  completion time when a clock is available.
- Added signed TEMP and limit conversions and the managed `readLatest` and
  `readTemperature` entry points. A timely complete read updates the sample
  cache. The public calls require verified initialization.
- Added pure CONFIG encoding and decoding, settings validation, and comparison
  of CONFIG and both limit registers. CONFIG observations retain raw MOD, status
  flags and the read-clear mask for the observed comparator mode.
- Added synchronous `begin(config)`, `begin()` and `recover()` with timed startup,
  an operation-local EEPROM lock/busy check and guarded CONFIG and limit
  readback. Rejected bindings preserve the prior target. Failed writes retain
  the desired settings for explicit recovery.
