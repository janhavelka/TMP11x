# Pinned development references

Acquired from Texas Instruments on **2026-11-15**. The archive contains five
exact-part datasheets and nine supporting application/EVM documents. It is
development evidence, not implemented sensor support or hardware qualification.

PDF filenames start with the chip or subject, state the document type, and end
with the TI document number including its revision. For example,
`TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf` is distinct from
`TMP117-Evaluation-Module-Quick-Start-Guide-SNOU163.pdf`. Manifest IDs used by
the fetcher, such as `tmp117`, remain stable when a local filename changes.

The 2026-11-17 finding review reconciled manifest visual-page lists as the
union of the recorded intake and later coverage passes: TMP117 pp.6/7/23/25/
27/28/31/32/46, plus new checks of TMP116 p.36 and TMP119 p.4. This updates
review metadata, not acquisition dates, technical revisions or PDF bytes.
SDAA311 is generic portfolio guidance: its active-high output/hysteresis
examples do not override TMP11x's exact-part open-drain and TLOW behavior.
If a TMP117EVM becomes a fixture, SNOU161 pp.3/22 require attention to the
onboard MSP430/GUI: establish exclusive sensor-bus ownership and record any
prior EEPROM programming as non-factory/unknown provenance.

| Source | Document revision | Purpose |
|---|---|---|
| [TMP117](source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf) | SNOSD82D, September 2022 | First implementation and intended PCB model; includes M/N grades |
| [TMP116](source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf) | SBOS740A, May 2019 | TMP116/TMP116N capability comparison |
| [TMP119](source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf) | SNIS236, January 2024 | TMP119 capability comparison |
| [TMP114](source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf) | SNIS214E, July 2023 | Excluded family comparison |
| [TMP118](source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf) | SNIS237A, April 2025 | Excluded family comparison |
| [TMP116 EVM guide](source/TMP116-Evaluation-Module-User-Guide-SNOU145A.pdf) | SNOU145A | Fixture wiring and board reference |
| [TMP117 EVM guide](source/TMP117-Evaluation-Module-User-Guide-SNOU161.pdf), [quick start](source/TMP117-Evaluation-Module-Quick-Start-Guide-SNOU163.pdf) | SNOU161 / SNOU163 | Fixture wiring and evaluation workflow |
| [TMP119 EVM guide](source/TMP119-Evaluation-Module-User-Guide-SNOU199.pdf) | SNOU199 | Fixture wiring and board reference |
| [Precise measurements](source/TMP116-TMP117-Precise-Temperature-Measurements-Application-Note-SNOA986A.pdf) | SNOA986A | Measurement setup |
| [Output data](source/Digital-Temperature-Sensor-Output-Data-Interpretation-Application-Note-SBAA588A.pdf) | SBAA588A | Digital output interpretation |
| [PCB thermal resistance](source/PCB-Thermal-Resistance-for-High-Accuracy-Temperature-Sensors-Application-Note-SNOAA47A.pdf) | SNOAA47A | Thermal design context |
| [NIST traceability](source/NIST-Traceability-for-Temperature-and-Humidity-Sensors-Overview-SBAT024.pdf) | SBAT024 | Traceability context |
| [Alert systems](source/Overtemperature-and-Undertemperature-Alert-System-Design-Application-Note-SDAA311.pdf) | SDAA311, June 2026 | Alert design context |

[manifest.json](manifest.json) records URLs, acquired bytes, SHA-256, revisions,
page counts and visually reviewed pages. [Intake notes](extracted/intake-notes.md)
record original-page pointers and unresolved questions; these are concise review
notes, not full text extracts or the stage 5 register ledger.
[Public code catalog](public-code-catalog.md) distinguishes comparison sources
from code eligible for later adoption.

The completeness audit includes full per-chip register/function inventories:
[TMP116/N](extracted/tmp116-coverage-audit.md),
[TMP117/M/N](extracted/tmp117-coverage-audit.md),
[TMP119](extracted/tmp119-coverage-audit.md), and
[excluded TMP114/TMP118](extracted/excluded-families-audit.md).
They preserve electrical/grade conditions, deliberate exclusions, source
contradictions and undocumented behavior. The
[gap report](../reports/feature-gap-audit.md) maps the findings into future
implementation stages; these notes do not replace the original PDFs.

From the repository root, using Python 3.10 or newer:

```text
python scripts/check_references.py
python scripts/test_check_references.py
python scripts/fetch_references.py
python scripts/fetch_references.py tmp117
```

The checker is offline and uses only the standard library. The fetcher restores
missing files from the pinned manifest, verifies bytes before saving and refuses
to overwrite existing or accept changed files. A TI URL can change its PDF even
without changing the cover revision; review a new download separately and update
the manifest deliberately. These commands do not silently refresh source facts.

TI retains copyright and all notices in the original documents. The repository
MIT license does **not** relicense vendor PDFs. Preserve them unchanged, and
exclude `docs/reference/**` from future PlatformIO/runtime package exports and
SDK installs. No vendor driver code has been copied into production source.
