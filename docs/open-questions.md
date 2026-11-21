# Open questions and decisions

## Which fixture should qualify TMP117 first?

Choose the first physical board and say which claims it needs to support.
The board, installed part, wiring and measurement setup are still unconfirmed.
The exact model comes from the populated BOM. A compatible ID at `0x48`
does not distinguish a TMP117 from a TMP119 or establish the accuracy grade.

**Example.** A bench board and the intended P4 product both return a matching
ID and show about 25 °C. The bench run can confirm the sensor transaction and
temperature plausibility. Only a run on the product can qualify that product's
bus, thermal layout and application behavior.

| Option | Pros | Cons |
| --- | --- | --- |
| Start with an identified, isolated TMP117 evaluation board or breakout. | Few variables make sensor and software faults easier to isolate. | An evaluation board needs verified exclusive sensor-bus access. Product wiring, shared-bus behavior and thermal accuracy need a later product run. |
| Start with the product board if available. | Hardware effort goes directly into evidence for the intended bus, enclosure and workload. | Sensor faults are harder to separate from board bring-up issues, and the first physical run depends on board availability. |

Identify the physical board, model or grade, route, ADD0 strap, supply, pullups,
ALERT wiring and available instruments. Use a current linked observation image.
Qualify read-only behavior first. Move to volatile configuration and recovery
trials when the fixture and scope support them. Extend physical coverage to other
exact parts when a consumer needs those claims. The planned software coverage
for all six BOM models still applies.
[Board record](board-integration.md#fixture-evidence) · [Read-only qualification](prompts/40_hil-readonly.md) · [Volatile qualification](prompts/41_hil-volatile-soak.md)

## How should the first shared-bus board address its sensors?

Choose the route from the fitted board when bringing up the optional P4
consumer. Its example leaves the pins, address straps and routing to the
application. A successful compile does not identify the product's wiring.

**Example.** Two populated modules are both strapped to `0x48`. They cannot
be treated as two independently addressed sensors on the same active segment.

| Option | Pros | Cons |
| --- | --- | --- |
| Fit unique ADD0 straps on the same segment when the board permits. | One bus, no route-switch transfers, and simple scheduling. | Needs a free address for each device. Fixed straps and existing peers can rule it out. |
| Put devices with the same fixed address on separate channels of an existing or planned mux. | Keeps both devices individually addressable without changing their straps. | Needs mux hardware and verified route selection. The owner must include route-switch traffic in deadline and cancellation tests. |

[Address straps](board-integration.md#pins-and-add0-straps) · [P4 shared-bus consumer](prompts/36_p4-consumer.md)

## How should the first product meet its temperature accuracy target?

Specify what temperature is being measured, the operating range, and the
allowed error and response time. Compare the fitted sensor with a suitable
reference under the intended workload before choosing a mitigation.

**Example.** A sensor agrees with a reference on a quiet bench at room
temperature. In the enclosure it reads higher when the processor is busy.
Repeat the comparison with typical and heavy loads. If active conversions or
I²C activity drive the error, compare a quieter conversion schedule. If heat
from the processor dominates, test the sensor location and thermal path.

| Option | Pros | Cons |
| --- | --- | --- |
| Keep the fitted layout and normal schedule if they meet the measured limits. | No board or scheduler changes. | Qualification covers the tested range and loads. A room-temperature check alone leaves the rest of the range open. |
| Reduce conversion duty or move unrelated I²C work away from conversions if measurement identifies that cause. | Can reduce workload-dependent error without a board change. | Lower duty can slow updates. Quiet windows can delay other bus clients. Recheck both against product timing needs. |
| Improve coupling to the measured surface or air and isolate the sensor from nearby heat sources if board heat dominates. | Addresses the measured source of bias while retaining the required reporting cadence if the revised placement meets it. | Requires a prototype or assembly change and renewed checks for settling time and error. |

[Accuracy conditions](board-integration.md#guaranteed-accuracy) · [Thermal layout](board-integration.md#thermal-layout-and-precision)

## How should the product obtain a fresh sample?

Choose the required update interval and maximum observation age. Establish
freshness with the driver's acquisition evidence. Two equal temperature
values can be two separate results or one repeated observation. The read
timestamp records when the controller observed the value.

**Example.** A dashboard that needs regular fresh updates and a test that
needs one identifiable conversion on demand can both display 25 °C. The
dashboard can accept successive verified updates with the same value. The
test needs a result attributable to its own trigger.

| Option | Pros | Cons |
| --- | --- | --- |
| Use trusted continuous conversion and paced fresh reads for regular updates. | No per-sample trigger writes. A single bus owner can schedule readiness and reads with the other clients. | Multiple updates can coalesce behind one ready bit, and an overlapping TEMP update has unspecified byte coherence. Report missed readiness or deadline failures explicitly. |
| Use an explicit, guarded one-shot from shutdown for on-demand acquisition. | A completed owned shot provides a stable shutdown result under exclusive ownership and gives the read a specific acquisition boundary. | Adds a guarded trigger and conversion wait per request. It can miss the product's latency or cadence target if triggered too often. |

For a display that only needs the latest observation, the latest-read route
uses one temperature transfer and reports freshness unproven.
[Acquisition routes](api-contracts.md#operation-gates) · [Freshness evidence](device-behavior.md#temp-destructive-reads-and-sample-publication)

## Does the product need an ALERT signal at the MCU?

Choose whether the owner can poll within the product's latency budget. If the
board needs the pin, choose its primary purpose. The same pin can route
threshold indication or conversion readiness according to configuration.

**Example.** With a threshold set at 40 °C, the product may need to wake on
an excursion. A periodic dashboard can read the temperature on its normal
schedule, while a wake-on-threshold application needs a qualified electrical
signal and acknowledgement behavior.

| Option | Pros | Cons |
| --- | --- | --- |
| Poll the status through the existing owner if it meets the latency budget. | Uses the existing bus without an MCU pin or interrupt configuration. | Detection depends on the conversion and poll intervals. The owner must account for read-to-clear flags. |
| Wire ALERT as a threshold signal if the product must wake on an excursion. | A threshold event can request service without a polling round trip. | Takes an MCU input and electrical and re-arm qualification. Conversion timing still limits when the excursion is detected. |
| Wire ALERT as a data-ready signal if the main need is scheduling samples. | The owner can service acquisitions on a cue and reduce empty polls. | The pin then carries readiness instead of the threshold signal. An edge alone cannot establish fresh-conversion provenance. |

Use the same owner to read and attribute events on either wired route.
If the product needs both threshold wake and fresh samples, reserve the pin
for threshold wake and have that owner service conversion readiness over I²C
at the required cadence.

[Board wiring](board-integration.md#pins-and-add0-straps) · [Alert behavior](device-behavior.md) · [Physical alert evidence](prompts/41_hil-volatile-soak.md)

## Who owns a temperature correction?

Decide this only if a product needs correction for its assembled thermal
environment. Offset management remains opt-in. The first release keeps EEPROM
data and shadows read-only. TMP116 and TMP116N have no supported hardware offset.

**Example.** An installed TMP117 reads 0.3 °C high against an independent
reference under the intended conditions. If firmware subtracts 0.3 °C only
from the displayed measurement, the sensor's own threshold comparison may still
use its uncorrected reading. Applying a hardware offset and subtracting the same
correction again in the application would double it.

| Option | Pros | Cons |
| --- | --- | --- |
| Preserve the device setup and apply the correction in the application. | Leaves existing calibration intact and works across the supported models. | The application owns correction provenance and threshold units. Corrected display values can diverge from the sensor's hardware threshold behavior. |
| Opt in to a managed volatile offset on an eligible model. | The correction follows the verified sensor profile. Consumers of fresh TEMP values can share the correction without applying it again. | Needs offset preimage and restore, clean recovery, and evidence before any threshold logic relies on how the comparator uses the offset. |

If an application-side correction is paired with a hardware threshold,
translate the required trip point into the sensor's uncorrected units and
verify the actual trip point against the reference.

[Offset and reset scope](eeprom-and-reset-policy.md#model-gates) · [Remaining silicon unknowns](device-behavior.md#partial-writes-and-remaining-unknowns)

## Evidence still needed

The documented conservative rules cover the unresolved silicon behavior. For
example, a CONFIG read that fails mid-transfer may already have cleared an
event. The owner retains that uncertainty. A successful bench retry would
give one observation, while a broader change would need exact-part evidence
about the clear edge. The same distinction applies to TEMP byte coherence,
partial writes, startup busy behavior and addressed reset effects.
[Silicon evidence ledger](prompts/SILICON.md) · [Source conflicts](reports/feature-gap-audit.md#source-contradictions-and-unresolved-questions) · [Current progress](prompts/PROGRESS.md)
