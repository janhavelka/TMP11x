# 7 — Set nonvolatile, identity and reset safety policy

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [3](3_ti-reference-intake.md), [4](4_family-identity.md), [5](5_register-ledger.md), [6](6_timing-status.md).
Produces: `safety-policy`.
Consumes: `variant-contract`, `register-ledger`, `behavior-contract`, `register-validator`.

## Objective and boundaries

Prevent accidental EEPROM programming, loss of NIST traceability and bus-wide reset effects while retaining useful read-only diagnostics.

## Source audit

Read TI EEPROM unlock, programming, busy, factory data and general-call reset sections for all three; compare 24Cxx EEPROM ownership (reference only) and TMP1x2 BusOperations (explicit shared-bus reset, deliberately not copied). Inspect exact TMP117/TMP119 CONFIG soft-reset bit and TMP116 absence/behavior.

## Complete implementation work

- Create `docs/eeprom-and-reset-policy.md`: EUN powers up locked (0), unlocked/unknown fail-closed condition, busy timing, model-dependent 0x07 (TMP116 EEPROM3 / TMP117–TMP119 Temp_Offset), protected read-only regions, actual factory unique-data claims.
- Freeze first-release scope: read EEPROM register words by checked model, never offer EEPROM write/unlock or a generic raw-write escape. Deny writes to EEPROM data addresses even when EUN=0 would only modify their volatile shadow registers. Reads observe those register shadows, not guaranteed untouched nonvolatile/factory contents if another writer previously changed them.
- Document when a guarded configuration/threshold/offset write must first verify EEPROM lock and ready state, how uncertain I2C failure affects trust, and how a consumer recovers without programming. Include prompt 6's independent startup guard; busy=0 is insufficient POR proof for TMP116's contradictory source. Require one application authority for sensor mutation during the operation: a lock read cannot protect against an uncoordinated second master unlocking EEPROM before the write.
- Read only 0x04 for the safety guard: EUN bit 15 and busy bit 14 (the mirror of CONFIG bit 12) in one word, without clearing CONFIG flags. Every mutating or freshness-producing operation reads it once at entry under exclusive sensor ownership; latest/diagnostic reads need not and cannot claim freshness. Reuse this observation only within that uninterrupted operation. Reset, interference, ambiguous failure or observed busy invalidates it; recheck before any later write or fresh result. EUN=1/busy=1/unestablished startup means no mutation, including shutdown/one-shot/reset, and no fresh result. There is no software attempt to stop conversions on an unlocked device.
- An observed EUN=1 stays fail-closed until the application power-cycles or an authorized external tool relocks the sensor and reestablishes startup/ownership. The library writes no relock and never uses soft reset as one. Ordinary recovery cannot make this unsafe state safe by itself.
- Distinguish addressed Soft_Reset's documented bit/duration from undocumented companion-bit application, EEPROM reload, EUN effects and bus availability. Stage 25 uses a conservative explicitly labelled write policy and mandatory postwait observations; fakes explore unknown outcomes rather than assuming general-call semantics.
- Separate optional individually addressed soft reset from forbidden general-call reset. An unresolved RESET register semantics creates a specific blocked reset capability rather than an invented API.
- Define explicit `UNSUPPORTED`/`EEPROM_UNSAFE` or equivalent errors for incompatible requests. This sequence contains no EEPROM-programming or destructive hardware campaign, including on sacrificial fixtures; such future maintenance work needs a separate user request and policy.

- Give every excluded hardware function an explicit disposition using X01–X08 in [FULL_COVERAGE](FULL_COVERAGE.md). Include ARA arbitration/acknowledgement and synchronized conversions via programmed startup plus general-call reset; neither becomes an API or CLI operation.
- Describe externally initiated ARA, general-call reset, power cycle and writes as interference with status/configuration assumptions. Invalidate affected evidence when such interference is known; do not promise detection of every outside action. NVM endurance/program timing and factory traceability remain documented hardware facts, not delivered library functionality.

## Verification and exit criteria

Adversarial review checks every path capable of a mutating I2C transaction; confirm no normal path reaches EUN or general-call address `0x00`. Record any incompatible TI source footnotes.
At this research stage, review planned paths and forbidden transcript vectors;
production callback assertions belong to the implementation owners. Update
stage 5's policy/access/reset metadata and run its register-ledger validator.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_7.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
