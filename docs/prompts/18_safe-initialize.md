# 18 — Implement EEPROM-safe initialization and configuration writes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [14](14_binding-lifecycle.md), [16](16_temperature-codec.md), [17](17_config-codecs.md).

## Objective and boundaries

Apply and verify a desired sensor profile without touching nonvolatile memory or corrupting factory traceability on an unknown power-on state.

## Source audit

Revisit TI EEPROM_UL EUN/busy behavior, CONFIG startup and TI registers loaded from EEPROM; inspect TMP1x2 `beginApply` and readback tests but rewrite timing and readiness logic.

TMP116 SBOS740A p.17 section 7.5.1.1 says busy is cleared during reset loading,
whereas p.26 Table 6 and p.29 Table 10 say busy is asserted during power-up.
Retain this source conflict: a clear busy bit alone is not startup proof.
TMP117 SNOSD82D p.18 and TMP119 SNIS236 p.20 describe asserted POR busy;
all describe writes ignored during the initial load interval.

## Complete implementation work

- Validate the full desired profile locally before I2C. Then verify identity, establish startup/reset readiness using the stage 6 timing policy, and check EEPROM lock/busy before applying stable CONFIG and limit words. Read back only stable fields; do not assume an acknowledged write escaped POR suppression.
- Before each mutating transaction or explicitly justified exclusive-device phase, establish EUN disabled and busy clear. A failed proof is an explicit unsafe/no-write result. Do not write EUN to lock/unlock silently; an I2C mutex alone cannot prevent another owner from changing EUN between callbacks.
- Handle EEPROM busy and startup timing with bounded waits and a clock, never indefinite loops or hidden delay. Use a conservative reset/load timing policy and readback; the approximate 1.5-ms POR interval is not a guaranteed worst-case specification. One-shot cannot be an EEPROM startup mode. Keep its trigger out of stable configuration application.
- Separate partial write uncertainty from physical transfer status; preserve staged desired profile for later explicit recover.
- Verify that initialization, even if repeated on a factory-configured sensor, does not read a destructive status and then misrepresent its freshness.

## Verification and exit criteria

Native scripted tests assert zero mutating traffic when busy/unlocked or startup
readiness is unestablished, including a TMP116 reset-load fixture with busy clear
and ignored writes. Check invalid profiles cause zero I2C, exact pointer
sequences, no general-call address, partial-write dirty state, canonical CONFIG
readback and independent limit verification.

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
