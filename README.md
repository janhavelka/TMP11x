# TMP11x — implementation promptset (planning-only checkout)

A staged, evidence-led implementation plan for an independent, framework-neutral
C++17 driver supporting TI TMP116 / TMP116N, TMP117 / TMP117M / TMP117N and
TMP119. The user's primary installed sensor is TMP117.

**This archive is a promptset, not an implemented library.** No chip support,
builds, hardware tests, CI results or GitHub repository are claimed by the
presence of these files.

Start at [docs/prompts/START_HERE.md](docs/prompts/START_HERE.md). The
machine-readable sequence is [SEQUENCE.json](docs/prompts/SEQUENCE.json).
The promptset checker is `python scripts/check_promptset.py`.

## Product boundaries

- Repository/package: `TMP11x`; namespace: `TMP11x`; main header:
  `<TMP11x/TMP11x.h>`; class: `TMP11x::TMP11x`.
- Supported programming family: **TMP116, TMP116N, TMP117, TMP117M, TMP117N,
  TMP119**, with explicit user-selected BOM model and capability checks.
- **Not automatically supported:** TMP102, TMP112, TMP114, TMP118, TMP1075,
  TMP110 or other parts merely sharing some name digits or bus addresses.
- MCU/framework-neutral C++17 sensor core; portable application-owned I2C
  callbacks; thin Arduino and native ESP-IDF examples (S2/S3, P4 where tooling permits).
- Volatile sensor settings and *read-only* EEPROM diagnostics only in the first
  library release. No ordinary-operation EEPROM unlock/program, general-call
  reset, bus initialization, hidden retry or bus-wide side effect.
- Explicit read-to-clear status semantics, genuine freshness evidence,
  bounded owner operations, retained results and failure/recovery visibility.

## Original sources

The agent must consult the exact-part datasheets, not trust this README as a
replacement for them:

- TMP116/TMP116N: https://www.ti.com/lit/ds/symlink/tmp116.pdf (SBOS740A, May 2019)
- TMP117/TMP117M/TMP117N: https://www.ti.com/lit/ds/symlink/tmp117.pdf (SNOSD82D, Sep 2022)
- TMP119: https://www.ti.com/lit/ds/symlink/tmp119.pdf (SNIS236, Jan 2024)
- Exclusion comparison: https://www.ti.com/lit/ds/symlink/tmp114.pdf and
  https://www.ti.com/lit/ds/symlink/tmp118.pdf

Read [SOURCE_MAP](docs/prompts/SOURCE_MAP.md), [SILICON](docs/prompts/SILICON.md)
and [EVIDENCE](docs/prompts/EVIDENCE.md). Original vendor PDFs and any
reference code acquired during execution retain original authorship/terms;
they must not silently enter the MIT production code or exported package.

## Quick orientation

1. Extract this promptset into a fresh local Git checkout (normally `main`).
   If `origin` exists, confirm it is the intended **new** TMP11x repository.
2. In VS Code, give the coding agent the whole-sequence instruction in
   `docs/prompts/START_HERE.md`; alternatively execute one numbered file per turn.
3. Let the agent inspect and selectively adapt the existing `janhavelka` GitHub
   repositories, download TI reference PDFs, record provenance and implement.
4. Agent records actual results under `docs/reports/prompt_N.md` and updates
   `docs/prompts/PROGRESS.md`. Hardware results require real identified hardware.

Promptset source/review history is separate from any code or hardware evidence.
