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

- **Start with a known TMP117 bench fixture.** An isolated evaluation board
  or identified breakout makes it easier to isolate sensor and software
  problems. It leaves the eventual product wiring, shared bus and thermal
  environment to be qualified on the product. An evaluation board requires
  verified exclusive access to the sensor bus.
- **Start on the intended product board if it is available.** One campaign can
  collect product-specific bus and measurement evidence sooner. Bring-up and
  sensor faults are harder to separate, and a board delay delays the first
  physical run.

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

- **Use unique ADD0 addresses on a direct shared bus when the board permits.**
  This avoids route-switch traffic and simplifies transfer ownership. Fixed
  straps or an existing peer can limit the available addresses.
- **Use distinct routes if addresses cannot be changed.** A board with a mux
  can isolate the two modules. The application owner must verify route
  selection and include its traffic in the deadline and cancellation tests.

[Address straps](board-integration.md#pins-and-add0-straps) · [P4 shared-bus consumer](prompts/36_p4-consumer.md)

## What must the first product prove about accuracy and freshness?

Choose the operating temperatures, acceptable error, and sample age or latency
the product needs. Those requirements determine whether the default reporting
route is enough and what must be measured on the fitted board.

**Example.** A sensor agrees with a reference on a quiet bench at room
temperature. In the product, the processor warms the board and other I²C
devices are active during conversions. The same sensor can report a different
temperature or deliver a fresh value later than the consumer needs.

- **Start with measured periodic reads on the actual bus.** Validate sample
  age and error at representative temperatures and traffic. This uses the
  simplest scheduling route. Sparse points only establish performance under
  their recorded conditions.
- **Add conversion-aware scheduling if the product needs it.** The application
  can compare quieter conversion windows or alternate settings under the same
  thermal setup, with traffic from real peers. This can improve performance
  when traffic limits it and adds scheduling constraints for the other devices.
  Tighter accuracy claims need a suitable calibrated reference and operating
  range evidence under either route.

[Accuracy conditions](board-integration.md#guaranteed-accuracy) · [Thermal layout](board-integration.md#thermal-layout-and-precision) · [Conversion and freshness policy](device-behavior.md)

## Does the product need an ALERT signal at the MCU?

Choose whether the owner can poll within the product's latency budget. If the
board needs the pin, choose its primary purpose. The same pin can route
threshold indication or conversion readiness according to configuration.

**Example.** With a threshold set at 40 °C, the product may need to wake on
an excursion. A periodic dashboard can read the temperature on its normal
schedule, while a wake-on-threshold application needs a qualified electrical
signal and acknowledgement behavior.

- **Poll using the existing owner.** No extra MCU pin or interrupt setup is
  needed. Response time depends on the conversion and polling schedules, and
  reads can consume ready or alert flags.
- **Route ALERT for a named purpose.** Use the interrupt as a wake or service
  hint. The owner still reads and attributes the event. This adds wiring and
  fixture work for polarity, the selected mode and re-arm. Choose a threshold
  route for an excursion or a data-ready route for sample scheduling.

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

- **Preserve the device setup and keep any required correction in the
  application.** This leaves existing calibration alone and can work across
  models. The application owns units, correction provenance and threshold
  expectations when display and hardware thresholds differ.
- **Opt in to managed volatile offset on an eligible model when the product
  needs it.** Retain the preimage and verify offset, measurement, threshold
  behavior and recovery. The correction lives with the sensor configuration,
  with extra ownership and restoration work. Offset-versus-comparator ordering
  needs evidence before relying on it.

[Offset and reset scope](eeprom-and-reset-policy.md#model-gates) · [Remaining silicon unknowns](device-behavior.md#partial-writes-and-remaining-unknowns)

## Evidence still needed

The documented conservative rules cover the unresolved silicon behavior. For
example, a CONFIG read that fails mid-transfer may already have cleared an
event. The owner retains that uncertainty. A successful bench retry would
give one observation, while a broader change would need exact-part evidence
about the clear edge. The same distinction applies to TEMP byte coherence,
partial writes, startup busy behavior and addressed reset effects.
[Silicon evidence ledger](prompts/SILICON.md) · [Source conflicts](reports/feature-gap-audit.md#source-contradictions-and-unresolved-questions) · [Current progress](prompts/PROGRESS.md)
