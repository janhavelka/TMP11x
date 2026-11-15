# 18 — Implement EEPROM-safe initialization and configuration writes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [14](14_binding-lifecycle.md), [17](17_config-codecs.md).

## Objective and boundaries

Apply and verify a desired sensor profile without touching nonvolatile memory or corrupting factory traceability on an unknown power-on state.

## Source audit

Revisit TI EEPROM_UL EUN/busy behavior, CONFIG startup and TI registers loaded from EEPROM; inspect TMP1x2 `beginApply` and readback tests but rewrite timing and readiness logic.

## Complete implementation work

- Build a safe initialization sequence: identity read; read/check EEPROM lock and busy; validate full desired profile locally; ensure safe write admission; apply stable CONFIG and limit words in documented order; read back only stable fields.
- Before each mutating transaction or justified locked phase, prove EUN disabled and busy clear; a failed proof is an explicit unsafe/no-write result. Do not write EUN to lock/unlock silently.
- Handle EEPROM busy with caller-driven bounded waits and clock, never indefinite tight loops or hidden delay. Identify single-shot startup behavior vs continuous configuration from source.
- Separate partial write uncertainty from physical transfer status; preserve staged desired profile for later explicit recover.
- Verify that initialization, even if repeated on a factory-configured sensor, does not read a destructive status and then misrepresent its freshness.

## Verification and exit criteria

Native scripted tests assert zero mutating traffic when busy/unlocked, exact pointer sequences, no general-call address, partial-write dirty state, and independent readback of CONFIG/limits.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_18.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
