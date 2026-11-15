# 4 — Freeze included models, IDs and capability classes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [3](3_ti-reference-intake.md).
Produces: `variant-contract`, `board-guide`.
Consumes: `ti-archive`.

## Objective and boundaries

Establish which devices can share one driver without conflating model names, silicon revisions, accuracy grades, or other TI parts.

## Source audit

Review the pinned [intake notes](../reference/extracted/intake-notes.md),
including TMP119 page 7's TMP119M rows. Establish actual orderable grades and
record any proposed scope change explicitly; do not silently add a model or
infer an accuracy grade from revision bits.

Compare exact part datasheets' feature/pin/accuracy/address/ID sections; especially TMP116 `0x1116`, TMP117 `0x0117` example, TMP119 `0x2117` device-ID section versus its register-map `0x0117` entry. Inspect Linux TMP116/TMP117/TMP119 support and confirm the upper nibble's revision semantics. Inspect TMP114 and TMP118 maps to establish exclusion.

## Complete implementation work

- Create `docs/variants.md` with explicit `Model::{TMP116,TMP116N,TMP117,TMP117M,TMP117N,TMP119}` table: family, supported address, register/capability classes, EEPROM words, optional offset, power-range and accuracy-grade references.
- Create `docs/board-integration.md` with source-backed package/pin, supply, ADD0, pull-up, decoupling and precision conditions. This is the early guide consumed independently by stages 34/35; those stages add actual adapter/wiring details and stage 42 finalizes it. Unknown fixture details stay unknown.
- Define model selection as BOM-supplied. Document the compatible response expected from TMP116 versus TMP117/TMP119: all 16 bits `0x1116` for TMP116 (SBOS740A p.31 Table 15), low 12 bits `0x117` for TMP117/TMP119 (SNOSD82D p.32 Table 7-15; SNIS236 p.39 Table 8-14). Only the latter documents define a revision nibble; do not invent one for TMP116 or promise acceptance of undocumented TMP116 IDs. No ID read proves an accuracy grade or unique physical part. Preserve contradictory TI table wording.
- Design narrow capabilities (offset supported, EEPROM word count, identity mask, any reset differences); keep one implementation where binary behavior matches. No generic inheritance or unrelated TMP11x parts.
- Document unsupported TMP114/TMP118/TMP112/TMP1075 reasons, addressing and safety of coexistence on a shared I2C bus.

- Close the source/documentation portion of F01/F02/F16/F17 in [FULL_COVERAGE](FULL_COVERAGE.md), leaving later implementation/adapter/HIL evidence open: a concrete exact-grade/package matrix must separate code range, recommended conditions, specified accuracy range, orderable operating range, supply limits and accuracy test conditions. In particular, retain TMP116's +150 versus +125 °C source conflict and TMP117M's pinned 0–85 °C orderable range. Check current ordering sources if making a current availability claim; dated addenda are not live inventory.
- Record ADD0 GND/V+/SDA/SCL → 0x48/49/4A/4B, package pinouts and open-drain ALERT. Include supply/temperature restrictions, averaging/cycle and near-rail input conditions where specified, plus package-specific thermal-pad guidance. Accuracy grades remain BOM metadata.
- Use the [excluded-family inventory](../reference/extracted/excluded-families-audit.md) to record TMP114's different map/CRC/auto-increment and TMP118's reversed limits, CONFIG fields, ID and mutating UID procedure. No probing another family's UID or configuration to disambiguate a 0x0F response.

## Verification and exit criteria

Cross-check each capability/ID row against exact PDF tables. A small variant-identity decision table and invalid/ambiguous observation examples must exist before implementation.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_4.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
