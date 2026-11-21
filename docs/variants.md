# TMP11x models and identity

## Model selection and capabilities

Select the model from the populated BOM and the seven-bit address from its
ADD0 strap on the application's bus route. TMP117 is the primary target.
Grade and package affect the board design and accuracy limits. Keep the
selected model even when two grades share the same register behavior.
The [orderable packages and operating conditions](board-integration.md#orderable-packages-and-conditions)
and [guaranteed accuracy tables](board-integration.md#guaranteed-accuracy) give
the exact-part conditions for each row.

| BOM model | Silicon family | Register map | Seven-bit I2C addresses | EEPROM shadow words | Optional managed offset | Addressed reset | Supply and accuracy source |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `Model::TMP116` | TMP116 | TMP116 | `0x48–0x4B` | 4 | Unavailable | Unavailable | [SBOS740A pp.4–5, TMP116 rows](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5) |
| `Model::TMP116N` | TMP116 | TMP116 | `0x48–0x4B` | 4 | Unavailable | Unavailable | [SBOS740A pp.4–5, TMP116N rows](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5) |
| `Model::TMP117` | TMP117 | TMP117-like | `0x48–0x4B` | 3 | `0x07`, opt-in | CONFIG bit 1 | [SNOSD82D pp.5–6, TMP117 rows](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6) |
| `Model::TMP117M` | TMP117 | TMP117-like | `0x48–0x4B` | 3 | `0x07`, opt-in | CONFIG bit 1 | [SNOSD82D pp.5–6, TMP117M rows](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6) |
| `Model::TMP117N` | TMP117 | TMP117-like | `0x48–0x4B` | 3 | `0x07`, opt-in | CONFIG bit 1 | [SNOSD82D pp.5–6, TMP117N rows](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6) |
| `Model::TMP119` | TMP119 | TMP117-like | `0x48–0x4B` | 3 | `0x07`, opt-in | CONFIG bit 1 | [SNIS236 pp.6–7, TMP119 rows](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7) |

Both maps use `0x00` for temperature, `0x01` for CONFIG, `0x02` for the
high limit, `0x03` for the low limit, `0x04` for EEPROM lock and busy
control/status, and `0x0F` for the read-only device ID. TMP116 uses `0x07`
for EEPROM3 and marks CONFIG bits 1:0 unused. The TMP117-like map uses
`0x07` for offset and defines a reset command at CONFIG bit 1. Represent
these differences with the ID mask/value and revision layout, EEPROM
word pointers/count, offset support and addressed reset support. Use shared
operations where register behavior matches and apply each gate to every
access path. Offset management is opt-in so applications can preserve
existing calibration. Reset is an explicit operation, and general-call reset
is outside the driver contract. [SBOS740A pp.20, 24, 26](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24),
[SNOSD82D pp.21, 25, 28](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25),
[SNIS236 pp.23, 32, 35](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32).

### TMP119M scope

SNIS236 p.7 gives TMP119M accuracy limits of ±0.08 °C at 0–45 °C and
±0.09 °C at −20–65 °C. The pinned package addendum dated 9-Nov-2025
lists `TMP119AIYBGR` and `TMP119AIYBGR.A`, both in YBG DSBGA with
a −55 to +150 °C operating range. The six-model scope excludes TMP119M.
Adding it would require a verified exact ordering code and package,
operating conditions and programming-model evidence. [SNIS236 electrical
characteristics, p.7](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7),
[package addendum, p.45](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=45).

## Identity decision

Read one 16-bit word, most-significant byte first, at pointer `0x0F` on
the designated route and address. The compatibility rule uses the selected
BOM model. Retain the raw word and the documented revision field, when
present, with the compatibility result. Model, package, accuracy grade and
physical-unit provenance require independent evidence. An unrelated
responder can coincidentally return compatible data.

| Selected models | Match rule for complete word | Revision and TI source |
| --- | --- | --- |
| TMP116, TMP116N | `(word & 0xFFFF) == 0x1116` | DID occupies all 16 bits. Revision is unavailable. [SBOS740A p.31 Table 15](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=31) |
| TMP117, TMP117M, TMP117N | `(word & 0x0FFF) == 0x0117` | Bits 15:12 are revision. The ID section gives `0x0117`. [SNOSD82D p.32 Table 7-15](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=32) |
| TMP119 | `(word & 0x0FFF) == 0x0117` | Bits 15:12 are revision. The register map lists `0x0117`, while the device-ID section gives `0x2117`. Both show base DID `0x117`. [SNIS236 p.32 Table 8-2](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32), [p.39 Table 8-14](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=39) |

| Selected model and observation | Result |
| --- | --- |
| Unsupported model value or address outside `0x48–0x4B`, including shifted address `0x90` | Reject locally with zero bus traffic. |
| Supported selection, NACK, timeout or malformed/short read | Report transport or framing failure. No complete ID observation exists. |
| TMP116 or TMP116N reads `0x1116` | Compatible full-word ID. Revision is unavailable and the grade remains BOM metadata. |
| TMP116 or TMP116N reads `0x0116`, `0x2116` or `0x0117` | Incompatible ID. End validation. |
| Any TMP117 grade or TMP119 reads `0x0117`, `0x2117` or `0xA117` | Compatible base ID, with revision 0, 2 or 10 respectively. The observed word leaves TMP117 versus TMP119, exact grade and physical identity unresolved. |
| Any TMP117 grade or TMP119 reads `0x1116` | Incompatible ID. End validation. |
| Any supported selection reads `0x0000` or `0xFFFF` as a complete word | Incompatible ID. End validation. |

The ID operation writes the pointer byte alone and reads two data bytes after
a repeated START in the same transfer. On failure or mismatch, it ends before
additional register reads or data writes. Use this operation only on a known
route and candidate with compatible framing. Subsequent volatile mutations
require their own readiness, lock/busy and configuration safety checks.
[Owner transport contract](prompts/ENGINEERING.md#transport-and-ownership),
[SBOS740A p.20](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=20),
[SNOSD82D p.21](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=21),
[SNIS236 p.23](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23).

As secondary evidence, Linux IIO compares the three full words `0x1116`,
`0x0117` and `0x2117` and uses firmware/device match information as a
fallback for other words. Here revision semantics come from the TI ID
tables and the model comes from the BOM. [Pinned public-code
catalog](reference/public-code-catalog.md#linux-iio-tmp11x-driver--factual-comparison-only),
[Linux source at the catalog revision](https://github.com/torvalds/linux/blob/23c62304dde29abe92013d85910d03a77dada35d/drivers/iio/temperature/tmp117.c).

## EEPROM shadow and traceability

| Selected models | EEPROM1 | EEPROM2 | EEPROM3 | EEPROM4 |
| --- | --- | --- | --- | --- |
| TMP116, TMP116N | `0x05` | `0x06` | `0x07` | `0x08` |
| TMP117, TMP117M, TMP117N | `0x05` | `0x06` | `0x08` | Unmapped |
| TMP119 | `0x05` | `0x06` | `0x08` | Unmapped |

The pointer assignments come from the exact-part register maps.
[SBOS740A p.24](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24),
[SNOSD82D p.25](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25),
[SNIS236 p.32](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32).

Each pointer names one 16-bit EEPROM data shadow. TI describes a unique ID
programmed during manufacturing to support NIST traceability. The EEPROM
overviews and EEPROM1 descriptions for both TMP117 and TMP119 refer to four
words or `EEPROM[4:1]`. Use the three data shadow pointers in their register
maps, which assign `0x07` to offset.
[TMP116 overview, SBOS740A p.17](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 overview, SNOSD82D p.18](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18),
[TMP117 EEPROM1, p.30](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=30),
[TMP119 overview, SNIS236 p.20](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20),
[TMP119 EEPROM1, p.37](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=37).

A register read observes the current volatile shadow. Earlier writes may
have changed a shadow or its backing EEPROM cell. Report the word as
observed, including all-zero or nonfactory data, without assigning serial
validity from its contents. Factory authentication, a calibration
certificate and the current contents of physical NVM require separate
evidence. The first-release policy keeps all EEPROM data words read-only,
including their volatile shadows. The [reset and safety policy stage](prompts/7_eeprom-reset-policy.md)
and [EEPROM observation stage](prompts/24_readonly-eeprom.md) own detailed
guard and observation behavior.

## Excluded and shared-bus devices

| Excluded family | Seven-bit I2C addresses | Protocol and register differences |
| --- | --- | --- |
| TMP114 / TMP114N | TMP114A/B/C/D use `0x48/0x49/0x4A/0x4B`. TMP114ND/NC/NB/NA use `0x4C/0x4D/0x4E/0x4F`. | CONFIG is at `0x03`, alert status is separate, and ID is at `0x0B` with base DID `0x114`. Its control byte supports auto-increment and the part supports optional CRC framing. [Addresses and ratings, SNIS214E pp.3–4](reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=3), [protocol and map, pp.21–29](reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=21), [ID, p.38](reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=38). |
| TMP118 / TMP118M | TMP118A–D and TMP118MA–MD each use `0x48–0x4B` in letter order. | Limits are reversed (`0x02` low, `0x03` high), and CONFIG has different one-shot, fault-count and averaging fields. ID at `0x0B` puts DID `0x118` in bits 15:4 and revision in bits 3:0. UID retrieval changes CONFIG and writes UID registers. [UID, SNIS237A pp.18–19](reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=18), [addresses, p.23](reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=23), [map, CONFIG and ID, pp.26–30](reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf#page=26). |
| TMP112 | Address-only TMP112D X2SON uses `0x40–0x43`. Fixed ALERT TMP112D0–D3 X2SON and strapped SOT563 grades use `0x48–0x4B`. | 12-bit or 13-bit temperature data, different CONFIG layout, and a four-register map. [Addresses, SBOS473L p.16](https://www.ti.com/lit/ds/symlink/tmp112.pdf#page=16), [map and CONFIG, pp.22–23](https://www.ti.com/lit/ds/symlink/tmp112.pdf#page=22). |
| TMP1075 / TMP1075N | TMP1075 uses `0x40–0x5F`, TMP1075N uses `0x48–0x4B`. | 12-bit LM75-style map with reversed limit order. ID at `0x0F` is `0x7500` where supported. TMP1075N lacks that register. [Addresses, SBOS854F p.14](https://www.ti.com/lit/ds/symlink/tmp1075.pdf#page=14), [map and ID, pp.21–24](https://www.ti.com/lit/ds/symlink/tmp1075.pdf#page=21). |

The TMP114 and TMP118 non-indexed read rules and register maps support an
inferred `0x0000` observation at `0x0F` under ordinary framing. A
CRC-disabled TMP114 is the assumed fixture in that case. With CRC enabled,
TI documents a NACK for a bad CRC and `0xFF` bytes if clocking continues.
Treat behavior with an omitted CRC as unresolved. End validation on a
mismatch or transfer error without trying the foreign ID, UID, CONFIG or
CRC framing. [Excluded-family inventory](reference/extracted/excluded-families-audit.md).

Coexistence requires one target per resolved address on each active bus
segment and compatible supplies, input levels, pullups and timing for
every participant. The application owns bus routing and address planning.
For TMP114, use a 1.08–1.98 V supply and keep SCL/SDA at or below 1.98 V in
normal operation (2.1 V absolute maximum). A direct 3.3 V pullup bus is
unsafe. A low-voltage or level-shifted segment must meet every participant's
supply, input HIGH and input LOW requirements. Host mismatch fixtures
provide software evidence only. [SNIS214E recommended and absolute
limits, p.4](reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=4).
