# TMP11x board integration

Use the exact ordering code and package in the BOM to select the
[driver model](variants.md#model-selection-and-capabilities). Confirm the
assembled part, supply and ADD0 strap before accessing a board.

## Orderable packages and conditions

The table gives example ordering codes and their operating ranges from the
pinned package addenda. The recommended conditions column is the datasheet
family envelope. Apply both limits for the selected ordering code, then apply
the applicable accuracy row and its test conditions.

| BOM model | DRV, 6-pin WSON | YBG, 6-ball DSBGA | Orderable operating range | Datasheet recommended conditions |
|---|---|---|---|---|
| TMP116 | `TMP116AIDRVR` | — | −55 to +125 °C | 1.9–5.5 V, −55 to +150 °C |
| TMP116N | `TMP116NAIDRVR` | — | −55 to +125 °C | 1.9–5.5 V, −55 to +150 °C |
| TMP117 | `TMP117AIDRVR` | `TMP117AIYBGR` | −55 to +150 °C | 1.7–5.5 V at −55 to +70 °C, or 1.8–5.5 V at −55 to +150 °C |
| TMP117M | `TMP117MAIDRVR` | `TMP117MAIYBGR` | 0 to +85 °C | Same TMP117 family envelope, restricted to the 0 to +85 °C orderable range |
| TMP117N | `TMP117NAIDRVR` | `TMP117NAIYBGR` | −55 to +150 °C | Same TMP117 family envelope |
| TMP119 | — | `TMP119AIYBGR` | −55 to +150 °C | 1.7–5.5 V at −55 to +70 °C, or 1.8–5.5 V at −55 to +150 °C |

The dated addenda establish package and grade evidence as archived.
[SBOS740A p.43, 25 Sep 2026](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=43),
[SNOSD82D p.46, 30 Sep 2026](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=46),
[SNIS236 p.45, 9 Nov 2025](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=45).

For TMP116 and TMP116N, the recommended free-air range reaches +150 °C, while
the addendum lists +125 °C and the electrical-characteristics limits end at
+125 °C. Use +125 °C as the upper orderable and specified electrical limit.
For TMP117M, +70 °C is the upper limit for operation from 1.7 V up to
1.8 V exclusive. At 1.8–5.5 V, the orderable range extends to +85 °C.
Use each exact-part electrical table for supply, current and accuracy data.
[SBOS740A §§6.3, 6.5, pp.4–5](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=4),
[SNOSD82D §§6.3, 6.5, pp.5–6](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=5),
[SNIS236 §§6.3, 6.5, pp.6–8](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=6).

## Guaranteed accuracy

These are the maximum absolute temperature errors from the pinned electrical
tables. All rows use eight averages and a 1 Hz conversion cycle. SBOS740A
specifies oil-bath measurements for TMP116 and TMP116N. For TMP117, TMP117M,
TMP117N and TMP119, the I²C input test levels are LOW ≤ 0.05 × V+ and HIGH
≥ 0.95 × V+. The TMP117 test condition calls for an unsoldered DRV thermal pad
across all three TMP117 grades. Apply the supply and orderable limits above
where the table shows “Family limits.”
[SBOS740A p.5, §6.5 and notes 1–2](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5),
[SNOSD82D p.6, §6.5](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6),
[SNIS236 p.7, §6.5](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7).

| BOM model | Test temperature range | V+ | Maximum absolute error |
|---|---|---|---|
| TMP116 | −10 to +85 °C | 3.3 V | 0.20 °C |
| TMP116 | −40 to +105 °C | 3.0–3.6 V | 0.25 °C |
| TMP116 | +105 to +125 °C | 3.0–3.6 V | 0.30 °C |
| TMP116 | −40 to +125 °C | 1.9–5.5 V | 0.40 °C |
| TMP116N | −25 to +85 °C | 3.3 V | 0.30 °C |
| TMP116N | −40 to +125 °C | 3.0–3.6 V | 0.40 °C |
| TMP116N | −40 to +125 °C | 1.9–5.5 V | 0.50 °C |
| TMP116 and TMP116N, note 2 | −55 to −40 °C | 1.9–5.5 V | 0.75 °C |
| TMP117 | −20 to +50 °C | Family limits | 0.10 °C |
| TMP117 | −40 to +70 °C | Family limits | 0.15 °C |
| TMP117 | −40 to +100 °C | Family limits | 0.20 °C |
| TMP117 | −55 to +125 °C | Family limits | 0.25 °C |
| TMP117 | −55 to +150 °C | Family limits | 0.30 °C |
| TMP117M | +25 to +50 °C | Family limits | 0.10 °C |
| TMP117M | 0 to +70 °C | Family limits | 0.15 °C |
| TMP117M | 0 to +85 °C | Family limits | 0.20 °C |
| TMP117N | −40 to +100 °C | Family limits | 0.20 °C |
| TMP117N | −55 to +125 °C | Family limits | 0.25 °C |
| TMP117N | −55 to +150 °C | Family limits | 0.30 °C |
| TMP119 | 0 to +45 °C | Family limits | 0.08 °C |
| TMP119 | −25 to +85 °C | Family limits | 0.09 °C |
| TMP119 | −40 to +85 °C | Family limits | 0.12 °C |
| TMP119 | −55 to +125 °C | Family limits | 0.15 °C |
| TMP119 | −55 to +150 °C | Family limits | 0.20 °C |

The signed 16-bit TEMP encoding has a step of 1/128 °C (0.0078125 °C) and a
numeric span of −256 to +255.9921875 °C. The power-up word `0x8000` represents
the initial sentinel until the first conversion result. The encoding describes
register resolution. The table above supplies the accuracy limits. Board
thermal conditions and any offset setting affect the measurement, so verify
system accuracy separately under the intended assembly and use conditions.
[SBOS740A p.25, Table 5](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=25),
[SNOSD82D p.26, §7.6.2](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=26),
[SNIS236 p.33, §8.5.2](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=33).

## Pins and ADD0 straps

Pin numbers and ball names match the datasheet top views. Use the package
drawing to orient the footprint.

| Signal | DRV pin | YBG ball | Function |
|---|---:|---|---|
| SCL | 1 | A2 | Serial clock input |
| GND | 2 | B2 | Ground |
| ALERT | 3 | C2 | Open-drain threshold or data-ready output |
| ADD0 | 4 | C1 | Address strap |
| V+ | 5 | B1 | Sensor supply |
| SDA | 6 | A1 | Serial data input and open-drain output |

Pin sources are [SBOS740A p.3](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=3),
[SNOSD82D p.4](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=4)
and [SNIS236 p.5](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=5).

| ADD0 connects to | Seven-bit I²C address |
|---|---|
| GND | `0x48` |
| V+ | `0x49` |
| SDA | `0x4A` |
| SCL | `0x4B` |

Give each target a unique address on its active segment, including any
simultaneously enabled mux routes. Pass the unshifted seven-bit address to
the driver. The I²C controller handles the wire address byte and R/W bit.
Address collisions can affect transfers even if the identity word appears
compatible. [SBOS740A p.20, Table 2](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=20),
[SNOSD82D p.21, Table 7-2](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=21),
[SNIS236 p.23, Table 7-2](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23).

## Pullups, logic levels and decoupling

Pull SDA up to a rail compatible with every participant. Provide a pullup for
an open-drain controller's SCL and for ALERT when used. The TI example uses
5 kΩ pullups. Choose actual resistance from total segment capacitance, rise-time
limits, sink-current and LOW-voltage limits, and the effective resistance of
pullups already fitted. For noise or high source impedance, review the supply
decoupling at the sensor. Place a 100 nF ceramic bypass capacitor close to V+
and GND and choose a temperature rating that covers the operating range.
[SBOS740A pp.32, 36](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=32),
[SNOSD82D pp.33, 35](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=33),
[SNIS236 pp.28, 30](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=28).

TMP117 grades and TMP119 specify a recommended 0–5.5 V range for SCL, SDA,
ALERT and ADD0. The TMP116/N recommended conditions list supply and ambient
limits. Their −0.3 to +6 V pin ratings are absolute maximum stress limits.
[SBOS740A p.4, §§6.1, 6.3](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=4),
[SNOSD82D p.5, §6.3](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=5),
[SNIS236 p.6, §6.3](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=6).

The ordinary serial-input bounds are LOW ≤ 0.3 × V+ and HIGH ≥ 0.7 × V+.
The TMP119 table includes ADD0 in these threshold rows. Check the host
thresholds, rail tolerances, leakage and actual signal levels as well. The
TMP117/TMP119 accuracy tables specify tighter I²C input levels. The supported
clock range is 1–400 kHz for all six models. Validate rise and fall times at
the chosen rate. [SBOS740A pp.5–6, §§6.5–6.6](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=5),
[SNOSD82D pp.6–7, §§6.5, 6.7](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=6),
[SNIS236 pp.7–8, §§6.5, 6.7](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=7).

TMP117 and TMP119 allow unused ALERT to float or connect to GND. For TMP116,
the pin table and Figure 41 show a pulled-up ALERT when used. The unqualified
grounding sentence in §8.1.1.1 conflicts with that interrupt circuit. Use the
pulled-up circuit for a connected host input. Figure 44 omits signal wiring
to show the supply filter and leaves the intended unused-ALERT guidance
unresolved. [SBOS740A p.3](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=3),
[SBOS740A p.32, Figure 41 and §8.1.1.1](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=32),
[SBOS740A p.36, Figure 44](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=36),
[SNOSD82D p.33](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=33),
[SNIS236 p.28](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=28).

A direct 3.3 V I²C connection exceeds the TMP114 SCL/SDA absolute maximum
of 2.1 V. TMP114 requires 1.08–1.98 V supply and SCL/SDA at or below 1.98 V
in normal operation. On a low-voltage or level-shifted segment, verify each
participant's supply range, HIGH and LOW thresholds, I/O limits and timing
across operating conditions. Check the [neighbor address ranges](variants.md#excluded-and-shared-bus-devices)
before assigning a strap. [SNIS214E p.4, §§7.1, 7.3](reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf#page=4).

## Thermal layout and precision

Route heat from the measured surface or air to the sensor and limit competing
paths to the host, regulators and pullups. A thin board and a low-resistance
contact path help with surface measurements. For moving air, board thermal
mass can reduce fluctuations at the cost of a slower response. Prevent board
bending near the package and control reflow strain. Verify the response and
steady-state error with the actual stackup, airflow or surface contact.
[SBOS740A pp.36–37](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=36),
[SNOSD82D pp.36–37](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=36),
[SNIS236 pp.30–31](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=30).

For DRV, TI recommends leaving the exposed pad unsoldered for highest
precision on rigid PCBs. Soldering the pad can improve thermal coupling and
flex-board reliability, while assembly stress can shift the reading. If
soldered, connect the pad to GND or leave it electrically floating. The
separate GND pin remains required. Use the six-ball land pattern for YBG.
[SBOS740A p.36](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=36),
[SNOSD82D pp.35–36](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=35),
[SNOSD82D pp.43–45, YBG drawing](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=43),
[SNIS236 pp.42–44, YBG drawing](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=42).

Active conversion heats the die. Account for supply voltage, conversion duty
cycle, averaging, input leakage and traffic on the shared bus in the power
and thermal budget. TI recommends allowing standby time between conversions
and reducing unrelated traffic when practical. TMP119 also recommends
avoiding I²C traffic during active conversion for best precision. Typical
current and self-heating curves describe their test setups. Measure the error
under the expected bus load and thermal conditions.
[SBOS740A pp.5, 34–35](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=35),
[SNOSD82D pp.6, 34–35](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=34),
[SNIS236 p.23, §7.5.3.1.1](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=23),
[SNIS236 pp.29–30](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=29).

## Fixture evidence

The board revision, installed part, controller pins, route, wiring, supply and
measurement setup are unconfirmed. The application owns these choices and
bus initialization. Record the exact fixture and waveform or instrument
evidence during adapter and physical qualification.
