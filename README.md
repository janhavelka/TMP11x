# TMP11x

A new, framework-neutral C++17 driver project for TI TMP116 / TMP116N,
TMP117 / TMP117M / TMP117N, and TMP119. TMP117 is the primary intended sensor.

**The sensor driver is not implemented yet.** This
repository contains the 44-stage implementation plan, source audits, pinned
TI reference documents, and local preparation tools. There are no completed
sensor, MCU build, or physical qualification claims.

## Start here

- [Agent and execution guide](docs/prompts/START_HERE.md)
- [Current progress](docs/prompts/PROGRESS.md) and [preparation report](docs/reports/preparation.md)
- [Audit of all 44 prompts](docs/reports/promptset-audit.md)
- [Review and responses to the 68-finding audit](docs/prompts/audit.md#response--2026-11-17)
- [Datasheet/functionality gap audit](docs/reports/feature-gap-audit.md) and [complete feature requirements](docs/prompts/FULL_COVERAGE.md)
- [Prompt handoff audit](docs/reports/handoff-audit.md) and [producer/consumer inventory](docs/prompts/HANDOFFS.md)
- [Library comparison](docs/library-comparison.md) and [source adoption](docs/source-adoption.md)
- [Toolchain and package plan](docs/toolchain-and-package-plan.md)
- [TI reference archive and provenance](docs/reference/README.md)
- [Ordered stages](docs/prompts/README.md) and [dependency manifest](docs/prompts/SEQUENCE.json)

## Planned product boundaries

- Repository/package: `TMP11x`; namespace/class: `TMP11x::TMP11x`;
  main header: `<TMP11x/TMP11x.h>`.
- Explicit BOM model selection for TMP116, TMP116N, TMP117, TMP117M, TMP117N,
  and TMP119. Device revision bits do not prove exact model or accuracy grade.
- TMP114, TMP118, TMP102, TMP112 and other similarly named devices are excluded.
- Portable C++17 core with application-owned I2C callbacks; thin Arduino and
  native ESP-IDF examples for ESP32-S2/S3, plus P4 where tooling permits.
- Volatile settings and read-only EEPROM diagnostics in the first release.
  No EEPROM programming, general-call reset, bus ownership or hidden retries.
- Explicit destructive-read effects, evidence of fresh conversions, bounded
  owner operations, retained results, and visible failure/recovery state.

## Local preparation checks

```sh
python scripts/check_promptset.py
python scripts/test_check_promptset.py
python scripts/check_references.py
python scripts/test_check_references.py
git diff --check
```

On Windows, `scripts\pio.cmd --version` uses the existing VS Code-managed
PlatformIO installation. It does not install tools. C++/firmware commands will
be added with the corresponding implementation stages.

## Sources and licenses

Original TI PDFs are pinned by revision, URL and SHA-256 in
[the reference manifest](docs/reference/manifest.json). Use the original PDF
and its page references when interpreting registers; the promptset's seed
notes are not a replacement. See [SOURCE_MAP](docs/prompts/SOURCE_MAP.md) for
sibling repositories and [EVIDENCE](docs/prompts/EVIDENCE.md) for source rules.

Repository-authored code and adapted MIT support files use [LICENSE](LICENSE).
Vendor documents and external reference code retain their own copyrights
and terms; the root MIT license does not relicense them. Reference artifacts
must be excluded from library package exports.
