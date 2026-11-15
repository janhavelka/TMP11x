# Requirement-to-stage coverage

A planned prompt is **not** implementation evidence. Use this table with the
machine-readable per-part feature ledger established in stage 5 and maintained
through stage 44. Record `documented`, `implemented`, `native tested`,
`firmware built`, `CI tested`, `hardware qualified` separately.

| Requirement | Owner stages | Evidence to close |
|---|---|---|
| Current GitHub code and licensing audit | 1–2; every relevant stage | Exact commit/path and adoption decision |
| TI exact-part sources, revisions, hashes | 3–7; 44 | Original PDFs + manifest + page-backed ledger |
| TMP116/TMP117/TMP119 capability selection | 4–5; 13; 32 | Per-model identity and capability fixtures |
| Framework-independent C++17, I2C ownership | 8–12; 28–30; 42 | Native linked no-SDK/installed consumer |
| No EEPROM writes or global reset | 7; 18; 23–26; 31–33; 44 | Forbidden I2C patterns zero callback on denial |
| Correct signed measurement and thresholds | 16–17; 21–22; 32–33 | Independent raw literal vectors |
| Startup and EEPROM-busy lock | 6–7; 18–20; 29–31 | Deterministic guarded I2C traces |
| Conversion cycle, average, mode, one-shot | 6; 20–21; 30 | Correct register/timing and fresh samples |
| Alert DATA_READY and destructive-read semantics | 6; 21–22; 26; 30 | Event-flag consumption and stale-negative tests |
| TMP117/119 offset, not TMP116 offset | 4–5; 23; 32 | Capability refusal with zero I2C |
| Safe read-only EEPROM identifiers | 7; 24; 32 | Variant word maps; write forbidden |
| Verified config, dirty trust, recovery | 18–19; 28–31 | Injected ambiguous-failure tests |
| Cooperative bounded jobs and retained results | 9; 28–31 | Exact callbacks, deadline and tokens |
| Full native tested behavior | 32–33 | Registered strict CTest/native suite |
| Arduino, IDF, P4 examples | 34–36 | Full target links; unavailable BLOCKED |
| Shared CLI + Python HIL | 37–39 | Native CLI and runner negative tests |
| Real exact-part hardware qualification | 40–41 | Raw fixture transcript, restoration evidence |
| Packages, docs, one CI checkpoint, audit | 42–44 | Isolated consumers, exact CI SHA, final audit |
