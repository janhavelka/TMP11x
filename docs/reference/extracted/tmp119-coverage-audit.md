# TMP119 datasheet coverage audit

Audit date: 2026-11-15. Baseline: promptset commit `8d2ffe3`.
Source: pinned [TMP119 SNIS236, January 2024](../source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf),
manifest ID `tmp119`, SHA-256
`7eabaeda3a51298768e188bfefafbdb8e8ae1f20200019c5fbb654d977adaa30`.
Page numbers below are one-based PDF pages; body pages match printed pages.
All 48 pages were inspected as text, with original table images checked for
pp.7,35,38. This is an original factual coverage inventory, not a replacement
for TI's document. No driver exists yet; all implementation/test references
below are future stage obligations, not passing implementation evidence.

## Complete register and field inventory

Every documented pointer is included. Unlisted pointers, including 0x09–0x0E,
have no supported meaning here. Read/write capability is silicon behavior;
the project deliberately permits fewer writes.

| Pointer / field | Source and behavior | Project disposition / stages |
| --- | --- | --- |
| 0x00 TEMP[15:0] | pp.15,33: signed 16-bit counts, 1/128°C; reset 0x8000 until completed conversion including averaging. Reading clears DATA_READY. | Latest/raw/fresh observations have different validity, 16,21,26,30. No writes. |
| 0x01 CONFIG[15] HIGH, [14] LOW | pp.18–19,34: high/low comparator flags; CONFIG read acknowledges both in alert mode. Therm HIGH retains hysteresis state and LOW is zero. TEMP does not clear comparator flags. | Typed status/read effects, 19,22,26. |
| CONFIG[13] DATA_READY | pp.16–17,34: set on completed result update, cleared by CONFIG or TEMP read. No sample counter. | Evidence belongs to one observation/profile, 21,27,30. |
| CONFIG[12] EEPROM_Busy | pp.20,34,37: programming/reset-load busy, mirrored by 0x04[14]. Not EUN. | Read-only preflight, 7,18,19,24–25. |
| CONFIG[11:10] MOD | pp.16–17,34–35: 00/10 continuous, 10 reads 00; 01 shutdown aborts conversion; 11 one-shot returns to shutdown. Bit 11 cannot persist in EEPROM. | Stable CC/SD versus explicit OS command, 17,20–21; EEPROM persistence excluded. |
| CONFIG[9:7] CONV | p.35 Table 8-6: eight cycle selectors; effective period depends on AVG, not an independent rate. | All 32 CONV/AVG combinations, 6,17,20. |
| CONFIG[6:5] AVG | pp.14,34: 00/01/10/11 select 1/8/32/64 conversions; accumulated average, not moving average. | Typed settings/timing, 17,20–21. |
| CONFIG[4] T/nA | pp.18–19,34: 0 alert window, 1 therm hysteresis. | Typed mode, 22. |
| CONFIG[3] POL | p.34: 0 active-low, 1 active-high ALERT. | Typed pin polarity, 22; GPIO belongs to application. |
| CONFIG[2] DR/Alert | p.34: 1 routes DATA_READY to ALERT, 0 comparator alert flags. | Typed output selection and distinct register/pin observations, 22. |
| CONFIG[1] Soft_Reset | p.35 detailed table: addressed software-reset command, 2-ms reset, reads zero. p.34 figure incorrectly labels it read-only. | Explicit guarded command only, 7,25; never ordinary settings replay. |
| CONFIG[0] unused | p.35: reads zero. | Zero in writes; no raw escape, 17,26. |
| 0x02 THIGH, 0x03 TLOW [15:0] | p.36: signed 1/128°C limits; factory 0x6000/0x8000, EEPROM-loaded on reset. | Guarded volatile limits/readback, 18,22. Representable range is distinct from rated temperature. |
| 0x04[15] EUN, [14] BUSY, [13:0] unused | p.37: EUN controls whether writes reach NVM; busy mirrors CONFIG; lower bits read zero. | Reads only, including lock/busy diagnostics; every write denied, 7,18,24,26. |
| 0x05 EEPROM1, 0x06 EEPROM2, 0x08 EEPROM3 [15:0] | pp.32,37–39: EEPROM-backed scratch/traceability shadows. Locked writes affect shadows; unlocked writes program NVM. | Checked three-word read-only map, 24. No shadow or NVM writes, no asserted immutable serial identity. |
| 0x07 Temp_Offset[15:0] | pp.32,38: signed 1/128°C offset after linearization; result sum saturates at signed endpoints; reset value EEPROM-backed, factory zero. | Guarded typed volatile offset only, 23. Reject invalid requested offset; never apply it twice in software. |
| 0x0F[15:12] Rev, [11:0] DID | p.39 Table 8-14: revision plus DID=0x117; detailed reset 0x2117 conflicts with map p.32 0x0117. | BOM model controls capabilities; compatible ID cannot prove TMP119 versus TMP117 or grade, 4,13,32. |

CONFIG masks derived from the field table: read-only status `0xF000`,
writable mode/settings `0x0FFC`, explicit reset `0x0002`, unused `0x0001`.
Only bits 10:2 (`0x07FC`) have EEPROM-storage footnote (1); MOD1[11] does
not. This is a persistence inventory, not authorization to program EEPROM.

## Non-register functionality and boundaries

| Datasheet coverage | Required disposition and evidence |
| --- | --- |
| pp.1–7: model, pins, ratings, accuracy | BOM/package metadata and application guide: 6-ball YBG, ADD0, open-drain SDA/ALERT; 1.7–5.5 V only through 70°C, 1.8–5.5 V over −55..150°C. Absolute maximums are not operating limits. Accuracy conditions include 8 averages, 1-Hz cycle and inputs within 5% of rails. Stage 4/42; physical accuracy is a separate HIL qualification. |
| pp.7–12: electrical/timing/typical curves | Preserve units and MIN/TYP/MAX. Single conversion is 13/15.5/17.5 ms without averaging. Drift, repeatability, hysteresis, response time, thermal/current curves are conditional performance facts, not driver-correctable guarantees. Stage 6/20/42. |
| pp.14–17,35: power-up, averaging, modes | POR about 1.5 ms and result invalidity precede conversion; averaged active durations derive from count; nominal cycle table includes active time. Shutdown aborts; OS omits standby and depends on AVG only. CC and shutdown are EEPROM startup modes, OS is not. Stages 6,18,20–21,32. |
| pp.18–19: alert/therm comparison | Strict `>` high and `<` low; equality retains therm state. Compare completed averaged result. Polarity, selected pin source and observed flags are separate. Stages 22,32. |
| pp.20–21: EEPROM programming/loading | Silicon supports unlocking, programming, changed startup defaults and reload; programming conditions/endurance/retention do not create a library programming API. First-release read-only policy is stronger than silicon permissions. Startup writes to CONFIG/limits are ignored; reads remain possible. Stages 7,18,24–25,32. |
| pp.22–26: addressing and word protocol | ADD0 GND/V+/SDA/SCL selects 0x48/49/4A/4B. Pointer resets to zero and is retained; upper four pointer bits must be zero. Word data is MSB first. Core always explicitly selects pointer and reads exactly two bytes in one combined transaction. Controller ACKs MSB, NACKs LSB, then STOP. No auto-increment or CRC extension is specified for this core. Stage 12 and adapters 34–36. |
| pp.8–9,22–24: bus speed and timeout | Supported SCL 1–400 kHz; device never drives SCL. STOP resets interface state, START is needed to resume. Serial timeout is 20–40 ms, 35 ms typical, for documented held-low conditions; it resets the interface, not sensor configuration. Distinguish it from callback timeout and conversion/job deadline. Stages 6,12,31,34–36,42. |
| pp.18–19,24,26: SMBus alert response | Hardware supports ARA at seven-bit address 0x0C (read byte 0x19); response includes address and threshold-direction bit, lowest address wins arbitration. Winner deasserts ALERT in alert mode; therm does not clear through ARA. First-release ARA traffic remains explicitly excluded. External ARA can change pin observation independently of CONFIG acknowledgement. Stages 7,22,32. |
| pp.24,26,29: general-call reset and synchronized sampling | Hardware supports broadcast reset and a synchronization example involving programmed startup settings. Both broadcast reset and that synchronization recipe are excluded; independent sequential one-shots are not simultaneous sampling. Stages 7,25,34–36,42. |
| pp.15–16,23,27–31: application precision and layout | Strain tolerance has no register switch. Self-heating, shared-bus traffic, pullups, decoupling, PCB thermal paths and mounting govern system accuracy/response. Avoid I2C during active conversion for best precision; use bounded sparse polling, not aggressive status loops as an accuracy claim. Stages 6/21/30 own poll policy; 34–36/40–42 own physical integration/evidence. 100 nF bypass and application-specific pullups are board responsibilities. Surface, moving-air and still-air layouts serve different goals. |
| pp.40–48: support, package, notices | Source revision/support provenance and package/orderable/assembly metadata belong in reference/BOM docs. They introduce no core API. Preserve vendor terms and exclude PDFs from runtime packages. Stages 3,4,42. |

## Remaining discrepancies and TMP119M scope

The earlier audit already captured ID/reset-bit, EEPROM word-count and
one-shot ALERT prose contradictions. Retain them. Additional contradictions
need explicit entries rather than guessed defaults:

- p.4 comparison accuracy differs from exact p.7 electrical limits; for
  example −40°C is 0.11°C in the comparison but 0.12°C in §6.5, and 85°C
  is 0.15°C versus the −25..85°C range's 0.09°C. Use exact §6.5 with its
  conditions for TMP119 metadata; do not copy cross-family comparison cells.
- The same p.4 summary gives TMP116/N supply 3.0–3.6 V versus SBOS740A p.4's
  1.9–5.5 V, and TMP119 shutdown 0.25 µA versus p.8's 0.15 µA typical.
  Exact-part electrical conditions govern every model, not just accuracy.
- Shutdown prose p.17 says 250 nA; p.8 specifies 150 nA typical and 350 nA
  maximum at 25°C with stated input conditions. Preserve the difference.
- A 1-MHz current-characterization curve on p.11 does not override the
  400-kHz protocol limit on p.22.
- EEPROM2 p.38 heading says reset 0x0000, while its figure/table and p.32
  show unknown contents. Do not assume zero or synthesize factory UID data.
- p.34 Table 8-5 labels CONV as standby-related while p.35's cycle table and pp.16–17
  describe active plus standby. Helpers must use the cycle interpretation.
- The p.20 stored-field list omits DR/Alert and OFFSET, while pp.32/34–35
  mark them EEPROM-backed. Four-word EEPROM references also occur in p.22
  Figure 7-9 and p.37; the actual three-word map plus offset remains decisive.
- Page 35 defines the addressed reset bit/duration, but does not explicitly
  establish its EEPROM reload, EUN effect, companion-bit application or bus
  availability. Page 20's explicit general-call relock is not proof of these
  addressed-reset outcomes. Observe them after a conservative wait.
- OS physically returns to SD (p.17), but MOD readback during/after OS and
  retrigger during active OS are not specified. Therm HIGH changes at completed
  conversions (p.19); only DATA_READY/alert-mode SD-entry flags remain unknown
  under an otherwise unchanged therm/routing/polarity/reset state.

p.7 has TMP119M-specific accuracy rows, but p.45's pinned orderable addendum
lists TMP119AIYBGR and TMP119AIYBGR.A only. TI's current
[TMP119AIYBGR detail page](https://www.ti.com/product/TMP119/part-details/TMP119AIYBGR),
checked 2026-11-15, identifies `.A` as the same part and supplies no TMP119M
mapping. This does not prove TMP119M never exists; it leaves its orderable
scope unresolved. Keep `Model::TMP119`, record M rows as unresolved evidence,
and require authoritative BOM/orderable mapping before adding an M model.

## Actionable prompt gaps relative to the audit baseline

| Priority / gap | Prompt owners and required verification |
| --- | --- |
| High: complete serial electrical/protocol contract absent | 5–6,12,34–36,40,42: explicit pointer high-nibble/word shape, address straps, bus range, ACK/NACK/STOP, hardware timeout distinction. Native tests reject holes/high-bit pointers and always send pointer; adapter/HIL traces prove actual waveform. |
| High: source conflicts can contaminate model defaults | 4–6,24,32: add exact-spec precedence and named discrepancies above; fixture EEPROM2 is arbitrary, model metadata does not claim M scope or 1-MHz operation. |
| Medium: application conditions omitted from full coverage | 4,34–36,40–42: supply/temperature envelope, electrical input conditions, open-drain/pullups, decoupling, sampling/bus/thermal tradeoffs and strain/package facts; keep HIL protocol success distinct from calibrated accuracy evidence. |
| Medium: persistence/averaging details need explicit fixtures | 5–7,17,20,32: MOD1 is not stored; AVG is accumulated, not moving; all 32 cycle combinations plus raw saturation endpoints after a confirmed offset conversion. Existing broad combination tests remain necessary. |
| Medium: exclusions are not complete coverage rows | 7,22,25,32,42/FULL_COVERAGE: list ARA, EEPROM scratch/persistence, broadcast synchronized sampling as intentional exclusions; show external ARA pin change without assuming CONFIG flags cleared. No new bus-wide command/API. |

No implementation, firmware build, remote CI, or physical HIL was performed
by this reference audit. It supplies stage inputs only.

## Response — 2026-11-17

Rechecked each actionable row and retained discrepancy against `171dbf5`,
current prompts and pinned SNIS236. The map, masks, address straps, signed
encoding, grade uncertainty and excluded functions remain valid. Page 4's
comparison table was visually rechecked for the supply/current discrepancies.

| Original actionable finding | Review and applied response |
|---|---|
| Complete serial electrical/protocol contract | Correct and already covered by 5–6/12/adapters. Preserve 1–400 kHz, explicit pointer/word framing and hardware-timeout distinction; no 1-MHz operating claim from a characterization curve. |
| Source conflicts contaminate defaults | Correct. Extended exact-spec precedence from accuracy to supply/current, fixed the CONV citation, and retained EEPROM/ID/reset-drawing conflicts. Added persistence-domain and addressed-reset uncertainty rather than inventing defaults. |
| Application conditions omitted | Correct and already in board/HIL owners, with a real integration gap: stages 6/21/30 now carry sparse default OS polling after the derived maximum active interval. Typical-time polling is not a guarantee of avoiding active conversion; accuracy remains physical evidence. |
| Persistence/averaging fixtures | Correct. Keep all 32 continuous timing pairs and four OS AVG cases plus CONV independence. Separate POR reload from uncertain addressed-reset outcomes; preserve unmanaged offset, and test explicit managed zero/guard refusal. |
| Exclusions not complete rows | Already addressed by X rows. Stage 32 explicitly consolidates no-ARA/general-call/programming/foreign-feature checks and disclosed external-interference effects. No new excluded-function API is needed. |
| TMP119M scope | Still unresolved from the pinned orderables; the historical dated webpage observation is not fresh availability evidence. Keep the selected TMP119 model without silently adding M. |

All original named discrepancies remain evidence obligations, with the
corrections above. These changes update instructions and source interpretation;
they do not implement a driver, run physical HIL or establish component accuracy.
