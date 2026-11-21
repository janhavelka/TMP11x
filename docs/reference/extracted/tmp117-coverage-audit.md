# TMP117/M/N complete source coverage audit

Audit scope: promptset coverage only; no driver or physical qualification is
claimed. All 50 pages of the pinned `TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf` were read, including revision
history, electrical footnotes, interface diagrams, register descriptions,
applications, packaging and notices. Compared prompts 4–7 and 11–33, shared
SILICON/FULL_COVERAGE/EVIDENCE contracts, and integration/qualification prompts
34–35 and 40–42. Baseline: `8d2ffe385f061575f4fa5402060ef63d2deaa579`.

Source: [TI SNOSD82D](../source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf), June 2018, revised September 2022,
SHA-256 `78212e91e478035a91165397a8de8a90bfc776759dc445243c033537f0c4b789`.
The pinned PDF additionally contains a package-option addendum dated
30 September 2026 and packing data dated 23 October 2026; these are distinct
from the technical revision date. Page references below are one-based PDF pages
and match printed pages through p.45. The original rendered pages 6, 7, 23, 27,
28, 31 and 46 were visually checked for column/footnote interpretation.

## Exact variant and package distinctions

All three selected BOM grades share the documented TMP117 register and protocol
model. No grade/package selector or accuracy-proof register is documented.
DEVICE_ID supplies base DID `0x117` and revision, not M/N identification.

| Selected grade | Accuracy maximum at the stated temperature intervals | Orderable operating range in pinned p.46 | Package choices in pinned p.46 |
|---|---|---|---|
| TMP117 | ±0.1°C: −20..50°C; ±0.15°C: −40..70°C; ±0.2°C: −40..100°C; ±0.25°C: −55..125°C; ±0.3°C: −55..150°C | −55..150°C | DRV WSON and YBG DSBGA |
| TMP117M | ±0.1°C: 25..50°C; ±0.15°C: 0..70°C; ±0.2°C: 0..85°C | **0..85°C** | DRV WSON and YBG DSBGA |
| TMP117N | ±0.2°C: −40..100°C; ±0.25°C: −55..125°C; ±0.3°C: −55..150°C | −55..150°C | DRV WSON and YBG DSBGA |

Electrical accuracy conditions are part of these claims: eight averages, a
1-Hz conversion cycle, unsoldered DRV thermal pad, and I2C low/high input levels
within 5% of the supply rails (p.6 §6.5). They differ from the ordinary digital
logic thresholds of 0.3/0.7 × V+. A 3.5-µA headline current is for averaging off;
the eight-average, 1-Hz row is 16 µA typical (p.6). Neither sample resolution nor
traceability words establish system accuracy.

Supply recommendation: 1.7–5.5 V only through 70°C, 1.8–5.5 V for the broader
temperature interval (p.5 §6.3). Intersect this with the selected orderable's
operating range, especially TMP117M. Absolute maximum and ESD ratings on p.5
are stress/handling information, not valid operating conditions or code ranges.

DRV is a six-pin 2 × 2 mm WSON; YBG is a six-ball DSBGA with a different pinout,
thermal mass and thermal metrics (pp.1,4–5). DRV mapping is SCL1/GND2/ALERT3/
ADD0=4/V+=5/SDA6; YBG mapping is SDA=A1/SCL=A2/V+=B1/GND=B2/ADD0=C1/ALERT=C2.
Mechanical drawings/land patterns/stencils are pp.40–45, and exact orderable
suffix/material/marking information is pp.46–49. These are board/BOM metadata,
not separate firmware capabilities. The pinned addendum marks the `YBGT`
orderables obsolete; this is dated evidence, not a promise of live availability.

**Missing precision:** stage 4 requests power/accuracy references but does not
require this actual grade-temperature/package/measurement-condition matrix.
Stages 4, 32, 40 and 42 should retain it and prevent a common −55..150°C claim
for every grade. Pure 16-bit codecs must still handle all representable codes;
physical limits must not be substituted for register encoding limits.

## Exhaustive register and field inventory

Legend: **S** = supported planned behavior; **R** = read-only first-release
observation; **X** = deliberately excluded write/command; **A** = application
responsibility. These are dispositions, not completed implementation claims.
All data registers are 16-bit words, MSB first. The reset domain of starred
values is EEPROM-loaded, so manufacturing values do not prove current state.

| Pointer/field | Mask, reset and semantics | Disposition / current owner |
|---|---|---|
| Pointer byte | Reset `0x00`; high nibble must be zero in Figs.7-9/7-10; selects a register, retained until changed | S; 5,12,26; no implicit pointer reuse or burst auto-increment |
| `0x00` TEMP[15:0] | `0xFFFF`; reset `0x8000`; signed 1/128°C; latest completed averaged result; TEMP read clears DATA_READY | S; 16,21,26,30; arithmetic and fresh validity separate |
| `0x01` HIGH_Alert[15] | `0x8000`; reset0; alert-mode read-to-clear; therm-mode hysteresis state | S; 6,17,19,22 |
| `0x01` LOW_Alert[14] | `0x4000`; reset0; alert-mode read-to-clear; therm mode always0 | S; 6,17,19,22 |
| `0x01` DATA_READY[13] | `0x2000`; reset0; set by result update; CONFIG or TEMP read consumes it | S; 6,19,21,26,30 |
| `0x01` EEPROM_Busy[12] | `0x1000`; reset0 in table; busy during programming/POR; mirror of `0x04[14]` | R; 7,18,24,25 |
| `0x01` MOD[11:10] | `0x0C00`; factory 00; 00=CC,01=SD,10=CC/readback00,11=OS; bit 11 not EEPROM-persisted | S stable CC/SD, explicit OS operation; 17–21,30 |
| `0x01` CONV[9:7] | `0x0380`; factory 100; eight cycle selectors, with AVG-dependent effective periods | S; 6,17,20 |
| `0x01` AVG[6:5] | `0x0060`; factory 01; 00/01/10/11 select 1/8/32/64 accumulated conversions | S; 6,17,20,21 |
| `0x01` T/nA[4] | `0x0010`; factory 0; 0=alert/window,1=therm/hysteresis | S; 17,22 |
| `0x01` POL[3] | `0x0008`; factory 0; 0=active low,1=active high | S; 17,22; pin remains open-drain |
| `0x01` DR/Alert[2] | `0x0004`; factory 0; 0 routes comparator flags,1 routes DATA_READY to ALERT | S; 17,22; flag observations separate from pin routing |
| `0x01` Soft_Reset[1] | `0x0002`; read0 always; write1 commands addressed 2-ms reset | S explicit guarded reset only; 25; ordinary writes force0 |
| `0x01` reserved[0] | `0x0001`; unused, R with reset value 0, write 0 in CONFIG | X meaningful writes; 5,17,26 |
| `0x02` THIGH[15:0] | `0xFFFF`; factory `0x6000` (+192°C); signed 1/128°C; EEPROM-loaded | S guarded volatile limits; 16,18,22 |
| `0x03` TLOW[15:0] | `0xFFFF`; factory `0x8000` (−256°C); signed 1/128°C; EEPROM-loaded | S guarded volatile limits; 16,18,22 |
| `0x04` EUN[15] | `0x8000`; reset0; 0=volatile writes,1=EEPROM programming enabled | R observation/X write; 7,18,24,26 |
| `0x04` EEPROM_Busy[14] | `0x4000`; reset0; mirrors CONFIG[12]; programming or POR loading | R observation; 7,18,24 |
| `0x04` reserved[13:0] | `0x3FFF`; unused, R with reset value 0 | R/X write; 5,24,26 |
| `0x05` EEPROM1[15:0] | `0xFFFF`; factory content not fixed; shadow of EEPROM; factory traceability content | R/X all writes including locked shadow writes; 7,24 |
| `0x06` EEPROM2[15:0] | `0xFFFF`; unknown factory content, despite contradictory section heading | R/X all writes; 7,24 |
| `0x07` OFFSET[15:0] | `0xFFFF`; factory 0, EEPROM-loaded; signed 1/128°C added after linearization; saturated result | S guarded volatile offset; 23; never a fourth EEPROM word |
| `0x08` EEPROM3[15:0] | `0xFFFF`; unknown factory content; shadow of EEPROM | R/X all writes; 7,24 |
| `0x0F` REV[15:12] | `0xF000`; revision field; default example0, not grade identity | R; 4,13 |
| `0x0F` DID[11:0] | `0x0FFF`; `0x117` | R; 4,13 |
| `0x09..0x0E`, other pointers | Not documented registers; no inferred meanings or alias access | X; checked access at 12,26 |

Sources: pp.20,22–23 and Tables7-3 through 7-15, pp.25–32. CONFIG factory word
is `0x0220`. Its ordinary writable bit positions are `0x0FFC`, with additional
semantic restrictions on MOD: the mask alone must not authorize OS as a stable
setting. Persistable CONFIG positions are bits 10:2 (`0x07FC`); first-release
firmware does not program them into EEPROM. Read effects and commands are
already required by existing prompts and are not newly missing features.

## Non-register behavior and disposition

| Source topic and evidence | Required behavior / disposition | Coverage finding |
|---|---|---|
| Power-up, pp.7,12,18 | Typical/approximate 1.5-ms startup, EEPROM busy during loading, CONFIG/limit writes ignored during POR, TEMP initial sentinel; S bounded guard/readback | Already covered by 6,18,25; never treat timing as guaranteed maximum |
| Reset domains, pp.18,25,27–32 | POR reloads custom CONFIG/limits/offset/shadows; p.18 explicitly attributes EUN clearing to general-call reset. Addressed Soft_Reset's reload/EUN/companion-bit/bus effects are not explicit; no assumed factory image | S guarded addressed-reset policy with postwait observations; separate POR/general-call fixtures from uncertain addressed-reset outcomes |
| Averaging, pp.12,27 | Block accumulation of 1/8/32/64 conversions, not running average; update TEMP/readiness/comparators only when result completes | Counts/timing covered; independent non-overlapping-average fixture missing |
| Continuous mode, p.14/Table7-7 p.28 | Active averaging plus standby; no standby when active period occupies cycle; S | Covered by 6,20 |
| Shutdown, p.14 | Entering SD immediately aborts active conversion; S; no new completed sample from aborted work | Covered by 20; add partial-averaging abort fixture |
| One-shot, p.15 | Active averaging only; CONV has no duration effect; completes into SD; OS not persistable startup mode | Covered by 6,17,21,30 |
| Alert/window and therm, pp.15–17 | Strict >high/<low comparisons at completion; read acknowledgement versus hysteresis; LOW=0 in therm | Covered by 22; all routing/polarity cross-products deserve explicit tests |
| ALERT electrical behavior, pp.4,6,33 | Open-drain output needs pullup even with active-high selection; GPIO observation is not comparator state or ownership | Core separation covered by 22; electrical application guidance missing |
| Four addresses, Table7-2 p.21 | ADD0 GND/V+/SDA/SCL → seven-bit 0x48/49/4A/4B | Valid range covered; physical mapping and doubled-address negative vectors missing |
| Serial words, pp.20–23 | Pointer then one MSB/LSB word; read repeated START, ACK MSB, NACK final LSB, STOP; slave never drives SCL | Core framing covered; explicit final ACK/NACK/electrical ownership requirements missing |
| Bus speed and hardware timeout, p.7/§7.5.3.1.7 p.22 | 1–400 kHz; serial interface timeout20–40 ms (35 ms typical prose), releases SDA/waits START | Missing distinct hardware-timeout/speed contract; app/adapter owns timing |
| Serial state, p.20 | Ignores traffic before START; STOP resets I2C state machine; no connecting during active traffic | Hot-plug restriction covered by41; no invented pointer reset on STOP |
| SMBus alert response, pp.21–23 | ARA responds at seven-bit 0x0C, reports sensor address/direction, arbitrates lowest address; winning alert-mode responder deasserts pin; therm does not clear | X driver operation under22; external ARA is possible and must be disclosed as outside interference |
| General-call reset, pp.22,24 | General-call 0x00 + byte 0x06 resets devices; simultaneous-reset sampling recipe p.34 depends on programmed EEPROM | X, including that synchronized-sampling recipe; A external bus authority |
| EEPROM programming, pp.7,18–19,30–32 | Unlock, persistent setting programming, scratch writes, endurance/retention and general-call relock sequence | X all programming, including volatile traceability-shadow writes; retained source inventory, no implementation task |
| Read-only EEPROM, pp.18,30–32 | Three noncontiguous observed shadows, mutable outside library, not immutable serial/NVM proof | Covered by24 |
| Offset, p.31 | Added after linearization to TEMP; signed full range; saturated output at extrema | API covered; independent saturation/no-double-offset tests missing explicitly |
| Thermal/power effects, pp.8–10,33–38 | Noise, averaging/current, bus traffic even in SD, self-heating, board-object coupling and mechanical stress affect results | A; missing engineering integration guide with test-condition caveats |
| Pins/supply/decoupling/layout, pp.4–7,33–38 | Exact package wiring, external pullups, 100 nF bypass near supply, qualified supply/temp, DRV pad advice | A; missing concrete hookup/reference matrix under4/34–35/40/42 |
| Mechanics, ESD, manufacturing, pp.5,39–49 | Handling, package drawings and assembly limits belong to board/BOM authority | A documentation references; not firmware functionality or new driver classes |
| Medical/NIST claims, pp.1,18,33,50 | Component source claims have conditions; system validation remains application responsibility | Already appropriately bounded by23/40/44; do not turn them into driver certification |

Nominal CC periods in Table7-7 (columns AVG=1/8/32/64):

| CONV bits | 1 | 8 | 32 | 64 |
|---|---|---|---|---|
| 000 | 15.5 ms | 125 ms | 500 ms | 1s |
| 001 | 125 ms | 125 ms | 500 ms | 1s |
| 010 | 250 ms | 250 ms | 500 ms | 1s |
| 011 | 500 ms | 500 ms | 500 ms | 1s |
| 100 | 1s | 1s | 1s | 1s |
| 101 | 4s | 4s | 4s | 4s |
| 110 | 8s | 8s | 8s | 8s |
| 111 | 16s | 16s | 16s | 16s |

The p.6 one-shot conversion entry is 13/15.5/17.5 ms MIN/TYP/MAX. The p.12
single-conversion discussion establishes the 15.5 ms nominal unit. Multiplying
single-conversion bounds by count is a derivation that must be labelled; rounded
cycle values and typical-characteristic curves are not independent guaranteed
deadlines. Existing stages 6/20 already require this distinction.

## Unspecified behavior: preserve UNKNOWN

The complete pinned datasheet does **not** specify:

- Whether writing an unchanged CONFIG restarts an active conversion, or exactly
  how changing CONV/AVG/other CONFIG settings affects a partial accumulated
  conversion. The explicit exception is SD abort. Do not give a simulator an
  invented immediate restart/preserved-average guarantee. Invalidate generation
  evidence; where a fresh new profile is required, use the owned SD baseline and
  new conversion policy, with its deliberate mutation documented.
- Whether TEMP is latched across its two I2C bytes when conversion completion
  coincides with a read, or the exact clock edge at which ready/alert reads clear
  flags. A normal combined two-byte read is required, but does not prove a
  stronger silicon latch/atomic snapshot claim. Record this unresolved boundary
  in the freshness evidence and any future targeted fixture investigation.
- An explicit internal ordering statement proving whether the limit comparator
  consumes the offset-adjusted or unadjusted value. Page15 describes comparison
  with the converted result; p.31 describes adding offset after linearization.
  An offset-to-comparator relationship is a plausible inference, not a stated
  block-order guarantee. Do not certify it via a self-authored fake; retain the
  uncertainty and qualify only with further primary evidence/authorized HIL.
- Atomic application of CONFIG, THIGH, TLOW and OFFSET across separate register
  transactions. Conversions and comparator decisions may occur between writes.
  State the library's apply ordering/intermediate-alert policy, and test partial
  failure; do not promise glitch-free paired thresholds or automatic rollback.
- MOD readback during/after one-shot and a MOD=11 write during a running OS.
  The physical return to SD is documented, but no MOD readback value proves
  it. The managed sequence explicitly writes guarded SD, establishes a CONFIG
  baseline and triggers once; no elapsed-time-only completion or retrigger.
- DATA_READY/alert-mode flag effects on SD entry and inverted therm limits.
  Therm HIGH itself is constrained by p.17's conversion-end-only rule while
  T/nA remains therm and no reset occurs. Routing/polarity affect the pin;
  pin persistence through SD with those settings unchanged is an inference.

The datasheet also exposes no conversion sequence counter, sensor conversion
timestamp, CRC/PEC register, running-average mode, or fault queue. No such
feature should be inferred from SMBus compatibility or related libraries.

## Actionable coverage additions

| ID | Missing obligation beyond the previous misconception audit | Prompt owners and closing evidence |
|---|---|---|
| T117-G1 | Concrete grade/package/supply/accuracy-condition matrix, especially TMP117M0..85°C and eight-average/1Hz accuracy conditions | 4,11,32,40,42: source-backed metadata and tests that never infer grade from DID or clip the arithmetic codec to grade range |
| T117-G2 | Complete serial electrical contract, ADD0 mapping, pointer high nibble, ACK/NACK ending, no assumed autoincrement/clock stretching | 5,12,34–36,40,42: all 4 address transcripts; adapter rejects bad clock configuration it owns; optional logic-analyzer proof kept separate from host tests |
| T117-G3 | Hardware20–40 ms timeout and1–400 kHz specification distinct from callback/job deadlines and conversion timing | 6,9,12,34–36,41: timeout documentation and modeled interface failure; no automatic bus/sensor reset |
| T117-G4 | Explicit unknown write-phase and byte-latch semantics; partial-profile application is not atomic | 6,18–21,29–32: record unknowns, fresh-profile policy, partial-average cancellation and partial limit/profile-write tests without invented phase behavior |
| T117-G5 | Accumulated-average semantics and independent offset saturation tests | 20–23,32–33: no intermediate ready/results before N samples; nonoverlapping input blocks; positive/negative saturation rather than wrap, no software offset reapplied; comparator-offset ordering remains unknown |
| T117-G6 | Exhaustive mode/routing/polarity checks, preserved external-ARA/reset effects and deliberate exclusions | 7,22,25–26,32,38,41: all 2×2×2 T/nA/POL/DR combinations, addressed CONFIG acknowledgement only, external intervention invalidation; no 0x00 or 0x0C operation in core |
| T117-G7 | Board integration/precision guide with thermal/current and electrical prerequisites | 4,34–35,40,42: pin/pullup/bypass/supply table, accuracy assumptions, bus-activity/self-heating notes; still-air/solid-contact/moving-air layout links; no generic solder-the-pad instruction |
| T117-G8 | Explicit source-conflict rows for EEPROM2 heading and older low-power prose | 5,7,24,42: preserved paired page references; unknown EEPROM2 content and condition-specific current claims |

Additional low-risk native vectors should explicitly include a nonfactory
EEPROM-loaded OFFSET/THIGH/TLOW/CONFIG fixture after reset and recovery, plus
all unimplemented pointer holes. This closes accidental hardcoded factory
defaults while staying within the existing read-only EEPROM policy.

## Retained source conflicts and limits

- p.18 and p.20 prose/diagram refer to four EEPROM locations; p.25 map and
  pp.30–32 descriptions expose three plus OFFSET. Already retained by prompts.
- The p.18 stored-field list omits DR/Alert and OFFSET, but p.27's footnote and
  p.25's offset default footnote mark them EEPROM-backed. Detailed fields
  govern persistence domains. Page 30 also repeats the four-word reference.
- **New explicit conflict:** p.31 EEPROM2 heading says reset `0000h`, whereas
  Table7-12/Fig.7-19 and Table7-3 p.25 say unknown (`xxxxh`). Do not fabricate
  a zero factory traceability word.
- p.27 register drawing labels Soft_Reset R-0; detailed p.28 Table7-6 calls it
  R/W and defines the command. Already retained by stage 25.
- p.15 one-shot wording mentions an ALERT flag without a distinct flag field;
  detailed comparator/routing definitions govern supported semantics. Already
  retained by stage 22.
- p.14 SD prose quotes250nA; p.1/p.6 and revision history p.3 use150nA typical
  under stated conditions. Do not promote the stale prose to a guaranteed
  shutdown current. p.18 EEPROM-current prose also says230µA against240µA in
  p.6's typical electrical row; programming remains excluded.
- p.33's address-table cross-reference says Table7-1; the actual address table
  is Table7-2 p.21. p.35's general1.7-V wording must retain p.5's temperature
  condition. These do not create new firmware modes.

No implementation, host C++ tests, MCU build, CI or physical measurements were
performed in this audit. The gaps above are prompt obligations for the lead to
integrate; the register/API feature set is already substantially covered.

## Response — 2026-11-17

Reviewed all T117-G rows and the source conflicts against `171dbf5`, current
prompts and pinned SNOSD82D. The register/grade tables remain valid; they are
not claims of implemented support. This review visually rechecked pp.6–7:
the DRV pad condition spans all grades and `tRESET=1.5 ms` is TYP, not MAX.

| Finding | Review and applied response |
|---|---|
| T117-G1 | Correct and already required by stage 4's matrix. Clarified that all grades offer DRV/YBG and the DRV condition spans them; M remains 0–85°C. |
| T117-G2 | Correct and largely addressed by 5/12/adapter obligations. Exact word/pointer framing and no burst/CRC fallback remain required; address acknowledgement alone proves no family identity. |
| T117-G3 | Correct and already represented by 6/9/12. Kept hardware line timeout separate from callback/job deadlines, and one-shot-conditioned conversion bounds separate from continuous estimates. |
| T117-G4 | Correct but incomplete. Added undocumented OS MOD/retrigger and addressed-reset effects; explicit SD/baseline boundaries replace MOD-only assumptions. Multi-register writes still cannot promise atomicity or rollback. |
| T117-G5 | Correct. All 32 continuous pairs remain; OS needs four AVG cases plus CONV independence. Stage 23 preserves unmanaged calibration by default, honors explicit managed zero and checks EUN/busy before offset writes. Saturation vectors remain independent of comparator-order uncertainty. |
| T117-G6 | Correct and already in the relevant stages. Added sticky capture for all CONFIG-consumed alert events, explicit therm inversion refusal, and X03/X08 negative-test consolidation. No ARA/broadcast/programming API was added. |
| T117-G7 | Correct and already assigned. Default maximum-based sparse OS polling now carries the bus-activity/precision concern into 6/21/30, without claiming a software schedule proves system accuracy. |
| T117-G8 | Correct; EEPROM2 heading and current-prose conflicts remain. Added the omitted DR/Alert/OFFSET persistence and four-word diagram/detail citations. |
| Reset-domain row | Partly incorrect: corrected its blanket EUN-clear/reload claim. General-call/POR evidence does not explicitly establish all addressed Soft_Reset effects. Stage 25 uses a labelled conservative encoding, wait, fresh 0x04 check and full observed readback. |
| Existing retained conflicts | EEPROM count, EEPROM2 default, reset access drawing, one-shot ALERT wording, current prose and address-table reference were rechecked and remain valid. No conflicting summary overrides an exact field/electrical table. |

The former “missing” labels describe the historical audit baseline. Their
prompt obligations now exist; production driver, MCU and physical evidence
remain unexecuted. No additional supported chip or nonvolatile write path is
authorized by this response.
