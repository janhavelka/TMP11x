# 18 — Implement EEPROM-safe initialization and configuration writes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [14](14_binding-lifecycle.md), [16](16_temperature-codec.md), [17](17_config-codecs.md).
Produces: `safe-apply-engine`, `sync-initialize-recover`.
Consumes: `safety-policy`, `api-contract`, `owner-contract`, `binding-api`, `public-identity-api`, `transport-health`, `temperature-codec`, `config-codec`.

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

- Implement real synchronous `begin(Config)`, `begin()` and `recover()` over one bounded apply/readback engine. Reuse stage 14 binding and stage 15 tracking. Validate all currently accepted profile fields locally before I2C, then verify identity, establish stage 6 startup readiness, and check EEPROM lock/busy before stable CONFIG and limit writes. Verify all owned stable fields; do not assume an acknowledged write escaped POR suppression. Stage 23 adds offset to this same engine. Until then, offset control remains unavailable and its trust is unknown; no accepted offset request, including zero, may be silently ignored or reported applied.
- Before each mutating transaction or explicitly justified exclusive-device phase, establish EUN disabled and busy clear. A failed proof is an explicit unsafe/no-write result. Do not write EUN to lock/unlock silently; an I2C mutex alone cannot prevent another owner from changing EUN between callbacks.
- Handle EEPROM busy and startup timing with bounded waits and a clock, never indefinite loops or hidden delay. Use a conservative reset/load timing policy and readback; the approximate 1.5-ms POR interval is not a guaranteed worst-case specification. One-shot cannot be an EEPROM startup mode. Keep its trigger out of stable configuration application.
- Own the minimum desired/observed/dirty state needed for truthful apply failure and explicit recovery now. Commit a staged sensor target on the first mutating write attempt; before that, rejection/failure preserves the old target. After an ambiguous attempt retain the candidate for explicit `recover()`. Keep binding immutable during settings replay. Stage 19 extends observation/snapshots; stage 29 reuses the same phases for jobs and stage 31 hardens them.
- Verify that initialization, even if repeated on a factory-configured sensor, does not read a destructive status and then misrepresent its freshness.

- Make apply order and intermediate states explicit for CONFIG and both limits, with a defined extension for the supported offset when stage 23 lands. Separate register writes are not atomic; do not promise glitch-free paired limits, suppressed intermediate alerts or rollback. Validate the entire candidate before the first write, record partial application, and require readback of all owned fields.
- Use the stage 11 Config without a public offset setting until stage 23. A successful begin here verifies CONFIG/limits only; it does not assert hardware offset zero or full final-profile trust. Preserve unknown offset provenance in observations and documentation while allowing real initialized temperature acquisition.

## Verification and exit criteria

Native scripted tests assert zero mutating traffic when busy/unlocked or startup
readiness is unestablished, including a TMP116 reset-load fixture with busy clear
and ignored writes. Check invalid profiles cause zero I2C, exact pointer
sequences, no general-call address, partial-write dirty state, canonical CONFIG
readback and independent limit verification.
Include public begin/recover success and failure transcripts, real managed
latest-read integration from stage 16, health integration from stage 15 and
profile validation failures after a structurally valid bus-silent bind.

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
