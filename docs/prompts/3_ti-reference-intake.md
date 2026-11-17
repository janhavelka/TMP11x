# 3 — Acquire and pin official TI source documents

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [1](1_ecosystem-core-audit.md), [2](2_tooling-peer-audit.md).
Produces: `ti-archive`.
Consumes: `ecosystem-audit`, `tooling-plan`.

## Objective and boundaries

Establish a reproducible exact-part reference archive and no-copy license boundary before encoding device behavior.

## Source audit

Verify any existing reference manifest and original bytes before downloading.
Acquire missing exact-part TI TMP116/TMP116N, TMP117/M/N and TMP119 datasheets,
plus TMP114/TMP118 for exclusion comparison. SBOS740A, SNOSD82D and SNIS236 are
seed revisions, not a command to reject newer official revisions. Record the
revision actually acquired; preserve pinned originals when updating. Inspect
original PDFs visually for non-text register tables and footnotes, not only
extracts. Search current TI errata/support and primary examples.

## Complete implementation work

- Save original PDFs in `docs/reference/source/` with stable names; record document IDs, revisions, source URLs, retrieval date, SHA-256 and licenses/notices in `docs/reference/manifest.json`. If remote revision changed, retain what was acquired and record new facts explicitly.
- Create concise original notes in `docs/reference/extracted/` with document/page pointers for each reviewed section; all interpreted tables must be traceable to the PDF, not just a scrape. Avoid committing full copyrighted text extracts alongside the original documents.
- Create `docs/reference/public-code-catalog.md` for available TI examples, Linux `drivers/iio/temperature/tmp117.c`, Adafruit drivers or other useful comparisons. Pin inspected code to its commit/release and license; record timeouts, access gates and uninspected licenses explicitly. Secondary driver/GUI code is optional comparison evidence, not a prerequisite for a datasheet-based implementation or closure of this source-intake stage.
- Do not paste GPL Linux code, copyrighted vendor snippets or large external text into the production MIT driver. Attribute adapted MIT source. Vendor PDFs must be excluded from runtime package exports.

## Verification and exit criteria

Run `python scripts/check_references.py` for hashes and path existence; also
check that cited sections are found at correct PDF pages. Hash verification
proves archive integrity, not a register review. Check original source version
and variants. Complete this stage when required exact-part PDFs, provenance,
reviewed-page notes and an honest available/unavailable source catalog exist.
An unresolved silicon question remains owned by its field/timing/safety stage;
an inaccessible optional code example alone does not block the archive.
No device/source similarity is assumed purely from a shared prefix.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_3.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
