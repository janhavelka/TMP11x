# TMP11x implementation progress

**State:** Repository preparation complete; sensor implementation NOT STARTED.
**Target:** 44 ordered prompts; TMP117 first, then TMP116/TMP119 variants.
**Repository:** `main`, tracking `origin/main` at `https://github.com/janhavelka/TMP11x.git`.
**Preparation input HEAD:** `6c5e4496d5c896af689228d4bee4382a53c20626` (clean and synchronized at entry).
**Hardware:** Physical TMP11x board and test wiring NOT CONFIRMED.
**Last completed stage:** 2. Stage 3 source intake is PARTIAL.
**Last update:** 2026-11-15; see [preparation report](../reports/preparation.md).

| Stage range | Objective | Current state and evidence |
|---|---|---|
| 1 | Current sibling API/ownership audit | COMPLETE ? [report](../reports/prompt_1.md) |
| 2 | Toolchain/package/prompt workflow audit | COMPLETE ? [report](../reports/prompt_2.md) |
| 3 | Official source intake | PARTIAL ? [report](../reports/prompt_3.md); all required PDFs pinned, TI code/license access remains open |
| 4?7 | Model identity, field/timing ledger, safety policy | NOT STARTED |
| 8?15 | Architecture, package foundation, transport/identity/health | NOT STARTED |
| 16?27 | Temperature, config, fresh conversions, limits, offset, EEPROM-read, diagnostics | NOT STARTED |
| 28?33 | Cooperative owner operations and adversarial native tests | NOT STARTED |
| 34?39 | Arduino/IDF/P4 integration, CLI and Python runner | NOT STARTED |
| 40?41 | Physical HIL qualification (fixture-dependent) | NOT STARTED; physical evidence NOT RUN |
| 42?44 | Package/docs, one CI checkpoint, independent final audit | NOT STARTED |

Next explicit implementation task: verify archive integrity and relevant audit
revisions. Retain stage 3's inaccessible TI source obligation; independent
stage 4 work may proceed using acquired datasheets after confirming concrete
prerequisite inputs. Stage 6 must revisit the unreadable E2E DATA_READY race
lead. Do not infer implementation completion from these downloads.

The [report index](../reports/index.md) separates completed preparation from
stage-owned evidence. Final commit/upstream synchronization is recorded in the
task handoff; use Git history to identify the commit containing this checkpoint.
A stage may be COMPLETE only when its owned exit criteria are met. A required
physical stage remains BLOCKED when invoked without a fixture; software tests
and compiled firmware never substitute for physical evidence.
