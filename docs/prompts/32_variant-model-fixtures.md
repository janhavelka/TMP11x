# 32 — Complete independent multi-model device register simulators

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Cooperative operation and exhaustive native qualification**. Type: **software**.
Prerequisites: [13](13_identity-selection.md), [17](17_config-codecs.md), [18](18_safe-initialize.md), [21](21_fresh-one-shot.md), [23](23_offset-feature.md), [24](24_readonly-eeprom.md), [29](29_cooperative-config.md), [30](30_cooperative-measurement.md), [31](31_failure-recovery.md).
Produces: `variant-fixtures`.
Consumes: `variant-contract`, `register-ledger`, `behavior-vectors`, `public-identity-api`, `config-codec`, `sync-acquisition`, `offset-feature`, `readonly-eeprom`, `configuration-jobs`, `measurement-jobs`, `fault-closure`, `register-fake`.

## Objective and boundaries

Exercise the same compiled production driver against deliberately different
TMP116, TMP117 and TMP119 fakes, including model-specific invalid requests.
Installed-package/relocation qualification remains an explicit consumer gate;
do not assume an install/export implementation already exists at this stage.

## Source audit

Inspect the direct production code against original register/behavior ledger. Draw inspiration from TMP1x2 `test/variants`, ADS1115 fake register model and OPT4001 result-ready tests, not from copied expected values.

## Complete implementation work

- Complete the incremental device fakes used by earlier stages, rather than postponing meaningful protocol tests until now. Model model-dependent 0x07 (TMP116 EEPROM3 / TMP117–TMP119 Temp_Offset), identity/revision, EEPROM-backed shadows distinct from stored cells, lock/busy, startup result, DATA_READY/status effects and mode/cycle/average behavior. TMP116 documents all startup writes ignored; TMP117/TMP119 specify CONFIG/limits, leaving startup OFFSET behavior unknown. Do not hard-code all models/registers identically.
- Exercise both conflicting TMP116 reset-busy interpretations as adversarial fixtures; do not promote either to a confirmed silicon fact. Include canonical MOD=10 readback, TMP117/119 reset-bit behavior, therm flags unaffected by CONFIG reads, TEMP leaving high/low flags intact, and failed reads that nevertheless clear status.
- Test BOM-selected `TMP116N`, `TMP117M`, `TMP117N` accuracy metadata without claiming electrical grade proof from I2C. TMP119 `0x2117` and alternate revision nibble must not cause invented exact identification.
- Verify the public API returns unsupported **before I2C** for TMP116 offset, rejects EEPROM writes, maps THIGH/TLOW in correct order, and uses one shared conversion codec.
- Introduce common operation fixtures run for all families plus model-specific tests. Test misleading same-address unrelated devices and mux-target context switching.
- Keep the simulator separate from production paths and document the constraints it cannot prove about real silicon.

- Add independent block-average stimuli, partial-average abort, all 32 timing pairs, all eight alert routing combinations, latch re-arm and positive/negative offset-saturation observations. Use integral-average stimuli when chip rounding is unspecified. Test nonfactory persisted CONFIG/limits/offset and MOD1's nonpersistent reset behavior.
- One-shot timing tests cover four AVG values plus CONV independence; all 32 pairs remain required for continuous timing. TMP116 comparator block-end timing is a labelled inference, unlike explicit TMP117/TMP119 wording. POR/general-call reload fixtures remain distinct from uncertain addressed-reset effects. Unmanaged offset survives default initialization/recovery with no offset write; explicit managed zero is honored.
- Add named TMP114/TMP118 mismatch fixtures: CRC-off reads of non-indexed 0x0F return zero under their documented protocol, with no fallback ID/UID probe or data write. Add CRC-enabled TMP114 transport-failure/0xFFFF adverse cases; p.26 documents bad-CRC responses, not necessarily omitted-CRC behavior. A mismatch dump stops at ID before foreign status reads. Physical TMP114 substitution requires the low-voltage/level-shifter constraints in the board guide; a host fake does not authorize it.
- Treat undocumented same-write restart, partial-word commit, MOD readback during/after OS, MOD=11 retrigger, inverted therm limits, DATA_READY/alert-mode SD-entry flags, byte coherence and offset/comparator ordering as uncertainty cases. Therm HIGH retention under unchanged therm/routing/polarity/reset is documented; do not randomly clear it on SD. Include both compatible post-OS MOD outcomes and the explicit guarded SD baseline. Stronger hardware claims require independent evidence.
- Consolidate negative assertions for X03/X08: no core ARA/general-call/EEPROM-program/foreign CRC/UID transaction, while externally reported ARA/reset/write interference invalidates applicable evidence. Do not invent an ARA clear rule for undocumented simultaneous flags or ready-pin routing.

## Verification and exit criteria

Verify model fixtures are not near-copies of implementation codecs; use manually derived literal bytes/masks and deliberately wrong vectors as negative controls.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_32.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
