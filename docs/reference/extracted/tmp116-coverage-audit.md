# TMP116 / TMP116N complete coverage audit

Reviewed 2026-11-15 against the pinned [TMP116 PDF](../source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf),
SBOS740A (May 2019 body), SHA-256
`93cd03c207c2871918ef9341a4baf189e2c0ea9a795673a7011e367c4e75c1c0`.
Page numbers are one-based PDF pages. All 47 pages were inspected as text;
pages 4, 5, 6, 15, 26, 27, 32, 36 and 43 were also rendered and visually
checked in this audit. The attached orderable addendum is dated September 25,
2026; it is distinct from the datasheet body revision. This is original audit
analysis, not a replacement for the vendor document.

Compared against all 44 numbered prompts and the shared silicon/coverage
contracts at input commit `8d2ffe385f061575f4fa5402060ef63d2deaa579`.
There is no driver implementation to qualify yet. "Supported" below means
included in the implementation plan, not implemented, tested or physically
qualified. Existing exclusions remain in force.

## Register and field inventory

| Register / field | Exact TMP116/N behavior and reset domain | Disposition / prompt owners |
|---|---|---|
| Pointer byte | 8-bit selector, power-up 0x00, retained until another pointer write; figures show upper nibble zero. No documented register auto-increment. pp.19-22 | Supported one-word transactions; always select the pointer as an ownership policy. 5, 12, 26 |
| TEMP 0x00, 15:0 | Read-only signed two's complement; 1/128 degree C per count. Reset 0x8000 until first conversion/averaging result. Read consumes DATA_READY. pp.11, 25-26 | Latest observations separate from freshness and initial validity. 5, 16, 21, 26 |
| CONFIG 0x01, 15 HIGH_Alert | Read-only reset 0. Alert-mode high event latch; CONFIG read acknowledges. Therm-mode hysteresis flag changes on completed conversions, not CONFIG reads. pp.15-16, 26 | Supported status and acknowledgement. 6, 17, 19, 22 |
| CONFIG 14 LOW_Alert | Read-only reset 0. Alert-mode low event latch acknowledged by CONFIG; always zero in therm mode. pp.15-16, 26 | Supported. 6, 17, 19, 22 |
| CONFIG 13 DATA_READY | Read-only reset 0; set when result register updates; CONFIG or TEMP read clears it. It is one bit, not an event counter. pp.13-14, 26 | Supported with consumption accounting. 6, 19, 21, 30 |
| CONFIG 12 EEPROM_Busy | Read-only reset 0; table says asserted during programming/power-up. Mirrors 0x04 bit 14. Reset-load prose conflicts; see below. pp.17, 26, 29 | Read-only safety evidence, insufficient alone for TMP116 startup. 5-7, 18, 24 |
| CONFIG 11:10 MOD | 00 CC, 01 shutdown, 10 CC alias reading back 00, 11 one-shot. **Bit11 MOD1 is not EEPROM-stored**, bit 10 MOD0 is stored. Thus only CC/SD can be startup settings. p.26 footnotes1-2 | Stable CC/SD settings plus explicit OS operation; no addressed reset. 5, 17, 20-21, 25 |
| CONFIG 9:7 CONV | All eight codes valid; controls CC cycle, not OS time or an independent standby duration. EEPROM-stored, factory 100. pp.12-14, 26-27 | Supported, all 32 CONV/AVG pairs. 6, 17, 20 |
| CONFIG 6:5 AVG | 00/01/10/11 accumulate 1/8/32/64 samples. Block average, not rolling average. EEPROM-stored, factory 01. pp.12, 26-27 | Supported. 6, 17, 20-21 |
| CONFIG 4 T/nA | 0 alert / 1 therm; EEPROM-stored, factory 0. pp.15-16, 26 | Supported. 17, 22 |
| CONFIG 3 POL | 0 active-low / 1 active-high. Pin remains open-drain. EEPROM-stored, factory 0. pp.3, 26 | Supported routing semantics; GPIO electrical handling application-owned. 17, 22, 34-35 |
| CONFIG 2 DR/Alert | 0 pin reflects alert flags / 1 pin reflects DATA_READY. EEPROM-stored, factory 0. p.26 | Supported; pin source distinct from comparator status. 17, 22 |
| CONFIG 1:0 | Not used, read-zero. No individually addressed software-reset bit. p.26 | Writes zero; reset request rejected before I2C. 5, 17, 25-26 |
| THIGH 0x02, 15:0 | Signed same format as TEMP; EEPROM-backed writable shadow; factory 0x6000 =192 C. Reset reloads programmed value. p.28 | Guarded volatile read/write; full representation not restricted to rated measurement range. 16, 18-19, 22 |
| TLOW 0x03, 15:0 | Signed same format; EEPROM-backed writable shadow; factory 0x8000 =-256 C. Reset reloads programmed value. p.28 | Same as THIGH. 16, 18-19, 22 |
| EEPROM_UL 0x04, 15 EUN | R/W, reset0. 0 keeps relevant writes in volatile shadows; 1 enables programming corresponding cells. p.29 | Read only; *all* writes to 0x04 excluded by library policy, including attempts to lock it. 7, 18, 24, 26 |
| EEPROM_UL 14 busy | R, reset0; mirrors CONFIG bit 12. Detailed table covers programming and reset load. p.29 | Supported read-only observation and write guard. 5-7, 18, 24 |
| EEPROM_UL 13:0 | Unused, read-zero. p.29 | Ledger/reserved-bit protection. 5, 24, 26 |
| EEPROM1 0x05, 15:0 | Full word, R/W shadow corresponding to NVM; detailed resetXXXX. pp.17, 29 | Read only in release, even while locked. 7, 24, 26 |
| EEPROM2 0x06, 15:0 | Same behavior as EEPROM1; detailed resetXXXX. p.30 | Same. 7, 24, 26 |
| EEPROM3 0x07, 15:0 | Same behavior as EEPROM1; detailed reset0000. **Not temperature offset.** p.30 | Same; offset requests rejected before I2C. 23-24, 26 |
| EEPROM4 0x08, 15:0 | Same behavior as EEPROM1; detailed resetXXXX. p.31 | Same. 7, 24, 26 |
| DEVICE_ID 0x0F, 15:0 | Read-only 0x1116; all 16 bits called DID. No documented revision nibble or N-grade discrimination. p.31 | Family compatibility only, grade supplied from BOM. 4, 13 |
| Pointers 0x09-0x0E and others | No mapped registers; no documented alternative offset/CRC/FIFO register. p.24 | Reject, do not probe holes as invented features. 5, 12, 26 |

CONFIG factory 0x0220 and THIGH/TLOW factory defaults are not immutable reset
values: EEPROM programming can change the persisted fields. Status flags,
reserved bits and MOD1 must not be modeled as EEPROM-persisted settings.
The complete ordinary CONFIG writable bit mask is 0x0FFC; the subset documented
as EEPROM-stored is 0x07FC. Those are hardware field facts, not permission to
emit OS through ordinary settings or to program EEPROM.

## Conversion and alert behavior inventory

The following nominal CC cycle matrix (milliseconds) covers every Table 7
combination on p.27. AVG columns represent 1, 8, 32 and 64 conversions.

| CONV code | AVG 00 | AVG 01 | AVG 10 | AVG 11 |
|---|---:|---:|---:|---:|
| 000 | 15.5 | 125 | 500 | 1000 |
| 001 | 125 | 125 | 500 | 1000 |
| 010 | 250 | 250 | 500 | 1000 |
| 011 | 500 | 500 | 500 | 1000 |
| 100 | 1000 | 1000 | 1000 | 1000 |
| 101 | 4000 | 4000 | 4000 | 4000 |
| 110 | 8000 | 8000 | 8000 | 8000 |
| 111 | 16000 | 16000 | 16000 | 16000 |

Single-conversion limits are 13.5/15.5/17 ms min/typ/max under p.5 conditions.
Multiplying these by AVG count gives an explicitly *derived* active interval;
it is not a separately specified averaged-conversion guarantee. Table 7 rounds
the active times (125/500/1000 ms), while p.12 illustrates 8 x 15.5 = 124 ms.
The rounded cycle matrix and derived bounds must remain distinct. Standby is
zero when the active time fills the nominal cycle. OS has no standby and is
independent of CONV. These are already substantially covered by prompts 6/20.

| Behavior | Source / disposition |
|---|---|
| CC updates TEMP only at the end of conversion or averaging block and sets DATA_READY | pp.11-13; supported, prompts 6/20/21 |
| SD write immediately aborts the running conversion and reduces current | p.13; supported, prompts 20/21/32. Aborted partial averaging is not a new completed sample |
| OS is requested from shutdown, completes AVG block, then returns to shutdown | p.14; supported explicit operation, prompts 21/30 |
| CONFIG/TEMP acknowledge readiness; no sample counter or read-triggered conversion | pp.13-14, 26; supported explicit evidence bookkeeping, prompts 19/21/26 |
| Alert-mode HIGH and LOW use strict greater-than/less-than; events latch until acknowledged | p.15/Table 6 p.26; supported, prompt 22. Returning in-range does not acknowledge a latched event; a still-out-of-range next conversion can reassert after CONFIG acknowledgement |
| Therm HIGH sets above THIGH, stays through hysteresis band including equality, clears below TLOW; LOW stays 0 | p.16; supported, prompts 22/32 |
| Pin comparator/ready routing and POL affect output interpretation, not temperature encoding | pp.3,13-16,26; all eight T/nA x POL x DR/Alert combinations need coverage, prompts 22/32/41 |
| ALERT remains an open-drain output even for active-high selection | pp.3,26; electrically assertion is release/high with external pullup, not an actively driven high output. GPIO/pullup ownership belongs to application, prompts 22/34/35 |

No wording was found establishing the following details in this revision:
same-value CONFIG writes restarting or preserving conversion progress; changing
AVG/CONV mid-conversion reusing or discarding the partial block; exact DATA_READY
or comparator-flag retention/clearing on entry to SD; read-byte latching of TEMP
when a conversion completes between MSB and LSB; exact partial-word-write
commit behavior. TEMP is described as the most recent completed conversion
(p.25), but there is no separate shutdown-reset guarantee.

Keep these as **unknown source semantics**, not fake-device facts. A conservative
generation invalidation policy, explicit shutdown-to-one-shot sequence, single
combined word transfer and ambiguous-write accounting are project policies.
One-shot followed by shutdown avoids continuous updates during its result read;
continuous-mode coherence must not be described as datasheet-proven byte
latching. Do not invent a double-read workaround or retry policy. Owners:
6/19/20/21/31/32, with physical questions retained for41.

## Serial interface, reset and nonvolatile functionality

| Documented feature | Source | Release disposition and test boundary |
|---|---|---|
| ADD0=GND/V+/SDA/SCL selects7-bit 0x48/0x49/0x4A/0x4B | p.20 Table 2 | Supported numeric addresses; straps and unique same-bus addressing are board-owned. 4/11/12/34-36/40 |
| Slave receiver/transmitter, START/STOP/repeated START, ACK/NACK, pointer persistence, MSB then LSB | pp.19-23 | Core uses complete one-word writes and combined reads with explicit pointer. Adapter must ACK first byte and finish with NACK/STOP correctly. 12/34/35/40 |
| Slave never drives SCL; integrated input spike filtering/Schmitt behavior | pp.19-20 | Board/controller fact, no clock-stretch feature to enable in core. 12/34/35 |
| Bus 1-400 kHz and timing setup/hold/rise/fall limits | p.6 | Application controller/pullups must comply; host fake cannot prove electrical compliance. 12/34/35/40 |
| Serial-interface timeout 20-40 ms; 35 ms typical held-line description | pp.6,21 | Hardware interface timeout, distinct from operation/callback/conversion deadlines. Avoid qualifying only from the typical 35 ms. The line-held/no-clock timeout is not a maximum total job or transfer duration. 6/12/31/34/35 |
| SMBus Alert Response Address:7-bit 0x0C/read address byte 0x19; response is device address plus high/low status LSB; lowest address wins arbitration | pp.21,23 | Hardware capability deliberately excluded. Record external-owner effects; do not create a normal ARA callback/CLI handle. CONFIG acknowledgement is included instead. 7/22/35/38 |
| Alert-mode ARA deasserts winning device pin; losing device keeps asserted; therm mode does not clear via ARA | pp.15-16,21 | Excluded transaction with documented external effect. Do not invent ARA behavior for simultaneous high/low flags or ready-output routing; these cases are not fully specified here. 6/7/22 |
| General-call address 0x00, command 0x06 resets registers to programmed startup values | pp.21,23 | Deliberately excluded bus-wide command; externally reported reset invalidates state. No addressed TMP116 equivalent. 7/19/25/31 |
| Synchronized measurements by persisted CC settings plus general-call reset | p.35 | Deliberately excluded; no multi-sensor synchronization guarantee from sequential one-shots. 7/25/36/42 |
| POR reloads EEPROM into shadows; reads possible during load, writes ignored; approximate 1.5 ms | pp.10,17 | Startup guard/readback supported; contradiction on busy retained. 6/7/18/32 |
| EUN unlock, writes to persistent CONFIG/limits/data, program busy, general-call reload/lock | pp.17-18,26,29-31 | Entire programming flow excluded, including locked scratch-shadow writes. Never use it as recovery. 7/24/26 |
| Programming 7 ms typical, endurance 1000 min / 50000 typ, retention 10 min / 100 typ years | p.6 | Reference-only characteristics of excluded NVM feature; no library persistence or endurance guarantee. 7/42 |
| NIST production traceability stored in four EEPROM words | pp.17,29 | Read-only shadow metadata; no guaranteed original factory serial/certificate. 7/24/40 |

No PEC/CRC byte, fault queue, FIFO, software address selection, hardware offset
or register auto-increment is documented for this part. Do not import them
from TMP114, TMP117 or unrelated SMBus drivers. Bus reset/recovery, mux control,
address scans, scheduling and physical IRQ configuration remain application-owned.

## Grade, electrical and application coverage

TMP116 and TMP116N share this programming description,6-pin 2 x 2-mm WSON DRV
package and 1.9-5.5 V supply specification (pp.1,3-5,43). The N suffix changes
the specified accuracy, not the register model or ID. Page 5 accuracy requires
its stated averaging/rate, supply and temperature conditions:

| Grade | Maximum error and conditions |
|---|---|
| TMP116 | 0.2 C at -10..85 C / 3.3 V;0.25 C at -40..105 C / 3.0-3.6 V;0.3 C at 105..125 C / 3.0-3.6 V;0.4 C at -40..125 C / 1.9-5.5 V |
| TMP116N | 0.3 C at -25..85 C / 3.3 V;0.4 C at -40..125 C / 3.0-3.6 V;0.5 C at -40..125 C / 1.9-5.5 V |

Page 5 footnote 2 specifies 0.75 C maximum error for -55..-40 C. Accuracy tests use
8 averages, 1-Hz cycle, oil-bath measurements. Resolution, repeatability,
typical drift, absolute accuracy and calibrated system uncertainty are distinct.
No I2C read proves grade or calibrated accuracy. Owners 4/16/32/40/42.

The pinned package addendum p.43 lists TMP116AIDRVR and TMP116NAIDRVR plus
`.A` entries, with T116/116N markings; the DRVT entries are marked obsolete
in that snapshot. Do not turn carrier/material suffixes or a dynamic lifecycle
status into new driver models. Recheck current ordering data for purchasing.

Electrical and physical constraints are application/fixture evidence, not new
core APIs: relative logic thresholds 0.7V+ and 0.3V+, SDA/ALERT sink behavior and
pullups (p.5); 0.1-uF local bypass and supply tolerance (pp.32,36); power and
serial-activity self-heating, duty-cycle/averaging tradeoff and thermal coupling
(pp.7-9,12-13,33-37); protection/ESD, absolute maximum versus normal operating
ratings (pp.4,10,39); PCB thermal mass, airflow/surface placement and stress
(pp.36-38). TMP116 precision guidance recommends **not soldering the thermal
pad on rigid PCB**, with specific flexible-PCB/reliability exceptions; if
soldered it may be floating or ground (pp.36-37). Do not copy a sibling's pad
connection guidance without checking the exact part.

Pages 40-42 give package/land/stencil geometry, and 45-46 shipping dimensions.
They are reviewed as hardware-manufacturing scope, not runtime capabilities.
Page 39 documentation/support and page 47 license/disclaimers remain reference
provenance; vendor content does not become MIT core code. Owners 4/34-36/40/42.

## Retained conflicts and coverage additions

| Priority / gap | Source-backed finding versus proposed policy | Owner prompts and acceptance evidence |
|---|---|---|
| P1: serial electrical/timeout coverage omitted | Facts: 1-400 kHz, no SCL drive, 20-40 ms timeout under held-line conditions. Policy: explicit documented adapter constraints, no 1-MHz default inherited from another chip; do not mistake hardware timeout for whole-operation bound | 6/12/34/35/40: literal bus limits/address mapping and physical ACK/repeated-START/clock record; adapter failure test for interface interruption without hidden reset/retry |
| P1: conversion transitions/coherence assumed by future fake | Datasheet silence for same-CONFIG writes, mid-average setting changes, SD flags and between-byte TEMP update must remain unknown. Conservative invalidation is library policy | 6/19/20/21/32: unknown-semantics list, new-profile sample trust tests, aborted-average no-completion test; 41 records actual silicon evidence without claiming fake proves it |
| P1: operational range conflict missing | p.4 recommended TA permits -55..150 C; p.5 guaranteed electrical conditions, p.10 specified operation and p.43 orderables are -55..125 C. Do not silently choose one or claim accuracy to 150 C | 4/5/42: explicit conflict and qualified metadata separating raw representability, recommended conditions, specified performance and actual fixture range |
| P2: alert latch/rearm not explicit enough | pp.15/26 describe latched alert events, CONFIG acknowledgement, and next conversion evaluation; therm behavior is separate | 6/22/32: out-of-range then in-range before ack, ack while still out-of-range then next-conversion reassertion, T/nA x POL x DR/Alert matrix; 41 physical pin/flag evidence |
| P2: EEPROM reset disagreement insufficiently enumerated | p.24 says all four data words 0000; detailed 1/2/4 say XXXX,3 says 0000; pp.17/29 describe unique data across all four. No all-zero/nonzero factory-identity invariant is justified | 5/24/32: explicit per-register conflicting defaults, arbitrary/zero shadow values accepted as observations, no certificate claim |
| P2: field-level persistent domain missing | p.26 footnotes: MOD1 not EEPROM-stored, MOD0 and lower settings stored. Register-level R/W alone loses this distinction | 5/17/32: field/reset-domain ledger and fake reset reloads persisted subset rather than replaying MOD=11 |
| P2: full hardware disposition incomplete | ARA arbitration/status response and synchronized general-call conversions exist but remain excluded; NVM programming statistics and flow are likewise excluded | 5/7/22/25/42: explicit excluded rows with rationale; API/CLI negative tests show no general-call/ARA/unlock traffic |
| P2: board/accuracy applicability underspecified | Exact address straps, open-drain active-high meaning, supply/rate conditions, self-heating and TMP116 pad guidance must accompany integration/HIL | 4/22/34-36/40/42: board checklist and eight routing cases, grade-condition references; no bus or thermal result fabricated by native tests |
| P2: ALERT wiring ambiguity missing | p.32 Figure 41 uses pullup plus MCU input, while its final sentence recommends ground without an unused-pin qualifier. p.36 Figure 44 shows unused ALERT unconnected. Do not apply the ground sentence to a used interrupt pin | 4/22/34/35/40/42: retain conflicting guidance, document functional pullup/input connection, identify unused-pin recommendation as requiring interpretation/vendor clarification |

The earlier audit's TMP116 busy contradiction (p.17 versus 26/29), approximate
POR timing, reserved reset bits, signed temperature format and OS-from-SD
requirements remain valid. They are already enforced by existing prompts and
are not new gaps here. The new evidence does not authorize EEPROM programming,
ARA/general-call implementation, hardware alterations or running physical tests.
