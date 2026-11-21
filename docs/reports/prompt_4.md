# Prompt 4 — family identity and board integration

**Status: PARTIAL.** Phase c artifact review finished on 2026-11-21. Phase d
acceptance and closeout are next. The execution manifest places prompt 4 after
completed [prompt 3](prompt_3.md) and before prompt 5.

Phase a started on clean `main` at
`e4597abb69ae95b9a25ae4e31c22b473f8f48416`. The wrapper recorded the
linked source brief at `0432fe1ce6af4f5e98aa32d3e7e52f96603ed6b0`.
The verified history rewrite maps that commit to
`9bc78688bfff8782fbb0c7f20270482c26ea0324`. Phase b started on clean
`main` at `a28ecc60f3bfe73d0a7b4fc4f1f0c92c80a694d1`, equal to the
local `origin/main` tracking ref at entry. The wrapper owns fetch,
staging, commit and push.

## Inputs and deliverables

Consumed `ti-archive`. Verified all 14 local PDFs against the
[manifest](../reference/manifest.json) and checked the applicable original
tables. The [intake notes](../reference/extracted/intake-notes.md),
[exclusion inventory](../reference/extracted/excluded-families-audit.md) and
[public-code catalog](../reference/public-code-catalog.md) provide comparison
and provenance. Phase b owns `variant-contract` in `docs/variants.md`,
`board-guide` in `docs/board-integration.md`, and source/documentation evidence
in [FULL_COVERAGE](../prompts/FULL_COVERAGE.md). Both guide paths were absent
at entry. Prompts 5, 7, 8 and 13 consume the variant decisions for the register
ledger, safety, API and identity checks. Prompts 34 and 35 independently extend
the board guide, and prompt 42 finalizes user-facing integration guidance.

## Variant brief

- Keep the six BOM-selected models TMP116, TMP116N, TMP117, TMP117M, TMP117N
  and TMP119. Use one implementation with explicit identity mask/value,
  EEPROM word indices, offset support and addressed reset support. TMP116/N
  require the complete 16-bit word `0x1116` and have no defined revision nibble.
  Their four EEPROM shadow pointers are `0x05/06/07/08`, and CONFIG bits 1:0
  are unused. TMP117/M/N and TMP119 require `(word & 0x0FFF) == 0x0117`,
  retain bits 15:12 as revision and have three EEPROM
  shadow pointers `0x05/06/08`, optional managed offset at `0x07` and an
  addressed CONFIG bit-1 reset. Cite the maps and ID/configuration fields.
  [SBOS740A pp.24, 26, 31](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24),
  [SNOSD82D pp.25, 28, 32](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25),
  [SNIS236 pp.32, 35, 39](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32).
- Use a small decision table for invalid model/address (local rejection),
  transport failure, incompatible word, and compatible word with unknown
  exact part. Include TMP116 `0x0116` as a rejection and TMP117/TMP119
  `0x0117`, `0x2117` and another high nibble with the matching low 12 bits as
  compatible on either selected family. Include `0x0000` and `0xFFFF` as
  mismatches. TMP119 Table 8-2 gives `0x0117` while §8.5.11/Table 8-14 gives
  `0x2117`. A matching ID conveys compatibility with the selected BOM
  model. The BOM and physical inspection supply exact part and grade evidence.
  The pinned Linux catalog records full-word matching; an independently
  accessible Linux revision confirmed the switch and firmware-match fallback.
  The project's revision semantics come from TI. TMP1x2's explicit
  model/address selection is a convention precedent. The local peer was
  clean at `afd50889312f6e218a6a34eca687e3e8547d8b40`; its protocol and GPL
  Linux code were not adopted. [SNIS236 Table 8-14](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=39),
  [Linux comparison revision](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/1f7267d8043e394933b0ed06070bfa7921d7c830/drivers/iio/temperature/tmp117.c).
- For F13, record TI's manufacturing unique-ID/NIST traceability claims
  alongside the actual per-model shadow maps. Retain the four-word prose
  in SNOSD82D p.18 and SNIS236 pp.20, 37 alongside their three mapped words.
  Readable data reflect the current shadow. Factory provenance and direct
  physical NVM contents require independent evidence. Accept arbitrary words
  as observations and preserve them without any data write. Stages 5–7 and
  24 own detailed defaults, safety and the observation API. [SBOS740A p.17](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
  [SNOSD82D p.18](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18),
  [SNIS236 p.37](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=37).
- Keep TMP119M outside the six-model scope. SNIS236 p.7 lists accuracy
  limits ±0.08 °C at 0–45 °C and ±0.09 °C at −20–65 °C for TMP119M.
  The 9-Nov-2025 addendum at PDF p.45 lists only `TMP119AIYBGR` and its
  `.A` variant, both YBG DSBGA at −55–150 °C. This is pinned ordering
  evidence. A future scope change needs a verified ordering code/package,
  operating conditions and programming-model evidence. [SNIS236 p.7](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7),
  [package addendum](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=45).
- Exclude TMP114/N (different CONFIG, CRC/auto-increment, ID at `0x0B`),
  TMP118/M (reversed limits, different CONFIG and ID/revision layout at
  `0x0B`, UID retrieval requires data writes), TMP112 (12-/13-bit
  format and different CONFIG) and TMP1075 (12-bit LM75-style map, with
  a different device ID where present). TMP114 A–D and TMP118 A–D/MA–MD
  overlap at `0x48–0x4B`. TMP114 ND/NC/NB/NA use `0x4C/4D/4E/4F`.
  TMP112 address-only X2SON uses `0x40–0x43`, while the fixed ALERT
  X2SON and SOT563 grades use `0x48–0x4B`. TMP1075 permits `0x40–0x5F`
  and TMP1075N `0x48–0x4B`. Require unique resolved addresses and
  compatible electrical/timing conditions on every shared segment. End
  the identity check on mismatch or error. Foreign UID, CONFIG and CRC
  probing are outside the supported transaction. [SNIS214E pp.3–4, 21–29, 38](../reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=3),
  [SNIS237A pp.18–19, 23, 26–30](../reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=23),
  [TMP112 SBOS473L pp.14, 16–23](https://www.ti.com/lit/ds/symlink/tmp112.pdf#page=16),
  [TMP1075 SBOS854F pp.14–15, 21–24](https://www.ti.com/lit/ds/symlink/tmp1075.pdf#page=14).

## Board guide brief

- Build the BOM matrix from the pinned package addenda (SBOS740A PDF p.43,
  SNOSD82D p.46, SNIS236 p.45). TMP116/N use DRV WSON, all three TMP117 grades
  list DRV WSON and YBG DSBGA, and TMP119 uses YBG DSBGA. List actual
  ordering-code examples and operating temperatures for each. TMP116/N
  recommended free-air range extends to +150 °C, while the addendum and
  electrical-table span end at +125 °C. TMP117M addendum range is 0–85 °C.
  Keep recommended conditions, dated orderable range and guaranteed
  accuracy rows separate from the signed code range −256 to +255.9921875 °C
  at 1/128 °C per LSB. Account for the initial `0x8000` sentinel.
  [SBOS740A pp.4–6, 43](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=43),
  [SNOSD82D pp.5–6, 46](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=46),
  [SNIS236 pp.6–7, 45](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=45).
- Populate guaranteed accuracy rows using exact-part electrical tables.
  TMP116/N use 1.9–5.5 V with narrower supply conditions for tighter
  limits. Include the ±0.75 °C footnote from −55 to −40 °C and the oil-bath
  condition. TMP117 grades and TMP119 use 1.7–5.5 V from −55 to +70 °C,
  and 1.8–5.5 V from −55 to +150 °C, subject to the exact orderable's
  range. Accuracy tests use eight averages and a 1 Hz conversion cycle
  for all three families. TMP117 and TMP119 further specify I2C inputs
  at or below 0.05 × V+ for LOW and at or above 0.95 × V+ for HIGH.
  These are tighter than the ordinary 0.3/0.7 × V+ logic limits.
  TMP117's unsoldered-pad test condition applies to the DRV package across
  grades. Use exact-part, condition-specific current rows wherever quoting
  current. [SBOS740A p.5](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5),
  [SNOSD82D p.6](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6),
  [SNIS236 p.7](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7).
- ADD0 straps GND/V+/SDA/SCL select seven-bit addresses
  `0x48/0x49/0x4A/0x4B` respectively. For DRV, pins 1–6 are SCL, GND,
  ALERT, ADD0, V+, SDA. For YBG, A1/A2/B1/B2/C1/C2 are SDA, SCL, V+, GND,
  ADD0, ALERT. Distinguish open-drain SDA and ALERT from the SCL input.
  The typical circuit uses 5 kΩ pulls and a local 100 nF ceramic bypass.
  Size the actual pulls for total bus capacitance, rise time, sink current,
  pullup rail and thresholds. TMP114 requires 1.08–1.98 V supply and
  recommended SCL/SDA ≤1.98 V (2.1 V absolute maximum). A direct 3.3 V
  bus is unsafe. Low-voltage or shifted coexistence needs a valid supply
  and HIGH/LOW levels for each participant. Host mismatch fixtures convey
  software evidence only. [SBOS740A pp.3, 20, 32](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=32),
  [SNOSD82D pp.4, 21, 33](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=33),
  [SNIS236 pp.5, 23, 28](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=28),
  [SNIS214E p.4](../reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=4).
- TMP116 p.3 and Figure 41 show a pulled-up ALERT when used. Preserve the
  conflict with the unqualified grounding sentence on p.32; Figure 44
  addresses the power filter and omits signal connections. TMP117 and
  TMP119 explicitly discuss unused ALERT. For DRV packages, record TI's
  unsoldered-pad precision guidance and grounded or electrically floating
  pad treatment if soldered. YBG uses six solder balls and separate layout
  guidance. Describe strain, thermal path to the object or air, pullup and
  traffic heating, and self-heating under conversion load. Board identity,
  wiring, route and measurement setup remain unconfirmed. [SBOS740A pp.32, 34–37](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=36),
  [SNOSD82D pp.33–36](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=35),
  [SNIS236 pp.28–31](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=30).

## Phase a verification and remaining evidence

- PASS — `python scripts/check_references.py`, Python 3.12.10, verified
  paths, sizes and SHA-256 for 14 PDFs.
- PASS — `python scripts/check_promptset.py` validated the 55-stage sequence,
  handoffs, coverage ownership and links. A separate Python link check
  resolved 30 local report links and checked PDF page bounds against the
  manifest. `git diff --check` and the new-file whitespace check passed.
- PASS — inspected original PDF text for orderable, map, ID, pin, address
  and electrical claims. PyMuPDF 1.27.2.2 rendered the TMP116 Figure 41
  circuit and merged TMP117/TMP119 accuracy test-condition columns on
  pp.32, 6 and 7 respectively for visual review.
- The exact pinned Linux file was unavailable through the browser lookup.
  The checked catalog and an accessible independent Linux revision provided
  secondary corroboration. This does not gate TI-source documentation.

## Phase b deliverables and checks

- [Variant contract](../variants.md) names all six BOM models, their two
  register maps, capability gates, EEPROM shadows, observed ID and revision
  rules, and incompatible neighbors. Named decision cases include invalid
  selection, incomplete transport, undocumented TMP116 IDs, shared
  TMP117/TMP119 revision examples, and complete `0x0000` or `0xFFFF`
  mismatches. TI governs the rule. The clean local TMP1x2
  `include/TMP1x2/Config.h` at
  `c91bac63313ea725ce174ab98abc50705c365639` was checked for the explicit
  model/address convention. No peer or Linux code was copied.
- [Board guide](../board-integration.md) records six orderable model rows,
  both pinouts, the ADD0 table, all supported accuracy rows, and their supply,
  temperature and assembly conditions. It preserves the TMP116 orderable
  versus recommended temperature conflict, the TMP116 ALERT conflict and
  the TMP117M orderable limits. Ordering examples come from pinned addenda.
  Current availability and physical fixture details remain unverified.
- [Coverage evidence](../prompts/FULL_COVERAGE.md#prompt-4-source-and-documentation-evidence)
  connects F01, F02, F13, F16 and F17 to the documented layer, with source
  evidence for the identity and exclusion rows R17, X06 and X07. Later
  ledger, API, adapter and physical owners retain their remaining checks.
- PASS — inspected exact TI PDFs with PyMuPDF 1.27.2.2. Rendered SBOS740A
  pp.5, 32, 36, SNOSD82D p.6 and SNIS236 p.7 to verify merged accuracy
  conditions, note markers and the two TMP116 figures. Checked the
  TMP112 SBOS473L and TMP1075 SBOS854F vendor documents for neighbor
  addresses and protocol differences. The stage 3 Linux revision remained
  inaccessible during this pass. The catalog and accessible revision
  `1f7267d8043e394933b0ed06070bfa7921d7c830` supply comparison evidence.
- PASS — Python 3.12.10: `python -B scripts/check_references.py` verified
  14 pinned PDFs, and `python -B scripts/test_check_references.py` passed
  9 tests. `python -B scripts/check_promptset.py` verified all 55 stages,
  coverage ownership, local links and PDF anchors.
  `python -B scripts/test_check_promptset.py` passed 36 tests.
  Focused checks resolved 121 local links including heading and PDF
  page anchors, matched all nine orderable examples to their package
  and temperature ranges, and counted 24 supported accuracy rows.
  `git diff --check` and whitespace checks on the new guides passed
  with Git 2.52.0.windows.1.
- NOT RUN — native implementation tests, MCU links, remote CI and physical
  fixture qualification. No workflow is tracked at the phase b baseline.
  Remote CI is deferred to stage 43. Phase c reviews the guides and source
  evidence. Phase d owns acceptance, report index and progress updates.

## Phase c artifact review

Reviewed `54e48e370023fe7f015960e971ebd2e40555ff37` and its phase b diff.
`main` was clean and equal to the local `origin/main` tracking ref on entry.
The wrapper's `runs.log` records a successful commit and matching remote
verification at that SHA on 2026-11-21. The clean local TMP1x2 checkout was
at `c91bac63313ea725ce174ab98abc50705c365639`. Its `Config.h` blob
`e7dfdf5ff836fa35fecea7eb7a0f48bd83a8d288` matched the GitHub `main`
file and corroborates explicit model and address selection.

- Two independent read-only reviewers checked identity and exclusions, and
  package and board conditions. The lead rechecked the findings against the
  original TI pages and checked the F01/F02/F13/F16/F17 coverage links and
  the R17/X06/X07 boundaries. The six capability rows and ID decisions match
  the sources. Reviewed examples include invalid selection, a short read,
  undocumented TMP116 words, shared TMP117/TMP119 revision words and
  complete `0x0000` or `0xFFFF` responses. The identity guide now states the
  repeated-START transfer required by the owner contract. The four required
  excluded-family rows agree with their vendor sources.
- Added the TMP117 EEPROM1 citation for the four-word prose and linked the
  three exact register maps at the shadow table. The guides retain the
  TMP119 reset-ID conflict and the limit on factory provenance. The GitHub
  connector returned the exact catalog-pinned Linux revision
  `23c62304dde29abe92013d85910d03a77dada35d` and expected blob
  `2d0e79416434bec0c03469ce71094882952098df`. It confirmed the full-word
  comparisons and firmware/device match fallback. The variant guide now
  links that revision. [SNOSD82D p.30](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=30),
  [Linux source](https://github.com/torvalds/linux/blob/23c62304dde29abe92013d85910d03a77dada35d/drivers/iio/temperature/tmp117.c).
- Added the recommended 0–5.5 V I/O range for TMP117 grades and TMP119,
  identified the TMP116/N absolute maximum pin ratings as stress limits,
  and gave the verified 1–400 kHz clock range. Linked the TMP119 guidance
  on I²C traffic during conversion directly. The nine orderable examples,
  24 accuracy rows, both pinouts and all ADD0 straps match the pinned PDFs.
  The TMP116 ALERT and temperature-range conflicts and TMP117M's orderable
  limits remain explicit. [Board guide](../board-integration.md).
- PASS — Python 3.12.10 with `-B` ran `scripts/check_references.py`
  (14 PDFs), `scripts/test_check_references.py` (9 tests),
  `scripts/check_promptset.py` (55 stages) and
  `scripts/test_check_promptset.py` (36 tests). A focused Python check
  with PyMuPDF 1.27.2.2 verified six model/capability rows, nine orderable
  code/package/range matches, 24 accuracy rows, the added I/O, clock and
  EEPROM source details, and the local links, heading anchors and PDF
  pages in the four reviewed files. `git diff --check` passed with
  Git 2.52.0.windows.1.
- NOT RUN — native tests, MCU links, remote CI and physical qualification.
  The fixture details remain unconfirmed. Stage 43 owns remote CI.
