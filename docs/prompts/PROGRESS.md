# TMP11x implementation progress

**State:** NOT STARTED — this file ships with the planning-only promptset.
**Target:** 44 ordered prompts; TMP117 first, then TMP116/TMP119 variants.
**Repository:** fresh checkout; remote URL, initial HEAD and working branch to
be recorded by prompt 1.
**Hardware:** physical TMP11x sensor board and test wiring NOT CONFIRMED.
**Last completed stage:** None.

The agent must update the table below with actual stage evidence, report path,
commit and sync status; `COMPLETE` requires all stage-owned tests/doc checks.
A blocked physical stage remains BLOCKED, while later independent software
work may complete.

| Stage range | Planned objective | Current state |
|---|---|---|
| 1–7 | Source and silicon evidence, variant safety | NOT STARTED |
| 8–15 | Architecture, package foundation, transport/identity/health | NOT STARTED |
| 16–27 | Temperature, config, fresh conversions, limits, offset, EEPROM-read, diagnostics | NOT STARTED |
| 28–33 | Cooperative owner operations and adversarial native tests | NOT STARTED |
| 34–39 | Arduino/IDF/P4 integration, CLI and Python runner | NOT STARTED |
| 40–41 | Physical HIL qualification (fixture-dependent) | NOT STARTED |
| 42–44 | Package/docs, one CI checkpoint, independent final audit | NOT STARTED |

For each stage append a row to `docs/reports/index.md` with stage, status,
immutable commit, checks and remaining gates. Do not create a success report
for planned work.
