# 4 — Freeze included models, IDs and capability classes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [3](3_ti-reference-intake.md).

## Objective and boundaries

Establish which devices can share one driver without conflating model names, silicon revisions, accuracy grades, or other TI parts.

## Source audit

Compare exact part datasheets' feature/pin/accuracy/address/ID sections; especially TMP116 `0x1116`, TMP117 `0x0117` example, TMP119 `0x2117` device-ID section versus its register-map `0x0117` entry. Inspect Linux TMP116/TMP117/TMP119 support and confirm the upper nibble's revision semantics. Inspect TMP114 and TMP118 maps to establish exclusion.

## Complete implementation work

- Create `docs/variants.md` with explicit `Model::{TMP116,TMP116N,TMP117,TMP117M,TMP117N,TMP119}` table: family, supported address, register/capability classes, EEPROM words, optional offset, power-range and accuracy-grade references.
- Define model selection as BOM-supplied. Document what a device-ID read proves for TMP116 versus TMP117/TMP119: `0x1116` as specified for TMP116, low 12 bits `0x117` for TMP117/TMP119; do not treat revision nibble as guaranteed specific model identification. Preserve contradictory TI table wording.
- Design narrow capabilities (offset supported, EEPROM word count, identity mask, any reset differences); keep one implementation where binary behavior matches. No generic inheritance or unrelated TMP11x parts.
- Document unsupported TMP114/TMP118/TMP112/TMP1075 reasons, addressing and safety of coexistence on a shared I2C bus.

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
