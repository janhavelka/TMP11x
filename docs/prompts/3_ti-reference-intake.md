# 3 — Acquire and pin official TI source documents

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [1](1_ecosystem-core-audit.md), [2](2_tooling-peer-audit.md).

## Objective and boundaries

Establish a reproducible exact-part reference archive and no-copy license boundary before encoding device behavior.

## Source audit

Download exact current official TMP116/TMP116N SBOS740A, TMP117/M/N SNOSD82D and TMP119 SNIS236 datasheets from TI; download TMP114/TMP118 for exclusion comparison. Inspect original PDFs visually for non-text register tables and footnotes, not only extracts. Search current TI errata/support and primary examples.

## Complete implementation work

- Save original PDFs in `docs/reference/source/` with stable names; record document IDs, revisions, source URLs, retrieval date, SHA-256 and licenses/notices in `docs/reference/manifest.json`. If remote revision changed, retain what was acquired and record new facts explicitly.
- Create `docs/reference/extracted/` text/Markdown notes with original document/page pointers for each reviewed section; all interpreted tables must be traceable to the PDF, not just a scrape.
- Create `docs/reference/public-code-catalog.md` including relevant TI official code, Linux `drivers/iio/temperature/tmp117.c`, Adafruit drivers or vendor examples as reference with commit/license, identifying observed defects and independent comparisons.
- Do not paste GPL Linux code, copyrighted vendor snippets or large external text into the production MIT driver. Attribute adapted MIT source. Vendor PDFs must be excluded from runtime package exports.

## Verification and exit criteria

Check hash recomputation, path existence and that cited sections are found at correct PDF pages. Check original source version and variants. No device/source similarity is assumed purely from a shared part prefix.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run unrelated sibling suites or remote CI (except stage 43).

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_3.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
