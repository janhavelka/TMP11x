# Source intake notes — 2026-11-15

These pointers refer to the exact PDFs hashed in [the manifest](../manifest.json).
Page numbers below are **one-based PDF pages** and match printed datasheet body
pages. Original register tables were rendered locally with PyMuPDF and visually
reviewed; the manifest lists the actual reviewed pages. Acquisition does not
close the independent field/timing/policy reviews in stages 4–7.

## TMP116 — SBOS740A

- [Page 24](../source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24), Table 3: `0x07` is EEPROM3;
  `0x08` is EEPROM4. The programmable-default footnote prevents treating factory
  values as immutable startup state.
- [Page 26](../source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26), Figure 32/Table 6: CONFIG bits 1:0
  are unused. TEMP and CONFIG reads clear DATA_READY; alert acknowledgement
  depends on operating mode. Do not port TMP117's bit-1 reset into this model.
- [Page 30](../source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=30), Table 13: independently confirms
  EEPROM3 at `0x07`.
- Stage 5 must reconcile Table 3's EEPROM default entries with the detailed
  EEPROM/traceability descriptions before defining diagnostic expectations.

## TMP117 — SNOSD82D

- [Page 25](../source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25), Table 7-3 and footnote:
  `0x07` is temperature offset; defaults can reflect EEPROM programming.
- [Pages 27–28](../source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27), Table 7-6: reads acknowledge
  DATA_READY; bit 1 is software reset and reads back zero. Figure 7-14 labels
  that bit `R-0`, while the detailed row says `R/W`; retain the distinction.
- [Page 28](../source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=28), Table 7-7: averaging changes the
  effective conversion cycle. This table alone does not establish a worst-case
  one-shot deadline.
- [Page 32](../source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=32), Table 7-15: upper ID bits describe
  revision; lower 12 bits identify `0x117`. EEPROM3's traceability warning must
  remain visible to diagnostic users.

## TMP119 — SNIS236

- [Page 32](../source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32), Table 8-2, lists ID reset `0x0117`.
  [Page 39](../source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=39), section 8.5.11/Table 8-14, lists
  `0x2117`, with revision in bits 15:12. Keep this inconsistency in the ledger;
  do not infer selected BOM model from the revision nibble.
- [Page 34](../source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34), Figure 8-7/Table 8-5: status reads
  can acknowledge readiness and alerts. The offset/EEPROM distinction matches
  the TMP117 map, not TMP116.
- [Page 7](../source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7) contains TMP119M accuracy rows. This
  intake does not add a supported grade: stage 4 must establish exact orderable
  variants and applicable programming model before making a support claim.

## Excluded families

- [TMP114 page 29](../source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=29), Table 8-4: CONFIG is at
  `0x03`, ALERT_STATUS at `0x02`, and device ID at `0x0B`; the TMP11x intended
  register model cannot be reused merely because the name is similar.
- [TMP118 page 26](../source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=26), Table 8-1: low/high limits
  occupy `0x02`/`0x03`, device ID is at `0x0B`, and unique ID at `0x0C..0x0E`.
  These PDFs are exclusion evidence, not promises of implementation.

## Supporting documents and open evidence

The application/EVM guides are downloaded and their cover metadata verified;
their full electrical/layout/firmware content has **not** been audited. Datasheets
remain authoritative for driver behavior. The SDAA311 PDF cover says June 2026;
the TI product-page listing showed March 9, 2026. The manifest records the PDF.

TI product pages and targeted TI errata/support searches were checked. No
separate applicable errata PDF was located in this intake; that is not proof
that no errata exist. A [TI E2E DATA_READY race discussion](https://e2e.ti.com/support/sensors-group/sensors/f/sensors-forum/1019457/tmp117-data_ready-flag-cleared-incorrectly-if-data-becomes-ready-during-read-of-configuration-register)
returned HTTP 403 during intake. Stage 6 subsequently reviewed the TMP117
capture. The reported CONFIG word conflicts with the stated pin routing.
The clear edge and applicability to TMP116 remain open. The linked TMP116
thread still returned HTTP 403 during audit 45.
[Stage 6 interpretation](../../device-behavior.md#temp-destructive-reads-and-sample-publication).

## Promptset audit additions

On 2026-11-15, the full promptset audit visually rechecked TMP116 PDF pages
17 and 29. Page 17 section 7.5.1.1 describes a cleared busy bit during reset
load; page 29 Table 10 and page 26 Table 6 describe it asserted. Preserve the
contradiction and do not use busy-clear alone as POR completion proof.

TMP117 page 18 and TMP119 page 20 describe four general-purpose EEPROM words,
while their detailed maps (pages 25 and 32) show three plus offset. The detailed
map governs exposed pointers; retain the inconsistent prose for source review.
Read values are volatile shadow observations, not independent NVM verification.
See the [per-prompt audit](../../reports/promptset-audit.md) for timing, command,
diagnostic and HIL corrections. The optional TI source-code access gaps above
do not block completed datasheet intake.
