# 5 — Create exhaustive per-variant register and bitfield ledger

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Exact silicon source and feature evidence**. Type: **software**.
Prerequisites: [3](3_ti-reference-intake.md), [4](4_family-identity.md).

## Objective and boundaries

Translate the actual device register maps into one checked, machine-readable source of truth with distinct access, reset and read effects.

## Source audit

Use TI TMP116 printed p.24-31, TMP117 p.25-32, TMP119 p.32-39; check CONFIG/EEPROM/threshold tables, read-to-clear flags, reserved and reset fields. Compare symbolic register definitions from TMP1x2, OPT4001 and open reference drivers only as secondary evidence.

## Complete implementation work

- Create `docs/registers/tmp11x.json` schema and validator under `tools/check_register_ledger.py`; record per-model register address, width, access, reset domain, field masks/encodings, side effects, prerequisites, source file/page, and feature-stage owner.
- Cover TEMP 0x00, CONFIG 0x01, THIGH 0x02, TLOW 0x03, EEPROM_UL 0x04, DEVICE_ID 0x0F; TMP116 EEPROM words are 0x05/0x06/0x07/0x08, while TMP117/TMP119 EEPROM words are 0x05/0x06/0x08 and OFFSET is 0x07. The same address across different model profiles is legal; reject overlapping fields/duplicate registers within one profile, not the intentional 0x07 model distinction.
- Record CONFIG R/C DATA_READY and high/low alert flags, EEPROM busy, conversion mode/cycle/average, therm/alert/polarity/pin select and per-part low-bit/soft-reset semantics. Explicitly distinguish `write 1 command` vs persistent bit and reserved bits.
- Record that MOD=10 is a documented continuous-mode alias that reads back 00, not a reserved mode. High/low alert read effects depend on alert versus therm mode. EUN is bit 15 of 0x04; its busy bit 14 mirrors CONFIG bit 12. Keep register access permissions separate from the stricter first-release policy that denies EEPROM and unlock writes.
- Preserve source conflicts explicitly: TMP116 p.17 says busy is cleared during reset-load, while p.26 Table 6/p.29 Table 10 say it indicates power-up; TMP119 p.20 says four EEPROM words, while its p.32 map exposes three plus offset; TMP119 ID defaults also disagree. Do not silently resolve contradictory prose by copying another model.
- Create `docs/feature-coverage.md` seeded with source-backed documented capability, planned implementation stage, test owner, CLI and qualification evidence columns. No entry is implemented merely because it is listed.

- Carry every R/F/X identifier from [FULL_COVERAGE](FULL_COVERAGE.md) into the coverage artifact with per-model disposition and source/test owners. Enumerate every field, reserved pointer 0x09–0x0E and invalid upper pointer bits; do not silently mask an invalid selector to a valid address.
- Record field-level reset/NVM domains: CONFIG MOD1 (bit11) is explicitly not EEPROM-stored, whereas MOD0 and bits9:2 are. Include THIGH/TLOW/OFFSET factory defaults versus prior programmed values, and arbitrary EEPROM-shadow contents.
- Preserve the additional conflicts in the three coverage audits: TMP116 EEPROM reset map versus detailed tables; TMP117/TMP119 EEPROM2 reset heading versus map/field table; range/current/accuracy-summary contradictions. Each conflict needs paired citations and a bounded implementation policy, not an unqualified selected default.
- Add pointer retention, no documented auto-increment, serial framing, ADD0 mapping, electrical timing and the hardware interface timeout as sourced non-register requirements with application/adapter ownership.

## Verification and exit criteria

Test schema rejection of same-profile overlap, duplicate keys, out-of-range masks, missing provenance, incompatible variant semantics, and marked-resolved conflicts without evidence. Verify duplicate JSON keys are caught while parsing rather than discarded by a default dictionary parser. Independent literal masks come from PDF, not from the driver's future code.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_5.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
