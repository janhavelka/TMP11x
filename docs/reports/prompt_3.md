# Prompt 3 ? official reference intake

**Status: PARTIAL.** Prerequisites: [prompt 1](prompt_1.md), [prompt 2](prompt_2.md).
Input: `6c5e4496d5c896af689228d4bee4382a53c20626`; acquired 2026-11-15.

Completed: 14 original official TI PDFs, including all five required family
and exclusion datasheets. [Manifest](../reference/manifest.json) pins URLs,
revisions, retrieval date, byte sizes, SHA-256, notices and reviewed pages.
[Intake notes](../reference/extracted/intake-notes.md) link the visually reviewed
register/ID/status tables. PyMuPDF 1.27.2.2 rendered 12 reviewed pages; supporting
guides received cover metadata review, not full engineering qualification.

[Public-code catalog](../reference/public-code-catalog.md) pins Linux and
Adafruit revisions/licenses and records incompatible identity, polarity,
timeout and EEPROM-safety assumptions. No code from either was copied.

Remaining owned evidence: obtain and inspect a versioned TI generated/firmware
source and its license. ASC Studio timed out; TMP116 software is export-approval
gated. Access limits are retained rather than inventing license/commit data.
The inaccessible E2E DATA_READY thread is a separate stage 6 research lead.

Checks: 14 PDF hashes/signatures/sizes PASS; nine reference-tool regressions
PASS (including corruption and concurrent-file preservation); live isolated
restoration probe PASS; prompt structure/link checker and diff checks PASS.
See [preparation report](preparation.md) for commands and handoff rules.

Register/field ledger, timing proof, model-grade freeze and safety contracts
remain owned by stages 4?7. No MCU build, CI or physical test was run. Acquisition
is not a claim of implemented sensor support or closed silicon ambiguities.
