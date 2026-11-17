# Silicon family facts and retained source conflicts

This is a **seeded source map**, not the full checked ledger. Prompt 3 downloads
and pins the original exact-part PDFs; prompts 4–7 independently validate
all fields and ambiguities. The datasheet, with exact revision/footnotes,
prevails over this secondary note.
The [promptset audit](../reports/promptset-audit.md) records corrections to
these seeds. Page numbers here refer to the pinned manifest revisions.
The subsequent [feature-gap audit](../reports/feature-gap-audit.md) and
[coverage inventory](FULL_COVERAGE.md) add complete register/function
dispositions, source unknowns and electrical/grade requirements.

## Included and excluded parts

| Selected BOM model | Register family | Variant statement |
|---|---|---|
| TMP116 / TMP116N | 116 | Same sensor programming model; accuracy specification differs. 64-bit EEPROM. |
| TMP117 / TMP117M / TMP117N | 117 | Same programming model and DRV/YBG packages; accuracy rows differ, TMP117M orderable range 0–85 °C. 48-bit EEPROM, hardware temperature offset. |
| TMP119 | 117-like | Very close programming model to TMP117, higher accuracy, 48-bit EEPROM, hardware temperature offset. |
| TMP114 / TMP114N | excluded | Different register/CRC and low-voltage behavior; supply 1.08–1.98 V, I/O 0–1.98 V, SCL/SDA absolute maximum 2.1 V. TMP114A/B/C/D use 0x48/49/4A/4B; TMP114ND/NC/NB/NA use 0x4C/4D/4E/4F. |
| TMP118 | excluded | Different 0x02/0x03 limit order, CONFIG bitfields, ID location and UID procedure. |
| TMP102 / TMP112 / TMP1075 / TMP110 | excluded | Different encoding/configuration or interface. Existing TMP1x2 handles supported TMP102/112 models. |

Product grades must be selected from the **BOM**. Identity reads prove at
most the hardware family documented below; they do not prove an advertised
accuracy grade. No auto-detect that silently picks TMP117 or TMP119 from
revision bits.
TMP119's electrical table also mentions TMP119M. This is an unresolved
orderable/grade scope question for stage 4, not permission to silently add a
new supported model.

## Register map seed — verified against TI register map pages

| Pointer | TMP116/TMP116N | TMP117/M/N and TMP119 |
|---|---|---|
| 0x00 | TEMP (read only, reset 0x8000) | TEMP (read only, reset 0x8000) |
| 0x01 | CONFIG (factory 0x0220, programmable reset) | CONFIG (factory 0x0220, programmable reset) |
| 0x02 | THIGH | THIGH |
| 0x03 | TLOW | TLOW |
| 0x04 | EEPROM unlock/busy | EEPROM unlock/busy |
| 0x05 | EEPROM1 | EEPROM1 |
| 0x06 | EEPROM2 | EEPROM2 |
| 0x07 | EEPROM3 (**not offset**) | Temperature offset (**not EEPROM3**) |
| 0x08 | EEPROM4 | EEPROM3 |
| 0x0F | Read-only ID, specified 0x1116 | Read-only ID, low 12 bits 0x117; upper nibble denotes revision |

Sources: TMP116 SBOS740A p.24; TMP117 SNOSD82D p.25;
TMP119 SNIS236 p.32. These are one-based PDF and printed page numbers.
TMP119's register-map table shows `0x0117` at 0x0F while its device-ID
section specifies default `0x2117`: **retain this vendor inconsistency in the
ledger**. Read the complete device-ID section and avoid asserting unique
TMP119 identity from a fixed high nibble.

## Common CONFIG field seed

TMP116/TMP117/TMP119 documented layout: `[15] high alert`, `[14] low alert`,
`[13] data ready`, `[12] EEPROM busy`, `[11:10] mode`, `[9:7] conversion cycle`,
`[6:5] averaging`, `[4] therm/alert`, `[3] alert polarity`, `[2] ALERT/data-ready
pin function`, `[1:0] part-specific/reserved/reset behavior`. Datasheets show
`0x0220` as factory CONFIG default, **not a guarantee after EEPROM programming**.
MOD=10 is a legal continuous-mode encoding and reads back MOD=00; it is not
reserved. TMP116 bits 1:0 are reserved. TMP117/TMP119 bit 1 commands software
reset and always reads zero; ordinary setting writes must never set it.
CONFIG bit11 MOD1 is explicitly not EEPROM-stored; bit10 MOD0 and bits9:2
are persistent-capable hardware fields. First-release software does not
program any of them into NVM. Factory CONFIG is not a whole-word persisted
image of status, commands and settings.
Sources: TMP116 p.26; TMP117 pp.27–28; TMP119 pp.34–35.

TEMP is signed 16-bit, 1/128 °C per LSB, MSB first. `0x0C80` = +25 °C,
`0xF380` = -25 °C, `0xFF80` = -1 °C, `0xFFFF` = -0.0078125 °C.
`0x8000` reads as -256 °C after reset until the first conversion **including
averaging** completes; do not blindly interpret it as a valid initial sample.
The raw signed representation remains valid arithmetic; freshness/validity is
separate from raw bit pattern.

CONFIG and TEMP reads clear DATA_READY. CONFIG reads clear HIGH/LOW in alert
mode; in therm mode HIGH follows threshold/hysteresis and LOW remains zero.
TEMP does not clear comparator flags. Do not allow a diagnostic dump to falsely
prove freshness or erase evidence without surfacing the side effect.

## EEPROM and identity constraints

- First-release API reads EEPROM but **does not unlock, program, delete,
  erase or persist any nonvolatile value**.
- Configuration/threshold/offset writes may hit nonvolatile memory if a prior
  writer left EEPROM unlocked; verify lock/busy before mutation, fail closed.
- TI states manufacturer EEPROM locations contain unique/NIST traceability
  content. Do not assume they are disposable scratch space.
- `TMP116` device ID `0x1116` is specified as 16 bits. TMP117/TMP119 document
  12-bit `0x117` ID with upper revision nibble; TMP117 example `0x0117` and
  TMP119 example `0x2117` may differ but are not a stable exact-part proof.
- Power-on may take a finite EEPROM load interval; no configuration writes
  until ready. A default configuration may have been customized by prior
  EEPROM writes.
- TMP116 p.17 section 7.5.1.1 says EEPROM_Busy is cleared during reset load,
  but p.26 Table 6 and p.29 Table 10 say it is asserted during power-up/load.
  Retain both statements. A cleared busy flag alone cannot establish startup
  completion; document the startup timing/owner assumptions and bounded
  readback. The approximately 1.5 ms load time is not a specified maximum.
- TMP117 p.18 and TMP119 p.20 prose mention four general-purpose EEPROM words,
  contradicting their three-word register maps (pp.25 and 32). Detailed maps
  assign 0x07 to offset and 0x05/0x06/0x08 to EEPROM; do not invent EEPROM4.
- The same four-word mismatch appears in TMP117 p.20 Figure 7-8/p.30 and
  TMP119 p.22 Figure 7-9/p.37. Their startup summaries (pp.18/20) omit
  DR/Alert and offset from stored fields, while the CONFIG and map footnotes
  (TMP117 pp.25/27; TMP119 pp.32/34) mark them EEPROM-backed. Detailed field
  tables govern the conservative write guards and programmable reset domains.
- Startup ignored writes are model-specific: TMP116 p.17 says all register
  writes; TMP117 p.18/TMP119 p.20 specify CONFIG and limits. Offset writes
  during load are undocumented on TMP117/TMP119; no fake may assert ignored
  as the only outcome. TMP117 p.7 tRESET=1.5 ms is typical, without a maximum.
- Reads at EEPROM addresses access volatile shadows. Earlier locked writes
  could have changed those values without changing NVM. They do not prove an
  immutable factory serial or physical nonvolatile contents.
- TMP117 p.28 Table 7-6 and TMP119 p.35 Table 8-5 define a 2 ms per-device
  software reset. TMP116 has no documented equivalent CONFIG reset command;
  its general-call reset remains intentionally excluded.
- For TMP117/TMP119 addressed soft reset, effects on EUN, EEPROM reload,
  other written bits and bus ACK availability are not explicitly specified.
  The general-call relock statement is not proof of an addressed-reset relock.

## Additional completeness-audit findings

The per-chip coverage notes linked from FULL_COVERAGE retain detailed pages,
grade tables and full hardware inventories. These are mandatory inputs to
stages 4–7, not new implemented capabilities:

| Finding | Source and bounded interpretation |
|---|---|
| TMP116 temperature-range discrepancy | SBOS740A p.4 recommended conditions say −55..150 °C; p.5 electrical conditions, p.10 specified operation and pinned orderables p.43 say −55..125 °C. Preserve both; do not advertise +150 °C accuracy. |
| TMP117M exact grade range | SNOSD82D p.46 pinned orderables specify 0..85 °C. Other grades' wider ranges and shared encoding do not extend this grade's guarantee. |
| Supply and accuracy conditions | TMP117 pp.5–6 / TMP119 pp.6–7 qualify 1.7 V through 70 °C and 1.8 V for the wider range; accuracy conditions include eight averages, 1-Hz cycle and near-rail digital inputs. TMP117's unsoldered thermal-pad condition applies to DRV across all grades; YBG has no pad. TMP116 has its own supply/grade/assembly conditions. |
| TMP116 EEPROM defaults | p.24 map says all four words 0000; detailed pp.29–31 say EEPROM1/2/4 unknown, EEPROM3 zero, while factory-data prose describes unique contents. No zero/nonzero serial validity rule is justified. |
| TMP117/TMP119 EEPROM2 reset | TMP117 p.31 / TMP119 p.38 heading says 0000, but their field tables and maps say unknown. Preserve arbitrary observed contents. |
| TMP119 portfolio summary | p.4 disagrees with exact §6.5 p.7 accuracy and p.8 shutdown-current rows; its TMP116 supply entry also contradicts SBOS740A p.4 (1.9–5.5 V). Source each model's metadata from its own detailed electrical tables, retaining conditions and discrepancies. |
| Current and speed summaries | TMP117 p.14 / TMP119 p.17 say 250 nA shutdown versus electrical rows 150 nA typical under stated conditions. TMP119 p.11's 1-MHz characterization curve does not override its 400-kHz protocol specification. |
| TMP116 ALERT wiring | p.3 requires a pullup if used; p.32 functional circuit uses pullup/MCU input, but prose recommends grounding without an unused qualifier. p.36 Figure 44 omits signal connections generally, not an explicit unused-ALERT instruction. Do not substitute TMP117/TMP119 unused-pin prose as TMP116 evidence. |

TMP114 negative hardware fixtures require compatible low-voltage logic or
level shifting; a pullup ≤1.98 V alone does not ensure every bus participant's
VIH/VIL requirements. Never attach it directly to a 3.3 V pullup bus. TMP118
has different I/O limits (SNIS237A p.6). Identity at 0x0F reads zero on these
excluded parts with the documented ordinary protocol; TMP114 CRC-enabled
error behavior needs separate fixtures (bad-CRC NACK/FF is documented, omitted
CRC is not assumed identical). Identity-first dumps must stop on mismatch.

All included devices specify I2C 1–400 kHz, a 20–40 ms hardware serial timeout
(35 ms typical under documented held-line/no-clock conditions), and no slave
SCL drive. The timeout resets the interface, not the complete sensor or desired
profile; software deadlines remain independent. ADD0 maps GND/V+/SDA/SCL to
0x48/49/4A/4B. Use explicit one-word pointers and no assumed auto-increment.

The datasheets do not explicitly settle same-value CONFIG restarts, mid-block
AVG/CONV updates, shutdown retention of DATA_READY/alert-mode flags,
MOD readback during/after one-shot, writes of MOD=11 during a running shot,
inverted therm limits, partial-word commits or TEMP
byte-latching during updates. TMP117/119's offset/comparator order is also
not explicitly proven by the offset-register text. Record conservative library
policy, uncertain claims and required evidence separately. Do not invent a
simulator behavior or repeated-read workaround and label it a datasheet fact.
Therm HIGH is set/cleared at conversion comparisons; with mode/routing/polarity
unchanged and no reset, shutdown/abort alone is not a documented clear path.
TMP116's comparison text omits the explicit "including averaging" phrase of
TMP117/TMP119; block-end comparison is an inference requiring separate evidence.
STOP-reset/ignore-until-START prose is explicit for TMP117 p.20/TMP119 p.23;
TMP116 documents timeout reset, not that exact stronger claim.

Authoritative documents:
https://www.ti.com/lit/ds/symlink/tmp116.pdf
https://www.ti.com/lit/ds/symlink/tmp117.pdf
https://www.ti.com/lit/ds/symlink/tmp119.pdf
https://www.ti.com/lit/ds/symlink/tmp118.pdf (excluded contrast)
https://www.ti.com/lit/ds/symlink/tmp114.pdf (excluded contrast)
