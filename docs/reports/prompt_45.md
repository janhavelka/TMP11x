# Prompt 45 — audit of preparation and silicon evidence

## Phase a — scope and baseline, 2026-11-21

Audit 45 is open. Its question is whether stages 1–7 supply enough
source-backed evidence for stage 8 to freeze the API for the six selected
BOM models. The lead read the seven original prompts and checked their
deliverables against the eleven inputs declared by audit 45. All eleven
are available. Stage 8 consumes the completed `audit-1-7` report.
[Audit prompt](../prompts/45_audit-1-7.md), [sequence](../prompts/SEQUENCE.json),
[producer handoffs](../prompts/HANDOFFS.md).

The entry revision was `31879b6d759867e9f21a6bd707abd186a1bb24cc` on
clean `main`, equal to local `origin/main`. The prompt 7 acceptance commit
matches the wrapper request for its report, the index and the progress ledger.
The wrapper recorded upstream equality at 18:00:13 Prague time. The progress
ledger records stages 1–7 as COMPLETE and places 45 before 8. This report
was absent at entry.

## Inputs at the entry revision

| Stage and input | Artifacts | Question to verify in the audit |
| --- | --- | --- |
| [1](prompt_1.md) · `ecosystem-audit` | [Library comparison](../library-comparison.md), [adoption record](../source-adoption.md) | Can each ownership, lifecycle, trust and diagnostics decision be traced to a declaration, implementation, registered test or caller, and a TMP11x destination? Check licenses for actual imports. |
| [2](prompt_2.md) · `tooling-plan` | [Toolchain and package plan](../toolchain-and-package-plan.md), [managed launcher](../../scripts/pio.cmd) | Trace selected build, example, version, test and workflow rules to the pinned sources. Check the configured Core selection, SDK evidence and both promptset precedents. Account for the original 44 prompts and 11 subsequent audits. |
| [3](prompt_3.md) · `ti-archive` | [Manifest](../reference/manifest.json), [intake and page notes](../reference/extracted/intake-notes.md), [public-code catalog](../reference/public-code-catalog.md) | Match original PDF bytes, revisions, notices and page citations. Verify applicability in the rendered tables and footnotes. Keep the recorded optional TI code and license access gaps visible. |
| [4](prompt_4.md) · `variant-contract`, `board-guide` | [Variant contract](../variants.md), [board guide](../board-integration.md) | Check the exact-grade/package rows, electrical limits, ADD0, ID comparison and incompatible neighbors. Exercise the ambiguous TMP117/TMP119 identity examples against the two device-ID sections. |
| [5](prompt_5.md) · `register-ledger`, `feature-ledger`, `register-validator` | [Register ledger](../registers/tmp11x.json), [feature coverage](../feature-coverage.md), [validator](../../tools/check_register_ledger.py), [regressions](../../tools/test_check_register_ledger.py) | Compare each profile's registers, fields, access, reset domains and side effects to the original maps. Reconcile all 46 R/F/X IDs and owners. Check that malformed ledgers fail, including duplicate keys, overlapping fields and unsupported conflict resolutions. |
| [6](prompt_6.md) · `behavior-contract`, `behavior-vectors` | [Device behavior](../device-behavior.md), [literal vectors](../registers/behavior-vectors.json) | Recalculate signed words, all 32 cycle cells per family and the conditioned active bounds. Trace averaging, aborted blocks, CONFIG/TEMP reads, alert routing and freshness from their stated start events. |
| [7](prompt_7.md) · `safety-policy` | [EEPROM and reset policy](../eeprom-and-reset-policy.md), including nine research transcripts, and the refined register ledger | Follow each proposed mutation and fresh acquisition through startup proof and the operation-local `0x04` guard. Check the model split at `0x07`, ambiguous failures, reset and interference. Check X01–X08 across every proposed API, diagnostic and CLI route. |

Two parallel read-only inventory checks confirmed source availability.
The peer check read 51 pinned files across nine local sibling repositories.
The tooling template was available via GitHub at
`b574041c686f44821aa8828675f480576e5eff55`. Its workflow, generator,
package manifest, public header, agent guide and license were readable.
The silicon check opened all 14 archived PDFs and all 24 register-section
pages across the three supported families. Use these one-based page ranges
when checking each claim:

| Pinned source | Electrical, conversion, read effects and startup | Register tables, cycle table and identity |
| --- | --- | --- |
| [TMP116/N, SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24) | pp. 4–6, 11–18 | pp. 24–31, cycle Table 7 on p. 27 |
| [TMP117/M/N, SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25) | pp. 5–7, 13–19 | pp. 25–32, cycle Table 7-7 on p. 28 |
| [TMP119, SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32) | pp. 6–8, 15–23 | pp. 32–39, cycle Table 8-6 on p. 35 |

## Evidence still required

Phase b should record claim-level comparisons and discrepancies with a
source revision, page or code path, affected artifact and disposition.
Retain the TMP116 POR busy contradiction, uncertain CONFIG read-clear edge
and TEMP byte latching, offset/comparator ordering, and unresolved addressed
reset effects on EEPROM reload, EUN, companion bits and bus ACK.
The TMP117 DATA_READY capture has conflicting routing details. The related
TMP116 support thread was inaccessible in the recorded intake.

In phase c, reread prompts 1–7 and run two independent reviewers in
parallel. One reviews stages 1–3 for peer-source claims, tooling, provenance
and licensing. The other reviews stages 4–7 against the original silicon
tables, literal vectors, validator tests and forbidden transactions. The
lead verifies each finding, corrects confirmed gaps and repeats affected
checks. Phase d records final results and any remaining evidence gates.

Phase-a verification used Python 3.12.10 and Git 2.52.0.windows.1.
`python -B scripts/check_references.py` passed for 14 pinned PDFs,
`python -B tools/check_register_ledger.py` passed for six BOM models, and
`python -B scripts/check_promptset.py` passed for 55 stages. Report links
and Git whitespace were checked locally. Semantic review and final
acceptance remain open. Driver tests, MCU links, remote CI and physical
qualification were NOT RUN.
