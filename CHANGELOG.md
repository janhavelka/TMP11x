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
- Added read-only CONFIG observation, whole-profile verification and a
  memory-only settings snapshot with per-register evidence and dirty causes.
  Applications can report external changes through `invalidateDeviceState()`
  and recover the desired profile under restored ownership.
- Added guarded mode, cycle and averaging setters and a bus-silent timing
  query for continuous cadence, standby, active block estimates and
  conversion-window budgets on all six model selections.
- Added continuous try-fresh and blocking reads and an explicit blocking
  one-shot from verified shutdown. Fresh samples require a guarded DATA_READY
  observation for the current profile and a timely combined TEMP read.
- Added guarded limit and alert setters, signed limit readback, and optional
  physical ALERT sampling. Applications can inspect and consume captured
  comparator events with their original source, separately from therm HIGH.
- Added signed hardware-offset readback on TMP117/M/N and TMP119 during
  guarded settings replay, explicit verification and typed reads. An
  unexplained change to unmanaged calibration invalidates initialization and pending readiness.
- Added opt-in volatile offset management through `managedOffsetCounts` and
  `setTemperatureOffset(celsius)`. Guarded replay writes and verifies the
  signed target, including zero. TMP116/N rejects management before I2C.
- Fresh samples carry observed unmanaged or verified managed offset counts.
  Latest reads on offset-capable parts leave attribution unknown. All TMP116/N
  samples report the hardware offset as unavailable.
- Added bound-only reads of individual EEPROM data shadows and an ID-first
  snapshot of lock, BUSY and the model's three or four shadow words.
  A failed scan reports the completed prefix and first unsuccessful pointer.
- Added guarded addressed reset on TMP117/M/N and TMP119 with timed identity,
  EEPROM guard and profile readback. Explicit recovery replays owned settings
  and preserves unmanaged calibration. TMP116/N refuses reset before I²C.
- Added checked single-register reads and an ID-first, ten-word register dump
  after binding. Results carry model-specific names, raw values, optional
  signed counts and read effects. A failed dump retains completed words and
  identifies flags the unsuccessful transfer may have acknowledged.
