# 24 — Implement safe EEPROM reads and traceability metadata

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Features, safe measurements and diagnostics**. Type: **software**.
Prerequisites: [4](4_family-identity.md), [5](5_register-ledger.md), [7](7_eeprom-reset-policy.md), [11](11_status-values.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [19](19_config-trust.md).

## Objective and boundaries

Provide read-only observations of EEPROM-backed register shadows, with no way
to program factory data through routine methods. These reads do not directly
prove the underlying nonvolatile cell contents after an outside volatile write.

## Source audit

Read TI EEPROM1..4 layout, read side effects, unique ID/NIST text, and EEPROM_UL EUN/busy for each model. Compare 24Cxx diagnostic read APIs only; do not borrow EEPROM programming methods.

## Complete implementation work

- Expose `readEepromWord(index, out)` with a documented index origin, checked 3-word (TMP117/119) versus 4-word (TMP116) ranges, raw bytes/word order and provenance. TMP116 pointers are 0x05/0x06/0x07/0x08; TMP117/119 pointers are 0x05/0x06/0x08, never a contiguous block including offset 0x07.
- Add a non-atomic read-only snapshot of the included shadow words and lock/busy state. Do not assert a stable unique serial number, factory provenance or actual NVM readback merely from these words. Locked writes can alter the shadows without programming EEPROM (TMP116 p.29; TMP117 p.30; TMP119 p.37). No diagnostic reset is allowed to reload the shadows.
- Make public `writeRegisterRaw` reject EEPROM_UL and all physical EEPROM word pointers; for TMP117/TMP119 `0x07` offset remains permitted only through guarded typed control.
- Ensure tests prove that normal begin/read/CLI/dump cannot emit EUN unlock or general-call reset; even a disguised 0x07 write cannot reprogram TMP116 EEPROM3.
- Document lock uncertainty and how application/user can collect read-only metadata without changing traceability.

## Verification and exit criteria

Test range bounds, per-model pointer map, repeated reads, busy state, prohibited write fuzz and absence of factory-identification modification.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_24.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
