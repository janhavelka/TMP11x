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
