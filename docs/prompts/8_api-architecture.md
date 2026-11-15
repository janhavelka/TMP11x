# 8 — Freeze public API and variant architecture

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Architecture and package foundation**. Type: **software**.
Prerequisites: [1](1_ecosystem-core-audit.md), [2](2_tooling-peer-audit.md), [4](4_family-identity.md), [5](5_register-ledger.md), [6](6_timing-status.md), [7](7_eeprom-reset-policy.md).

## Objective and boundaries

Choose one coherent driver model and no unnecessary shared foundation with TMP1x2. Document real type and method semantics before copying code.

## Source audit

Revisit `TMP1x2/include/TMP1x2/{Config,Status,TMP1x2}.h`, `src/TMP1x2*.cpp`, `OPT4001` IDs/freshness, ADS1115 dirty profile, and FieldCore-node current integration seams read-only. Verify source revisions and tests.

## Complete implementation work

- Create `docs/api-contracts.md` and `docs/architecture.md`. Select namespace `TMP11x`, class `TMP11x`, exact public file names, C++17, `Config` with selected `Model`, address, borrowed I2C callbacks, user/time/GPIO contexts, timeout and desired sensor settings.
- Define `Status`, health, identity/family confidence, sample provenance, snapshots, typed setters/getters, limits, offset and safe EEPROM reads; normal latest and confirmed-fresh methods distinct.
- Map every register functionality to *one* owner (transport/identity/config/measurement/diagnostic). Choose one managed desired-profile replay engine and one cooperative operation vocabulary.
- Specify failure-output policy, read-to-clear side effects and package eligibility for public APIs. Avoid inheriting TMP1x2 extended-mode API or including TMP1x2 as dependency.
- Add explicit side-by-side `TMP1x2` and `TMP11x` consumer sketches sharing one I2C owner without any runtime inheritance.

## Verification and exit criteria

Independent API/ownership review: no model-inappropriate functionality is exposed as success, no duplicate scheduler, no runtime SDK coupling, no hidden EEPROM write.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_8.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
