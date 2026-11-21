# 18 — Implement EEPROM-safe initialization and configuration writes

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [7](7_eeprom-reset-policy.md), [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [14](14_binding-lifecycle.md), [16](16_temperature-codec.md), [17](17_config-codecs.md), [47](47_audit-10-17.md).
Produces: `safe-apply-engine`, `sync-initialize-recover`.
Consumes: `safety-policy`, `api-contract`, `owner-contract`, `binding-api`, `public-identity-api`, `transport-health`, `temperature-codec`, `config-codec`, `latest-read-engine`, `behavior-contract`, `behavior-vectors`, `register-fake`, `audit-10-17`.

## Objective and boundaries

Apply and verify a desired sensor profile without touching nonvolatile memory or corrupting factory traceability on an unknown power-on state.

## Source audit

Revisit TI EEPROM_UL EUN/busy behavior, CONFIG startup and TI registers loaded from EEPROM; inspect TMP1x2 `beginApply` and readback tests but rewrite timing and readiness logic.

TMP116 SBOS740A p.17 section 7.5.1.1 says busy is cleared during reset loading,
whereas p.26 Table 6 and p.29 Table 10 say busy is asserted during power-up.
Retain this source conflict: a clear busy bit alone is not startup proof.
TMP117 SNOSD82D p.18 and TMP119 SNIS236 p.20 describe asserted POR busy and
ignored CONFIG/limit writes during load; offset-write behavior is not specified.
TMP116 p.17 describes all register writes as ignored during that interval.

## Complete implementation work

- Implement real synchronous `begin(Config)`, `begin()` and `recover()` over one private resumable apply/readback step engine. Each step performs at most one physical callback; waits retain an explicit phase and due time and return pending. The synchronous wrapper drives these same steps with a bounded clock/work loop; stage 29 adds job admission/poll glue. Test transfer counts and suspend/resume directly without test-only production job kinds.
- Reuse stage 14 binding and stage 15 tracking. Extend bus-silent bind validation to every accepted sensor-profile field using stages 16/17 codecs, retaining the prior binding on rejection. Then verify identity, establish startup readiness, and guard stable CONFIG/limit writes. Verify all owned fields; an ACK does not prove the write escaped POR suppression. Stage 23 adds opt-in offset to this engine. Until then, offset control remains unavailable and its provenance unknown; no accepted request may be silently ignored.
- Establish startup readiness from stage 8 and acquire the operation-local pre-write guard by reading only 0x04: EUN bit 15 zero and busy bit 14 clear. Busy mirrors CONFIG bit 12; do not use destructive CONFIG merely as a guard. EUN=1 refuses immediately, including shutdown. EUN=0/busy=1 permits only paced 0x04 re-reads within the deadline/work cap, with no writes until a safe word is observed. No retry after a failed transfer. Reuse an acquired safe word only within uninterrupted exclusive ownership; reset/rebind/outside access/ambiguous failure/observed busy invalidates it before further writes. There is no automatic relock.
- Handle startup/busy waits with the authoritative clock and finite work cap. When proof is absent, FIRST_ACK anchors the wait at the first successful compatible read's completion in this operation, never an earlier bind/operation. This is the only origin policy, stored internally with no caller timestamp or Config seed. Require stable power/no outside reset from that read through all operations relying on its proof; an ACK establishes a conservative origin, not completed POR. Retain established proof until invalidated. After reported reset/power loss use stage 19's bus-silent invalidation, restore stable power, then recover. Approximate 1.5-ms POR is not a guaranteed maximum. One-shot is not a replayable EEPROM startup mode and uses the same guard.
- Own minimum desired/observed/dirty state for truthful apply failure and recovery. An accepted bind/`begin(Config)` installs its validated target directly. For settings changes, commit the staged sensor target on the first mutating write attempt; pre-write rejection preserves the old target, while ambiguous attempts retain the candidate for explicit recovery. `begin()`/recover replay the existing target. Keep binding immutable. Stage 19 extends snapshots, stage 29 reuses phases and stage 31 hardens them.
- Verify that initialization, even if repeated on a factory-configured sensor, does not read a destructive status and then misrepresent its freshness.

- Make apply order explicit: all managed profile changes, including limit-only and alert routing/polarity-only changes, use the shared guarded SD → apply/verify → baseline → desired-mode → final readback sequence. This conservative uniform policy prevents an old conversion being attributed to a new profile; stage 23 extends it with managed offset. Limit write ordering still preserves valid intermediate pairs but does not promise atomic/glitch-free alerts or rollback. Validate the whole candidate before writing and retain partial effects. Every CONFIG read feeds the shared alert record; later zero flags cannot erase captured alerts.
- Use the stage 11 Config without a public offset setting until stage 23. A successful begin here verifies CONFIG/limits only; it does not assert hardware offset zero or full final-profile trust. Preserve unknown offset provenance in observations and documentation while allowing real initialized temperature acquisition.
- After the final desired-mode write, read CONFIG and compare all stable fields before clearing dirty or reporting initialized. An ACK alone is insufficient. Retain its returned alert evidence and use its consumed readiness as the current-generation baseline; failure at this final phase preserves dirty state and partial effects.
- Stage 19 later connects `invalidateDeviceState()` to this startup engine. In this stage, exercise missing/renewed startup proof through fresh binding and private state seams; do not call or expose an unimplemented future invalidation API.

## Verification and exit criteria

Native scripted tests assert zero mutating traffic when busy/unlocked or startup
readiness is unestablished, including a TMP116 reset-load fixture with busy clear
and ignored writes. Check invalid profiles cause zero I2C, exact pointer
sequences, no general-call address, partial-write dirty state, canonical CONFIG
readback and independent limit verification. When moving from therm to an
inverted alert-mode pair, require a timely matching SD/alert CONFIG readback
before the first limit write that would invert the pair. In the reverse
direction, verify both ordered limits before enabling therm.
Include public begin/recover success and failure transcripts, real managed
latest-read integration from stage 16, health integration from stage 15 and
full-profile bind rejection preserving the prior binding without I2C.
Test late successful callbacks retaining physical/flag evidence without clearing
dirty, marking initialized or publishing a sample. Alert evidence from
initialization survives until explicit consumption; readiness observed during
initialization establishes the baseline only, not a pending fresh result.
Test delayed power after an earlier bind, FIRST_ACK anchored after operation
start, internal clock wrap, end/begin renewing proof, immediate EUN refusal and
busy-to-safe/busy-timeout sequences without premature writes.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_18.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
