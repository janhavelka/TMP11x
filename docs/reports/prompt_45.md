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

## Phase b — engineering evidence, 2026-11-21

The audit started from clean `main` at
`3bb582587dbd136a81babfc488d5e35a8f93a293`. Git history shows the
phase-a report as that commit's only path. The wrapper recorded upstream
equality at 18:15:29 Prague time. Two read-only agents checked stages 1–3
and 4–5 against the pinned sources. The lead checked the reported corrections,
recalculated the behavior vectors and traced the safety policy. Phase c
still requires its own fresh parallel review.

### Peer code, tooling and archive

| Stage | Source comparison and disposition |
| --- | --- |
| 1 | At TMP1x2 `afd5088`, `src/TMP1x2.cpp` counts tracked successes and selected failure codes and permits reads while OFFLINE. The registered `helpers` case `trackedWordObservations` contrasts raw and tracked reads. The `variants` assertion covers a bound probe. Corrected the test attribution and the ADS1115 `e07622a` helper name to `_markHardwareConfigDirtyIfClean` in the [comparison](../library-comparison.md). That helper preserves an earlier dirty fault. OPT4001 `441ef57` supplies destructive-read bookkeeping, TCA9548A `4a244ad` supplies an example of recovery that disables mux routes, and MB85RC `3ace168` supplies retained results. The FieldCore caller on `rodtester` at `c40ea83` clips the callback budget. The checked TMP1x2 MIT notice matches the local license. |
| 2 | TMP1x2 `afd5088` registers eight native C++ tests in `CMakeLists.txt`. Its CI pins Core 6.1.19 and IDF 5.3.2/5.5.1/6.0.1. Its generator embeds the project name. The 24Cxx `1bd9f96` CMake test runs a Python transcript against its CLI executable. The template at `b574041` has an unpinned PlatformIO install and no root CMake file. MotorControl-CAN `60bcefe` and DP8382x `87aafc0` support the sequence and evidence-layer conventions. The current TMP11x manifest keeps the original 44 stages and inserts 11 audits. The [toolchain plan](../toolchain-and-package-plan.md) distinguishes the installed Core 6.2.0 at `C:\pio` from the peer CI pin. SDK discovery confirmed S2/S3/P4 target metadata. The peer working-tree edits in MotorControl-CAN were excluded by reading commit objects. |
| 3 | All 14 original PDFs matched the manifest hashes, cover revisions and page counts. The Linux and Adafruit source blobs match the GPL-2.0-only and BSD-3-Clause comparisons in the [catalog](../reference/public-code-catalog.md). Added TI's SAIL 1.50.00.00 TMP116 API documentation, which describes a blocking open and a close that disables conversions. Its linked license manifest was unavailable. Updated the [intake notes](../reference/extracted/intake-notes.md) to point from the original HTTP 403 to the subsequently reviewed TMP117 capture. Thread 1019457 reports a missed indication and gives conflicting DR/Alert settings. The linked TMP116 thread 822036 still returned HTTP 403. |

### Silicon and coverage

| Stage | Source comparison and disposition |
| --- | --- |
| 4 | Compared the 24 conditioned accuracy rows and nine selected model/package combinations against SBOS740A pp.5,43, SNOSD82D pp.6,46 and SNIS236 pp.7,45. TMP117M's orderable range is 0–85 °C. The TMP119M electrical rows lack a corresponding orderable in the pinned addendum. Across all 65,536 possible identity words, the TMP116/N rule admits only `0x1116`. Each TMP117 grade and TMP119 admits the same 16 words ending in `0x117`. The TMP114 pin/interface evidence and TMP118 register map support their exclusion. [Variant contract](../variants.md), [board conditions](../board-integration.md). |
| 5 | Compared 30 register addresses/access codes and 70 field masks/access codes with the three map-to-ID page ranges listed above. The stored CONFIG subset is `0x07FC`. Bit 11 is volatile. Bit 1 is an addressed command on 117/119 and reserved on 116. Added the TMP119 supply-summary discrepancy to the [ledger](../registers/tmp11x.json). SNIS236 pp.4,30 give an unqualified 1.7 V lower bound. Pages 6–7 require at least 1.8 V above +70 °C through +150 °C. The board guide already gave the conditioned values. The ledger now retains 32 open source records. Compared all 18 R, 20 F and eight X rows with FULL_COVERAGE and restored omitted forward owner references in R12, R16, F04, F05, F11 and F18. [Coverage matrix](../feature-coverage.md). |
| 6 | Parsed each original CONV × AVG table and matched all 96 cells to both the ledger and [vectors](../registers/behavior-vectors.json), including all 32 TMP116 standby footnotes. Independently multiplied the electrical rows by 1/8/32/64. At 64 conversions the derived upper intervals are 1088 ms on 116 and 1120 ms on 117/119. The 117 electrical row is for one-shot and the 119 row for no internal averaging. The ledger validator previously accepted substituted limits or an incorrect AVG source page. It now checks each family's electrical row and page and requires a nonempty timing condition. A new negative test reproduces each defect. |
| 7 | Compared 24Cxx `0bb27f4:docs/integration.md` on indeterminate writes and TMP1x2 `c91bac6:src/BusOperations.cpp` on general call and ARA against the planned TMP11x paths. SBOS740A Fig.23, SNOSD82D Fig.7-5 and SNIS236 Fig.7-6 all show the winning threshold-alert pin deasserting while LOW stays latched until an addressed CONFIG read. Captured that sequence in the [behavior contract](../device-behavior.md#comparator-and-pin-evidence), literal vector, X03 and [safety policy](../eeprom-and-reset-policy.md#external-interference). DATA_READY routing and the direction bit when both thresholds are latched remain open. |

### Safety transcript walkthrough

The nine rows are planned transfer sequences. The audit checked permitted
selectors, forbidden callbacks and evidence transitions against the policy
and the pinned maps.

| Transcript cases | Result |
| --- | --- |
| Identity and shadow indices / protected memory and raw escape | TMP116/N permits reads at `0x05/0x06/0x07/0x08`. The other four selections use `0x05/0x06/0x08` and reserve `0x07` for opt-in offset. EEPROM_UL and all data shadows deny data writes. Invalid indices, unsupported features and mismatched identity stop dependent access. |
| Startup / busy or guard failure / unlocked | A compatible ACK at 50.9 ms on the stated floor clock admits the guard from tick 54. Test EUN and BUSY together at `0x04`. A timely locked/busy word permits only paced guard rereads on a timed path. A failed callback ends the attempt. CONFIG BUSY invalidates a previous guard. An untimed attempt stops. Either EUN-set word refuses all writes and fresh publication until external correction. |
| Supported reset / recovery | Verified stable words `0x0220` and `0x0620` encode a single reset write of `0x0222` and `0x0622`, respectively. Invalidate before attempting it. Completion at 50.9 ms permits postwait observation from tick 53. Recheck identity and safe state. Recovery uses renewed startup evidence and verifies both limits and any managed offset. The transcript literals decode to +37, −10 and +0.5 °C. An ambiguous write leaves trust dirty. |
| Interference after the guard / shared bus exclusions | Reset, a power cycle or an outside write invalidates the evidence used by the dependent operation. External ARA can change the pin while a comparator latch remains available to CONFIG. Checked X01–X08 across the proposed typed, diagnostic, cooperative and CLI routes. No allowed path sends a general call, ARA, EEPROM data write or relock. The bus-silent lifecycle row now requires zero callbacks, including reads. |

### Focused checks

Python 3.12.10 and PyMuPDF 1.27.2.2 reproduced the 96 nominal cells,
36 active-time values, 11 signed words, 14 host rounding/rejection cases,
three offset clamps, eight pin routes, five poll schedules and three clock
origins. The two eight-conversion means are 7 and 27 counts. Aborting the
third block retains 27. The comparator walkthrough checked equality,
acknowledgement, re-arm and therm hysteresis. The external ARA case retains
LOW with the pin high and records zero driver ARA transfers.

The commands `python -B tools/check_register_ledger.py` and
`python -B tools/test_check_register_ledger.py` passed for six models and
30 tests. Under `python -B`, `scripts/check_references.py` verified the
14 PDFs and `scripts/test_check_references.py` passed nine tests.
`scripts/check_promptset.py` verified 55 stages and
`scripts/test_check_promptset.py` passed 36 tests. All 148 local links
in the changed documents resolved, including 77 PDF pages and nine
Markdown section anchors. `git diff --check` passed.
The startup BUSY contradiction, read-clear edge, TEMP byte latching,
offset/comparator ordering and addressed-reset effects listed in phase a
remain open. Native driver execution, MCU links, remote CI and physical
qualification were NOT RUN.

## Phase c — independent review, 2026-11-21

The lead reread prompts 1–7 in full. The review started on clean `main`
at `4ea6744fd78999658196d27af4bd37e5b06b3013`. Its eleven changed paths
matched the phase-b commit request. The wrapper recorded upstream equality
at 18:38:33 Prague time. Two read-only reviewers worked in parallel on that
revision and returned their findings before the handoff.

| Finding | Lead verification and correction |
| --- | --- |
| Rejected replacement binding | At TMP1x2 `afd5088:src/TMP1x2.cpp` lines 19–36, `bind` calls `unbind()` before validating the replacement. The comparison now identifies the ordering required by TMP11x stage 14 to preserve the prior binding and observations on rejection. [Lifecycle contract](../prompts/14_binding-lifecycle.md). |
| Health during startup | In the same file, `healthFailure` selects the failure codes to count. Lines 120–136 gate the READY/DEGRADED/OFFLINE transitions on initialization. Stage 15 counts each tracked physical outcome and updates health during startup and apply. Corrected the peer attribution in the comparison and the phase-b summary. [Peer comparison](../library-comparison.md), [health contract](../prompts/15_passive-health.md). |
| Unused bits on reads | SBOS740A pp.26,29, SNOSD82D pp.28,30 and SNIS236 pp.35,37 list R access and reset value 0 for the unused CONFIG and EEPROM_UL fields. Corrected the readback guarantee in all six ledger entries, the three source inventories and R13. CONFIG writes still encode zero in reserved positions. The guard masks with `0xC000`. Synthetic words `0x3FFF` and `0x0000` therefore give the same locked/idle decision. [Ledger](../registers/tmp11x.json), [safety transcripts](../eeprom-and-reset-policy.md#transcript-vectors-for-implementation). |
| Addressed-reset encodings | SNOSD82D p.28 Table 7-6 and SNIS236 p.35 Table 8-5 define bit 1 as write-one to reset. Before the fix, deleting the encoding map or exchanging its zero/one meanings passed validation on both profiles. The validator now rejects all four mutations. The TMP116 profile continues to exclude the command. [Validator](../../tools/check_register_ledger.py), [regression cases](../../tools/test_check_register_ledger.py). |

The stages 1–3 reviewer checked pinned peer code, registered callers, build
rules, the managed PlatformIO 6.2.0 installation, PDF cover revisions and the
Linux and Adafruit license records. The stages 4–7 reviewer matched 30 register
map entries, 96 nominal cycle cells and all 32 TMP116 standby footnotes, and
traced the EEPROM and reset exclusions. The lead also recalculated 36 active
times, 11 signed words, 14 host quantization cases, three offset cases and
eight pin routes. An exhaustive check of the guard masks covered all 65,536
input words.

Python 3.12.10 with `-B` passed `scripts/check_references.py` (14 PDFs),
`tools/check_register_ledger.py` (six models) and `scripts/check_promptset.py`
(55 stages). Their regression scripts passed 9, 31 and 36 tests respectively.
`git diff --check` passed with Git 2.52.0.windows.1.

Phase d owns final acceptance and the completion record. TMP116 startup BUSY,
the CONFIG clear edge, TEMP byte latching, offset/comparator ordering and
addressed-reset side effects retain their evidence gates. The optional TI
license manifest and linked TMP116 discussion remain unavailable. Native
driver execution, MCU links, remote CI and physical qualification were NOT RUN.

## Phase d acceptance — 2026-11-21

**COMPLETE.** The audit produces `audit-1-7` for stage 8. All eleven
inputs from stages 1–7 were present and matched their producer reports and
handoff entries. The two independent reviews are recorded above. All four
confirmed findings were corrected, and no audit finding remains open.

Acceptance started on clean `main` at
`f80231ce882bed6036402f98809a27398ab548e2`, equal to local `origin/main`.
The commit message, parent and ten changed paths match the phase-c baseline
and wrapper request. The wrapper logged upstream equality at 18:48:40
Prague time. The wrapper owns staging, commit and upstream verification
for this acceptance update.

| Acceptance check | Result |
| --- | --- |
| `python -B scripts/check_promptset.py` and `python -B scripts/test_check_promptset.py` | PASS. 55 stages and 36 regressions. Stage 8 follows audit 45 and consumes its report. |
| `python -B scripts/check_references.py` and `python -B scripts/test_check_references.py` | PASS. 14 pinned PDF paths, sizes and hashes, plus nine regressions. Opened all 14 originals and confirmed the manifest page counts. |
| `python -B tools/check_register_ledger.py` and `python -B tools/test_check_register_ledger.py` | PASS. Six BOM selections and 31 regressions, including missing and reversed addressed-reset encodings. |
| Peer and toolchain spot checks | PASS. Rechecked the pinned TMP1x2 bind order and health gate, eight C++ test registrations, CI pins and the identical MIT license blob. `scripts/pio.cmd --version` selected Core 6.2.0 at `C:\pio`. |
| Inline `python -B -` source and vector checks | PASS. Matched 96 cycle cells and 32 TMP116 standby footnotes to the original PDFs. Recalculated 36 active times, 11 signed words, 14 host quantization cases, three offset clamps, eight pin routes and two averaging blocks with an abort. Exhaustively checked each identity mask against its cited field table. |
| Inline `python -B -` safety checks | PASS. Checked all 65,536 guard inputs, nine safety transcripts, eight exclusions, 46 coverage IDs, shadow and reset gates, reset command words and both wait boundaries. |
| Completion records | PASS. All 100 local links in the report, report index and progress ledger resolved, including the cited PDF pages and Markdown anchors. |
| Git whitespace | PASS. `git diff --check 31879b6 HEAD`, `git diff --check` and `git diff --cached --check`. The index was empty before wrapper staging. |

Checks used Python 3.12.10, PyMuPDF 1.27.2.2 and Git 2.52.0.windows.1.
TMP116 startup BUSY, the exact CONFIG clear edge, TEMP byte latching, TMP117/119
offset/comparator ordering and addressed-reset reload, EUN, companion-bit and
ACK effects remain open. The TMP117 DATA_READY capture retains its routing
discrepancy. The linked TMP116 discussion and optional TI software license
manifest remain unavailable. Native driver execution, MCU links, remote CI
and physical qualification were NOT RUN. Stage 43 owns the first remote
CI checkpoint. [Handoff inventory](../prompts/HANDOFFS.md).
