# Full feature/variant ledger and deliberate first-release limits

This is the mandatory **requirements inventory**, not implementation evidence.
Stage 5 creates `docs/feature-coverage.md` and the checked register ledger from
these rows and the original PDFs. Preserve the row IDs through implementation,
tests, CLI and final review. Split rows where fields/models need different
evidence; matching addresses do not imply matching access/reset/side effects.
In `docs/feature-coverage.md`, put each preserved ID in the first column of a
Markdown table. A split row may use a suffix such as `R01.tmp116` or
`R01-tmp117`; every base R/F/X ID must remain represented. Prose mentions alone
do not satisfy the inventory check. This is structural preservation, not proof
of per-model implementation or test coverage.

Source audits: [TMP116/N](../reference/extracted/tmp116-coverage-audit.md),
[TMP117/M/N](../reference/extracted/tmp117-coverage-audit.md),
[TMP119](../reference/extracted/tmp119-coverage-audit.md), and
[excluded TMP114/TMP118](../reference/extracted/excluded-families-audit.md).
They cover the pinned datasheets' register, functional, interface, electrical,
application and orderable sections. They supplement the original PDFs and
[SILICON](SILICON.md); stages 4–7 still own the final variant, register,
timing and safety decisions.

## Models and complete register coverage

The BOM selects TMP116, TMP116N, TMP117, TMP117M, TMP117N or TMP119. Models
share register capabilities where the exact-part maps agree. A register read
checks compatibility. The BOM records the accuracy grade. The six-model
scope excludes TMP119M because the pinned addendum gives no TMP119M ordering
code. Every row requires per-model applicability, including explicit refusal
of absent features. See the [variant contract](../variants.md).

| ID | Hardware function | Planned owners | Required closure evidence |
|---|---|---|---|
| R01 | TEMP 0x00, signed16, 1/128 °C, initial 0x8000 | 5–6, 16, 21, 32–33 | Independent signed vectors, endpoints vs rated range, initial validity vs confirmed conversion, DATA_READY read effect |
| R02 | CONFIG 0x01 HIGH/LOW flags [15:14] | 5–6, 17, 22, 32 | Alert latch/acknowledge/re-arm vs therm hysteresis; equality; TEMP versus CONFIG reads |
| R03 | CONFIG DATA_READY [13] | 5–6, 21, 26, 30, 32 | Complete averaged conversion, consumed evidence, diagnostics/failed reads; not a sample counter |
| R04 | CONFIG EEPROM_Busy [12] and 0x04 busy [14] | 5–7, 18–19, 21, 23, 25, 29–30, 32 | Mirror semantics; guard reads 0x04; startup conflict, bounded observation, no write/fresh claim on unsafe state |
| R05 | CONFIG MOD [11:10] | 5–6, 17, 20–21, 30 | MOD=10 aliases continuous and reads back 00; one-shot returns physically to shutdown but MOD readback is unknown; explicit SD abort boundary; MOD1 not EEPROM-stored |
| R06 | CONFIG CONV [9:7] | 5–6, 17, 20, 32 | All eight encodings × four AVG settings, continuous total period vs active/standby; no OS duration effect |
| R07 | CONFIG AVG [6:5] | 5–6, 17, 20–21, 32 | Counts 1/8/32/64; readiness after full average; model-specific timing bounds |
| R08 | CONFIG T/nA [4] | 5–6, 17, 22, 32 | Alert latch vs therm hysteresis/LOW=0; transitions with old status explicitly assessed |
| R09 | CONFIG POL [3] | 5–6, 17, 22, 34–35, 41 | Logical active-high/low vs physical open-drain pin/pull-up, no push-pull assumption |
| R10 | CONFIG DR/Alert [2] | 5–6, 17, 22, 32, 41 | DATA_READY vs threshold output; all mode/polarity/routing combinations |
| R11 | CONFIG [1:0] per-model meaning | 5, 7, 17, 25–26, 32 | TMP116 reserved; TMP117/119 bit 1 soft reset always reads 0; ordinary writes never replay commands; unresolved addressed-reset effects |
| R12 | THIGH 0x02, TLOW 0x03, signed16 each | 5, 16–19, 22, 29, 32 | Negative/end-point vectors, register order, pair-policy vs silicon encoding, full-profile replay/readback |
| R13 | EEPROM_UL 0x04 EUN[15], busy[14], reserved[13:0] | 5, 7, 18, 24, 26, 32 | Read all fields; deny every write including relock; external unlock stops mutation |
| R14 | TMP116/N EEPROM1/2/3/4 at 0x05/06/07/08 | 5, 7, 24, 26, 32 | Four shadow words, index bounds, 0x07 never offset, reset-content conflicts |
| R15 | TMP117/M/N and TMP119 EEPROM1/2/3 at 0x05/06/08 | 5, 7, 24, 26, 32 | Three explicit pointers, no burst over 0x07 or fourth word; arbitrary contents accepted |
| R16 | TMP117/M/N and TMP119 offset 0x07 | 5, 7–8, 16, 23, 25–26, 29, 32, 38, 41 | EEPROM-backed reset domain; opt-in management, preserve unmanaged calibration; signed bounds, saturation, shared guard/replay, TMP116 zero-I2C refusal |
| R17 | DEVICE_ID 0x0F | 4–5, 13, 26, 32 | TMP116 full 16-bit 0x1116 versus 117-family low-12-bit DID plus revision nibble; no exact-part/grade inference; identity-first diagnostic dumps |
| R18 | Reserved pointers 0x09–0x0E and invalid upper bits | 5, 12, 26, 32–33 | Reject without I2C; never mask invalid input into valid pointer or dump through holes |

Reset/default entries distinguish factory value, EEPROM-loaded value, unknown
factory data, status and command behavior. Record field-level NVM eligibility:
CONFIG is not uniformly persistent; MOD1 is not stored. Include THIGH/TLOW/
OFFSET factory values and programmable reset domains without assuming a used
device retains factory settings. Separate hardware access from release policy.

## Behavior outside the register table

| ID | Behavior/integration requirement | Planned owners | Required closure evidence |
|---|---|---|---|
| F01 | BOM model/grade/package and identity confidence | 4, 8, 13, 32, 40, 42 | Orderable, accuracy/operating-range conditions/conflicts; no grade auto-detection |
| F02 | ADD0 GND/V+/SDA/SCL →0x48/49/4A/4B | 4–5, 12–13, 34–36, 40 | Board strap, 7-bit address, collisions; no core strap control or broad scan |
| F03 | Pointer, MSB-first word and retained pointer | 5, 12, 24, 26, 34–35 | Upper pointer nibble 0, explicit pointer per access, repeated START, ACK first/NACK last read byte; no assumed auto-increment |
| F04 | I2C timing and sensor interface timeout | 5, 9, 12, 34–36, 40, 42 | 1–400 kHz, sensor never drives SCL, 20–40 ms timeout (35 ms typical); STOP-reset/ignore-until-START explicit for TMP117/119, not TMP116; separate callback/operation deadlines and actual bus load |
| F05 | POR/EEPROM load and initial sample invalidity | 6–9, 14, 18–19, 21, 25, 29–32, 41 | Owner power stability/internal FIRST_ACK origin; lifecycle/reset invalidation, ignored writes per model/register, TMP116 busy conflict, conversion evidence; approximate 1.5 ms is not a maximum |
| F06 | Continuous/shutdown/owned-one-shot transitions | 6, 20–21, 30, 32 | Abort/physical return versus unknown MOD readback; baseline and one trigger; all 32 continuous CONV×AVG pairs, four one-shot AVG durations plus CONV independence |
| F07 | Configuration writes during conversion | 6, 17–23, 29–32 | Documented transitions vs unknown same-value/AVG/CONV-write behavior; conservative profile invalidation, no invented restart rule |
| F08 | TEMP acquisition during update | 6, 12, 21, 30, 32, 41 | Two-byte transfer; datasheets do not explicitly specify byte latching. Record continuous coherence assumption/evidence gate; assess stable owned-one-shot result separately |
| F09 | Alert latch/re-arm, therm hysteresis and GPIO | 6, 19, 22, 32, 41 | Sticky driver evidence survives CONFIG acknowledgement; therm HIGH remains a level; equality has source/native proof, optional HIL; routing/polarity and pin vs status |
| F10 | Offset and limit comparator path | 6, 22–23, 32, 41 | TEMP saturation documented; exact offset/comparator ordering requires evidence; do not encode an unsupported comparator assumption |
| F11 | Complete desired/observed/trusted profile | 8, 18–19, 22–23, 27, 29–31 | Stable CONFIG plus limits and opt-in managed offset; unmanaged offset observed only; readback, partial failure/cancel and full owned-field replay; no unsupported 0x07 access |
| F12 | Reset domains and outside access | 6–7, 19, 25, 31–32 | Individual reset where supported; outside power/reset/write/ARA invalidates affected assumptions; no automatic recovery/reset |
| F13 | Shadow data and NIST provenance limits | 4–7, 24, 26, 40, 42 | Shadow vs NVM/factory distinction, reset conflicts, word/byte order; no zero/nonzero serial validity rule or certificate claim |
| F14 | API/jobs/CLI coverage | 8–9, 27–30, 37–39, 44 | Every included row maps to typed API, observation and appropriate command/job or justified not-applicable; no placeholder success |
| F15 | Destructive reads and snapshots | 6, 19, 21–22, 24, 26–27, 30, 38–40 | Read order, non-atomicity, consumed flags, bound-only unknown trust; failed reads may consume status |
| F16 | Electrical and thermal integration | 4, 34–36, 40–42 | Voltage vs temperature/grade, logic levels, open-drain pulls, decoupling, leakage/traffic, thermal response/strain/self-heating, package-specific layout |
| F17 | Accuracy conditions vs resolution/code range | 4, 16, 20, 23, 40–42 | Averaging/cadence/supply/test and assembly conditions; no guarantee from typical curves, resolution or an offset setting |
| F18 | Health/deadlines/cancellation/ownership | 8–9, 14–15, 27–31, 33 | Passive OFFLINE, post-callback clock, finite work, exclusive sensor authority across dependent transfers, exactly-once results |
| F19 | Variant and incompatible-neighbor tests | 13, 32–33 | All six selected models, named TMP114/TMP118 negative fixtures; ACK/unrelated pointer data never unique part proof |
| F20 | Independent consumers and evidence layers | 10, 33–36, 40–44 | SDK-free C++17 core, linked S2/S3/P4, installed package, exact CI SHA, per-fixture HIL; missing evidence never PASS |

### Prompt 4 source and documentation evidence

The links cover the documented layer for the six selected BOM models.
Implementation, adapter and physical evidence remain open under the owners
listed above.

- **F01** records the exact grade, packages, supply and orderable limits in
  the [BOM matrix](../board-integration.md#orderable-packages-and-conditions).
  The [identity decision](../variants.md#identity-decision) also supplies the
  source rule and example words for R17.
- **F02** records the [ADD0 straps and seven-bit addresses](../board-integration.md#pins-and-add0-straps).
  The [neighbor inventory](../variants.md#excluded-and-shared-bus-devices)
  supplies the shared-bus exclusions for X07.
- **F13** records the per-model [EEPROM shadow map and provenance limits](../variants.md#eeprom-shadow-and-traceability).
  The same evidence documents the factory-identity limit for X06.
- **F16** records [I/O limits, timing, pullups and decoupling](../board-integration.md#pullups-logic-levels-and-decoupling)
  and [package-specific thermal layout](../board-integration.md#thermal-layout-and-precision).
- **F17** records [guaranteed accuracy and its conditions](../board-integration.md#guaranteed-accuracy),
  including averaging, cycle, assembly, code range and startup sentinel.

## Excluded or application-owned hardware functions

An exhaustive audit includes these rows; it does not silently expand release
scope. Unsupported operations must not become successful no-ops or raw/CLI
escape paths. Record silicon capability and release disposition separately.

| ID | Function | Planned owners | Disposition and evidence |
|---|---|---|---|
| X01 | EEPROM unlock/relock, shadow writes, programming, persistent startup | 7, 24, 26, 33, 38 | Excluded, including sacrificial fixtures; prove no forbidden writes. Endurance/program timing are source facts, not implemented API. |
| X02 | General-call reset and broadcast-synchronized conversions | 7, 18, 25, 33, 38 | Excluded bus-wide commands; prove no address 0x00 traffic. Outside reset invalidates state; distinguish its domain from addressed reset. |
| X03 | SMBus ARA, arbitration and acknowledgement | 7, 22, 32–33, 35, 38 | Hardware capability, no ARA API/traffic; document external effects. CONFIG is addressed acknowledgement; stage 35 creates no ARA handle. |
| X04 | Public raw data writes, bare current-pointer reads and multi-register bursts | 8, 12, 24, 26 | Typed guarded mutations only; reads supply an explicit pointer and one word. No shared pointer history or undocumented auto-increment. |
| X05 | Bus rate/pulls/pins/mux/GPIO interrupts/recovery/retries | 9, 34–36, 40 | Application-owned integration and sensor timing constraints, no core bus ownership. Authorized mux routing is recorded separately from sensor writes. |
| X06 | Authenticated factory identity/certificate or calibrated system accuracy | 4, 24, 40–42 | No driver claim; explain provenance and fixture limits. |
| X07 | TMP114/N, TMP118, TMP102/112/1075/110, speculative TMP119M | 4, 13, 32 | Excluded; record incompatibilities, no data writes/protocol probing to identify excluded parts; identity pointer access only on authorized compatible fixtures. |
| X08 | Undocumented CRC/PEC/fault queue/extended mode/sample counter | 4–6, 8, 22, 33 | Do not import features from other sensors or SMBus terminology; each owner records unsupported capabilities at its ledger/API/test layer. |

## Completion accounting

For **every row and applicable model**, record source revision/page, hardware
capability, release disposition, owner stage, implementation path, registered
test, CLI command (or justified not applicable), real board/grade and remaining
evidence. Keep `documented`, `implemented`, `host-tested`, `MCU-linked`,
`remote-CI` and `physically-qualified` separate. These six names are canonical
throughout the promptset. Thermal integration closes
with a reviewed guide/fixture record, not a fake unit test of physics.
A source contradiction requires evidence or a bounded explicitly unresolved
claim. A simulated assumption is never primary evidence.

Stages 33/44 reconcile **all R/F/X rows** and finer stage 5 rows. Every included
software feature needs executing tests where observable; denied mutations need
zero-transfer/forbidden-pattern assertions. One model's physical run cannot
close another model or grade's HIL row.
