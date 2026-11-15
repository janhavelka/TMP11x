# Pinned development references

Acquired from Texas Instruments on **2026-11-15**. The archive contains five
exact-part datasheets and nine supporting application/EVM documents. It is
development evidence, not implemented sensor support or hardware qualification.

| Source | Document revision | Purpose |
|---|---|---|
| [TMP117](source/tmp117.pdf) | SNOSD82D, September 2022 | First implementation and intended PCB model; includes M/N grades |
| [TMP116](source/tmp116.pdf) | SBOS740A, May 2019 | TMP116/TMP116N capability comparison |
| [TMP119](source/tmp119.pdf) | SNIS236, January 2024 | TMP119 capability comparison |
| [TMP114](source/tmp114.pdf) | SNIS214E, July 2023 | Excluded family comparison |
| [TMP118](source/tmp118.pdf) | SNIS237A, April 2025 | Excluded family comparison |
| [TMP116 EVM guide](source/snou145a-tmp116evm.pdf) | SNOU145A | Fixture wiring and board reference |
| [TMP117 EVM guide](source/snou161-tmp117evm.pdf), [quick start](source/snou163-tmp117evm-quickstart.pdf) | SNOU161 / SNOU163 | Fixture wiring and evaluation workflow |
| [TMP119 EVM guide](source/snou199-tmp119evm.pdf) | SNOU199 | Fixture wiring and board reference |
| [Precise measurements](source/snoa986a-precise-temperature.pdf) | SNOA986A | Measurement setup |
| [Output data](source/sbaa588a-output-data.pdf) | SBAA588A | Digital output interpretation |
| [PCB thermal resistance](source/snoaa47a-pcb-thermal-resistance.pdf) | SNOAA47A | Thermal design context |
| [NIST traceability](source/sbat024-nist-traceability.pdf) | SBAT024 | Traceability context |
| [Alert systems](source/sdaa311-alert-systems.pdf) | SDAA311, June 2026 | Alert design context |

[manifest.json](manifest.json) records URLs, acquired bytes, SHA-256, revisions,
page counts and visually reviewed pages. [Intake notes](extracted/intake-notes.md)
record original-page pointers and unresolved questions; these are concise review
notes, not full text extracts or the stage 5 register ledger.
[Public code catalog](public-code-catalog.md) distinguishes comparison sources
from code eligible for later adoption.

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
