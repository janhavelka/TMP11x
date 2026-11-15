# Full feature/variant ledger and deliberate first-release limits

Implement and verify **all included non-destructive, software-accessible
features** supported by each exact device. "Documented", "implemented",
"host-tested", "MCU-linked", "remote-CI", and "physically qualified" are
separate status/evidence columns. This checklist seeds the actual maintained
matrix at `docs/feature-coverage.md` created in stage 5.

| Capability | TMP116/N | TMP117/M/N | TMP119 | Planned stages |
|---|---|---|---|---|
| 7-bit address 0x48–0x4B, MSB-first I2C | Yes | Yes | Yes | 12–14 |
| 16-bit temperature 1/128°C, negative values | Yes | Yes | Yes | 16, 32–33 |
| ID / revision and family proof | 0x1116 | base 0x117 | base 0x117 | 4, 13 |
| Volatile configuration read/write/verification | Yes | Yes | Yes | 17–19 |
| Continuous / shutdown / one-shot | Yes | Yes | Yes | 20–21, 27–30 |
| Averaging and conversion cycle timing | Yes | Yes | Yes | 6, 20 |
| DATA_READY / alert side effects | Yes | Yes | Yes | 6, 21–22 |
| High/low limits and therm/alert | Yes | Yes | Yes | 22 |
| Polarity and ALERT vs data-ready output | Yes | Yes | Yes | 22 |
| Temperature offset register at 0x07 | No | Yes | Yes | 23 |
| EEPROM read-only data | 4 x 16-bit | 3 x 16-bit | 3 x 16-bit | 7, 24 |
| EEPROM unlock/program/persist | **Intentionally excluded** | **Intentionally excluded** | **Intentionally excluded** | 7, 24 |
| Individual software reset | Check exact part | Check exact part | Check exact part | 25 |
| General-call reset | **Intentionally excluded** | **Intentionally excluded** | **Intentionally excluded** | 7, 25 |
| Raw read / safe diagnostics | Yes | Yes | Yes | 26 |
| Safe raw volatile writes | Guarded | Guarded | Guarded | 26 |
| Health, dirty state, explicit recovery | Yes | Yes | Yes | 15, 19, 31 |
| Cooperative jobs / retained results | Yes | Yes | Yes | 28–31 |
| Arduino S2/S3 and native IDF examples | Core + thin adapters | Core + thin adapters | Core + thin adapters | 34–35 |
| ESP32-P4 native IDF consumer | Core compatible | Primary PCB use | Core compatible | 36 |
| Shared CLI / Python runner | Yes | Yes | Yes | 37–39 |
| Actual physical temperature/ALERT proof | Per fixture | Primary expected | Per fixture | 40–41 |

**Exclusions not to disguise as missing work:** TMP114/TMP118, auto-identifying
accuracy grades by I2C, general-call reset, EEPROM reprogramming, automatic
retry/bus recovery, I2C/TCA mux ownership, device certificate claims, and
absolute physical accuracy validation without a calibrated fixture.

At each prompt, add `source reference`, `owner stage`, `implementation path`,
`registered test`, `CLI command`, `real board/variant`, `status`, and `remaining
evidence` for every changed feature. Variant-specific absent features must
be tested as explicit rejection, not successful no-op.
