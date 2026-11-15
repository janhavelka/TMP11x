# Silicon family facts and retained source conflicts

This is a **seeded source map**, not the full checked ledger. Prompt 3 downloads
and pins the original exact-part PDFs; prompts 4–7 independently validate
all fields and ambiguities. The datasheet, with exact revision/footnotes,
prevails over this secondary note.
The [promptset audit](../reports/promptset-audit.md) records corrections to
these seeds. Page numbers here refer to the pinned manifest revisions.

## Included and excluded parts

| Selected BOM model | Register family | Variant statement |
|---|---|---|
| TMP116 / TMP116N | 116 | Same sensor programming model; accuracy specification differs. 64-bit EEPROM. |
| TMP117 / TMP117M / TMP117N | 117 | Same general programming model; grade/package/accuracy differ. 48-bit EEPROM, hardware temperature offset. |
| TMP119 | 117-like | Very close programming model to TMP117, higher accuracy, 48-bit EEPROM, hardware temperature offset. |
| TMP114 / TMP114N | excluded | Different register/CRC and low-voltage behavior. |
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

Sources: TMP116 SBOS740A printed p.24 (PDF page index 23);
TMP117 SNOSD82D printed p.25 (PDF index 24);
TMP119 SNIS236 printed p.32 (PDF index 31).
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
- Reads at EEPROM addresses access volatile shadows. Earlier locked writes
  could have changed those values without changing NVM. They do not prove an
  immutable factory serial or physical nonvolatile contents.
- TMP117 p.28 Table 7-6 and TMP119 p.35 Table 8-5 define a 2 ms per-device
  software reset. TMP116 has no documented equivalent CONFIG reset command;
  its general-call reset remains intentionally excluded.

Authoritative documents:
https://www.ti.com/lit/ds/symlink/tmp116.pdf
https://www.ti.com/lit/ds/symlink/tmp117.pdf
https://www.ti.com/lit/ds/symlink/tmp119.pdf
https://www.ti.com/lit/ds/symlink/tmp118.pdf (excluded contrast)
https://www.ti.com/lit/ds/symlink/tmp114.pdf (excluded contrast)
