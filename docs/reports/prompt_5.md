# Prompt 5 — Register and bitfield ledger

**Phase a assessed on 2026-11-21. Phase b is next.** Entry was clean
`main` at `1707f3fd36eabf2c74f76cb4e7ffc5034e465b6b`, equal to the
local `origin/main`. Git history and the wrapper log confirm that prompt 4's
acceptance commit reached upstream. The [sequence](../prompts/SEQUENCE.json)
selects prompt 5 after completed prompts 3 and 4. Its four output files were
absent at entry.

## Verified inputs

The `ti-archive` from [prompt 3](prompt_3.md) supplies the original PDFs,
revisions and hashes. The `variant-contract` from [prompt 4](prompt_4.md)
supplies six BOM selections. TMP116/N use four EEPROM words and the full
`0x1116` device ID. TMP117/M/N and TMP119 use three EEPROM words, an offset
register and a low-12-bit device ID of `0x117`. Their upper four bits report
revision. TMP119M and the incompatible TMP114/TMP118 families stay outside
the six profiles. [Archive manifest](../reference/manifest.json),
[variant contract](../variants.md), [board guide](../board-integration.md).

| Selected models and pinned PDF | Register map through device ID | Other primary pages needed |
|---|---|---|
| [TMP116/N, SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24) | pp.24–31, Tables 3–15. CONFIG Table 6, EEPROM unlock Table 10, full-word ID Table 15. | pp.4–6 electrical/timing, p.17 startup, pp.19–22 pointer, ADD0 and word transfers. |
| [TMP117/M/N, SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25) | pp.25–32, Tables 7-3–7-15. CONFIG Table 7-6 spans two pages, unlock Table 7-10, ID Table 7-15. | pp.5–7 electrical/timing, p.18 startup, pp.20–23 serial interface and word diagrams. |
| [TMP119, SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32) | pp.32–39, Tables 8-2–8-14. CONFIG Table 8-5 spans two pages, unlock Table 8-9, ID Table 8-14. | pp.6–9 electrical/timing, p.20 startup, pp.22–25 serial interface and word diagrams. |

The two clean sibling checkouts match GitHub `main` at the revisions below.
Their `CommandTable.h` files match the preparation-pinned blobs. TMP1x2
separates command bits from persistent bits, but places TLOW at `0x02`,
where TMP11x places THIGH. OPT4001 illustrates naming for masks and status
fields. Derive every TMP11x numeric literal independently from TI. The
catalog's Linux and Adafruit comparisons cover offset gating, full-word ID
comparisons and a polarity discrepancy. TI's exact-part field tables govern
those decisions. [TMP1x2 header](https://github.com/janhavelka/TMP1x2/blob/c91bac63313ea725ce174ab98abc50705c365639/include/TMP1x2/CommandTable.h),
[OPT4001 header](https://github.com/janhavelka/OPT4001/blob/f17d4944640145773d3331e155a5bc728fcc4c66/include/OPT4001/CommandTable.h),
[pinned public-code catalog](../reference/public-code-catalog.md).

## Phase b deliverables and acceptance cases

- **Register ledger** (`docs/registers/tmp11x.json`). For each selected
  model, cover all ten mapped 16-bit registers, every field and encoding,
  reserved pointers `0x09–0x0E` and invalid upper pointer bits. Carry the
  literal TI access type, field mask, factory value, reset/NVM domain, read
  and write effects, prerequisites, release policy, source page and stage
  owner. Keep signed TEMP and initial conversion validity distinct. On
  TMP116/N, `0x07` is EEPROM3 and CONFIG[1:0] is reserved. On the other
  four models, `0x07` is OFFSET and bit 1 is an explicit write-one reset
  command that reads zero.
- **Field and policy checks.** HIGH, LOW and DATA_READY have access code
  `R`. CONFIG or TEMP reads consume DATA_READY. CONFIG reads acknowledge
  high/low flags in alert mode. In therm mode HIGH follows hysteresis and
  LOW stays zero. Encode all MOD/CONV/AVG values, therm/alert, polarity and
  pin routing. MOD=`10` selects continuous and reads back `00`. MOD1[11]
  cannot be stored in EEPROM. MOD0 and bits 9:2 can. THIGH/TLOW factory
  words are `0x6000/0x8000`. Supported OFFSET has a factory value of zero.
  Previously programmed values can load on power-up. EUN[15] and busy[14]
  share pointer `0x04`. Its busy field mirrors CONFIG[12]. Require startup
  readiness and a locked, idle EEPROM before allowed volatile writes.
  Deny unlock, relock and EEPROM data writes, including writes to shadows.
- **Feature ledger** (`docs/feature-coverage.md`). Carry the 18 R, 20 F
  and 8 X base IDs in the first Markdown table column, with per-model
  dispositions, sources, implementation and test owners, CLI route or
  reason for exclusion, and separate documented, implemented, host-tested,
  MCU-linked, remote-CI and physically-qualified evidence. Include the four
  ADD0 straps, retained pointer, explicit pointer plus one MSB-first word,
  repeated START and final NACK. No auto-increment is documented. Record
  the 1–400 kHz SCL range and 20–40 ms interface timeout (35 ms typical)
  under the stated line conditions. The application and adapter own bus
  settings and deadlines. [Required coverage rows](../prompts/FULL_COVERAGE.md).
- **Offline validator and regressions** (`tools/check_register_ledger.py`,
  `tools/test_check_register_ledger.py`). Exercise a valid ledger with
  different meanings at `0x07` across models. Require nonzero failure for
  duplicate JSON keys at parse time, duplicate addresses or overlapping
  fields within one profile, out-of-width masks, missing provenance,
  incompatible model semantics and a conflict marked resolved without
  evidence. Check invalid pointer inputs without masking them into range.
  Run both files with Python and run `python scripts/check_promptset.py`.
  Compare masks and encodings directly to the PDFs during review.

## Conflicts to retain in the ledger

| Evidence pair | Bounded policy |
|---|---|
| TMP116 p.17 describes busy as clear during startup. Tables 6 and 10 on pp.26, 29 describe it as busy. | A clear busy bit alone cannot establish readiness. TMP116 describes all writes as ignored during load. TMP117 p.18 and TMP119 p.20 name CONFIG and limits. Leave OFFSET response during that interval unknown and forbid the write. |
| TMP116 p.24 lists zero for all EEPROM words. Detailed pp.29–31 give unknown for 1, 2 and 4, and zero for 3. TMP117 p.31 and TMP119 p.38 label EEPROM2 reset zero while their field tables and maps give unknown. | Accept arbitrary observed shadow values. They establish neither factory identity nor direct NVM contents. |
| TMP117 pp.18, 20 (Fig. 7-8), 30 and TMP119 pp.20, 22 (Fig. 7-9), 37 describe four words. Their maps on pp.25 and 32 expose three plus OFFSET. Startup stored-field lists omit DR/Alert and OFFSET, which the map and CONFIG footnotes mark EEPROM-backed (TMP117 pp.25, 27–28 and TMP119 pp.32, 34–35). | Use the three mapped shadow pointers and retain prior programmed offset and pin routing as possible startup values. |
| TMP119 p.32 gives ID reset `0x0117`, while p.39 gives `0x2117`. TMP117 p.27 and TMP119 p.34 draw Soft_Reset as read-zero while their detailed tables on pp.28 and 35 specify a writable command. | Retain both ID words and use the documented DID field for compatibility. Keep reset out of ordinary settings and retain unknown addressed-reset side effects. |
| TMP116 p.4 recommended range versus pp.5, 10, 43 electrical and orderable range. TMP117 p.14 and TMP119 p.17 shutdown-current prose versus their electrical rows on pp.6 and 8. TMP119 p.4 portfolio summary versus pp.7–8 accuracy/current and TMP116 p.4 supply. | Use exact-grade electrical conditions, retain paired citations and distinguish typical from guaranteed values. A characterization plot at 1 MHz on TMP119 p.11 does not extend its specified bus range. |

Use each model's own protocol evidence. TMP117 p.20 and TMP119 p.23 explicitly
reset the I²C state machine on STOP. TMP116 documents the interface timeout
on p.21. Track remaining register and non-register discrepancies from the
three audits in the same ledger. [TMP116 audit](../reference/extracted/tmp116-coverage-audit.md),
[TMP117 audit](../reference/extracted/tmp117-coverage-audit.md),
[TMP119 audit](../reference/extracted/tmp119-coverage-audit.md).

## Verification and handoff

Python 3.12.10 ran `python -B scripts/check_references.py` (14 PDF paths,
sizes and SHA-256 hashes). A focused input check verified the two consumed
IDs, three produced IDs, six model rows, 46 base coverage IDs and absence
of the planned files at entry. PyMuPDF 1.27.2.2 provided direct inspection of
the pinned map, CONFIG, unlock, ID, startup and serial pages.
`python -B scripts/check_promptset.py` passed for all 55 stages. Local
report links and PDF page anchors resolved. Git 2.52.0.windows.1 reported
no whitespace errors in the working diff or the new report.

Phase b can proceed with the four files above. Stages 6 and 7 refine the
same ledger with timing, behavior and safety evidence. Stage 10 registers
both Python commands in the host entrypoint. Stage 12 owns the reviewed
C++ constants. Ledger validation and its regressions await implementation.
Native driver tests, MCU links and physical evidence remain open. Stage 43
owns the remote CI checkpoint. [Producer and consumer handoffs](../prompts/HANDOFFS.md).
