# TMP114 and TMP118 exclusion coverage

Audit date: 2026-11-15; promptset baseline `8d2ffe3`. Sources are pinned
[TMP114 SNIS214E, July 2023](../source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf), manifest ID `tmp114`,
and [TMP118 SNIS237A, April 2025](../source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf), ID `tmp118`.
Page numbers are one-based PDF pages and match printed body pages.
Register/protocol/feature sections were read; original map/CONFIG table
images were checked on TMP114 pp.29,32 and TMP118 pp.26–27. These families
are reference exclusions, not additional supported models or implementation
stages. Their common signed temperature scale and overlapping I2C addresses
do not make them compatible with TMP116/TMP117/TMP119.

## TMP114 / TMP114N

| Pointer | Complete register inventory and relevant fields |
| --- | --- |
| 0x00 TEMP | Signed 16-bit, 1/128°C, reset zero. p.30. |
| 0x01 SLEW_RESULT | [15:2] signed 14-bit slew result, 1/32°C/s; [1:0] zero; negative slew warning unsupported. p.30. |
| 0x02 ALERT_STATUS | [15:8] reserved; [7] CRC flag, [6] slew status, [5] slew flag, [4] high status, [3] low status, [2] high flag, [1] low flag, [0] data ready. Status and read-cleared event flags differ. pp.31–32. |
| 0x03 CONFIG | Figure: [15:11] reserved, [10:9] ADC time, [8] reset, [7] AVG, [6] CRC enable, [5] reserved, [4] OS, [3] shutdown mode, [2:0] period. Reset 0x0004. pp.32–33. |
| 0x04 LOW, 0x05 HIGH | [15:2] signed 14-bit limits, 1/32°C; [1:0] zero. Reset −25°C/+85°C; high less than or equal to low disables limit comparison. p.34. |
| 0x06 HYSTERESIS | [15:8] high, [7:0] low hysteresis; unsigned 0.5°C steps. Each threshold has independent hysteresis. p.35. |
| 0x07 SLEW_LIMIT | [14:2] unsigned 13-bit positive slew threshold, 1/32°C/s; [15],[1:0] reserved. p.36. |
| 0x08/09/0A UID1/2/3 | Read-only 48-bit traceability identity, most-significant word first. pp.37–38. |
| 0x0B DEVICE_ID | [15:12] revision, [11:0] DID=0x114; documented reset 0x1114. p.38. |
| 0x10–0x2A | Reserved entries in p.29 map; never infer a readable user feature or core scan range. |

Feature and protocol differences:

- pp.1,3–7,13: four pins only, no ALERT or ADD0; 1.08–1.98 V supply,
  static low-voltage logic. TMP114 A/B/C/D orderables select 0x48–0x4B;
  TMP114N D/C/B/A select 0x4C–0x4F. Selection is factory supplied.
- Page 4 limits SCL/SDA to 1.98 V recommended and 2.1 V absolute maximum.
  A direct 3.3-V bus can damage it. A coexistence fixture needs compatible
  supplies and pullup/input thresholds for every participant or appropriate
  level shifting; an upper pullup limit alone is insufficient.
- pp.13–18,32–36: separate alert status/events, positive slew warning,
  independent high/low hysteresis, selectable ADC times 6.4/3.5/2.0/1.2 ms,
  one-or-eight averaging, and different periods. OS restarts an in-flight
  conversion and returns to shutdown after completion; repeated triggers can prevent
  completion. None maps to TMP11x's MOD[11:10]/CONV/AVG semantics.
- pp.21–28: control-byte bit 7 enables auto-increment; optional CRC changes
  both read and write framing. CRC polynomial 0x07, seed 0xFF, no reflection,
  final XOR zero. CRC-enable takes effect after STOP. Burst CONFIG writes
  have a documented limitation, so TI recommends single-register writes.
  These are not TMP11x register transactions.
- Page 24 states that a non-indexed read returns zero; it names neither 0x0F
  nor a CRC state. The expected 0x0F zero response is inferred from that rule
  and its absence in the p.29 map, using the ordinary CRC-disabled fixture.
  Page 26 documents bad-CRC detection followed by NACK and FFh
  if clocking continues. It does not explicitly establish every missing-CRC
  sequence; transport-error and 0xFFFF adverse fixtures must retain that scope.
- p.27: general-call reset, 30-ms typical serial timeout and I3C mixed-bus
  coexistence exist. Mixed-bus coexistence is not native I3C support and
  must not be transferred to TMP119 without its own evidence.
- pp.29–38 have no EEPROM unlock, general-purpose EEPROM programming,
  temperature offset register, or physical alert output to expose here.

Retain source limitations without trying to implement this excluded family:
p.32's reserved [15:8] table row overlaps its documented fields; the figure
shows [15:11] reserved. p.17's diagram indicates TEMP or ALERT_STATUS clears
ready while p.32 says only ALERT_STATUS. p.36 contains copied TMP126 text
and a mismatched slew-field label. These conflicts reinforce exact-part
review; they must not become TMP11x facts or fake-silicon assumptions.

## TMP118 / TMP118M

| Pointer | Complete register inventory and relevant fields |
| --- | --- |
| 0x00 TEMP | Signed 16-bit, 1/128°C, reset zero. pp.14,26–27. |
| 0x01 CONFIG | [15] OS; [14:13] reserved (reads 11); [12:11] fault count; [10] polarity; [9] reserved zero; [8] shutdown; [7:6] conversion rate; [5] alert flag; [4] reserved (reads one); [3:2] AVG; [1:0] reserved zero. Reset 0x60B0. pp.27–28. |
| 0x02 LOW, 0x03 HIGH | Signed 16-bit, 1/128°C; default +75°C/+80°C. Order is reversed from TMP116/117/119. p.28. |
| 0x0B DEVICE_ID | DID=0x118 in [15:4]; revision is the LOW nibble [3:0]; reset 0x1180. p.29. |
| 0x0C/0D/0E UID0/1/2 | Three 16-bit traceability words, map marked read-only but retrieval requires special procedure. pp.18–19,29–30. |

Feature and protocol differences:

- pp.3–8,22–23: four-pin package, no physical ALERT or ADD0. Factory
  A/B/C/D orderables (including medical variants) select 0x48–0x4B.
  Supply is 1.4–5.5 V, static 1.2-V-compatible logic, and grade/supply
  accuracy ranges differ from TMP119.
- pp.16–17,27–28: AVG offers none, four/eight back-to-back conversions,
  or moving-four average. Alert asserts at `>= THIGH` and clears at
  `< TLOW` after selectable 1/2/4/6 consecutive conversions. POL affects
  the software flag; there is no ALERT pin. Do not import the inclusive
  high comparison, fault queue or moving average into TMP11x.
- pp.20,27: OS is CONFIG bit 15 and applies from shutdown; its readback
  indicates conversion state differently from TMP11x's mode field.
  CONFIG contains no TMP11x DATA_READY/EEPROM_Busy/Soft_Reset fields.
- pp.18–19: UID retrieval first sets shutdown, writes 0x0000 to the desired
  UID pointer, then reads that word, repeating per word. This procedure
  must not appear in TMP11x read-only metadata or diagnostic routes.
- pp.21–25: I2C up to 1 MHz, retained pointer, general-call reset,
  approximately 30-ms timeout and mixed I3C bus coexistence. Undefined
  register reads return zero; writes outside the map are ignored while
  acknowledged. Thus ACK alone proves neither register compatibility nor
  successful configuration.
- The complete map has no TMP11x EEPROM scratch/unlock/programming or
  temperature-offset functionality. No CRC or auto-increment facility is
  described here; do not borrow TMP114's options into TMP118 either.

Known internal mismatch remains outside implementation scope: p.16's note
discusses 64/128-Hz fast settings, while p.27's rate field specifies
0.25/1/4/8 Hz. Do not pick whichever value is convenient or infer a new
TMP11x feature from this excluded-family document.

## Gaps to close in the supported-family promptset

The baseline already excludes TMP114/TMP118 by name. Add the concrete
boundary evidence above to stages 4–5 and explicit negative fixtures to
13,26,32–33:

1. A same-address TMP114/TMP118 responder must not authorize a managed
   CONFIG write. Probe only the supported family's 0x0F ID location;
   transport failure or incompatible content remains failure. Do not
   traverse foreign register maps to identify or configure them.
2. Derive the ordinary-protocol 0x0F zero response from TMP114's non-indexed-read
   rule (p.24) and register map (p.29), and TMP118's corresponding rule/map
   (pp.23/26). The rule sentences do not literally name 0x0F; select CRC-disabled
   TMP114 as fixture policy. Reject before any data-register mutation. Add
   CRC-enabled TMP114 transport-failure/0xFFFF adverse fixtures with no retry;
   distinguish documented bad-CRC behavior from an omitted-CRC assumption.
   Pointer writes used by an attempted read are distinct from data-register
   mutations. A multi-register dump checks 0x0F first and stops on mismatch,
   before reading foreign 0x02 alert status.
3. Invalid/unsupported public model selection rejects locally with zero
   I2C. Raw diagnostics cannot bypass model/pointer checks or invoke
   foreign UID, CRC, burst, fault-queue, or reset behavior.
4. Independently assert that supported-family THIGH is 0x02, TLOW 0x03,
   offset at 0x07 is only TMP117/119, revision nibble for those families
   is high, and read transactions are one pointer plus two data bytes.
5. Add excluded-family/feature rows to FULL_COVERAGE with explicit status,
   instead of treating absence as a missing implementation. Stage 42
   documents that similar names, compatible temperature decoding, and
   overlapping addresses do not authorize hot-swapping these chips.

The probe cannot prove physical identity against an unrelated device that
happens to return compatible bits; BOM selection remains authoritative.
These are planned tests and documentation gates. No excluded-family driver,
hardware operation, CI run, or scope expansion was performed.

## Response — 2026-11-17

Reviewed the two maps, listed protocol differences, internal source conflicts
and all five proposed boundary fixes against `171dbf5`, the current prompts
and pinned SNIS214E/SNIS237A. The register exclusions remain correct; similar
temperature coding and address overlap do not establish compatibility.

| Original finding | Review and applied response |
|---|---|
| 1: Same-address responder must not authorize writes | Correct and already required. Added exact TMP114 electrical/address qualifications in stage 4: only A–D overlap 0x48–0x4B; N D/C/B/A use 0x4C–0x4F. Host mismatch tests do not authorize a 3.3-V physical substitution. |
| 2: Unsupported pointer observations | Partly incorrect for TMP114: p.24 specifies zero for non-indexed locations, just as TMP118 p.23 does. The expected 0x0F response is an inference using the respective register maps, with CRC-disabled TMP114 selected as fixture policy. Added bounded CRC-error fixtures plus ID-first dumps in 26/32. No foreign-ID/UID scan or CRC fallback is permitted. |
| 3: Invalid model/raw bypass | Correct. Invalid model selection remains zero-I2C; first release exposes no arbitrary register-data write API. Diagnostics retain checked pointers and cannot add UID-write/reset/CRC escape paths. |
| 4: Independent supported-family literals | Correct and already assigned to codec/transport/variant fixtures. Stage 32 retains negative controls, THIGH/TLOW order, model-dependent 0x07 and differing ID layouts. |
| 5: Explicit exclusion rows | Already addressed in FULL_COVERAGE. Stage 32 consolidates X03/X08 absence/interference checks; stage 42 still owns final user-facing support boundaries. |
| Internal datasheet mismatches | TMP114 reserved-row overlap, ready-clear wording, copied slew labels and TMP118 rate-note disagreement remain documented source limitations; none authorizes adding these devices to TMP11x. |

No excluded-family driver, physical fixture operation or stronger hardware
identity claim was introduced. The earlier “missing” labels describe the
historical baseline; current prompt coverage is not implementation evidence.

## Follow-up response to the cb73740 review - 2026-11-17

Accepted the source-attribution correction: SNIS214E p.24 gives a general
non-indexed-read rule, not a literal 0x0F/CRC-disabled statement. The p.29 map
and that rule support the expected zero observation as an inference; the
CRC-disabled condition belongs to the fixture. Updated the earlier response
and stage 32 wording accordingly. CRC-enabled adverse cases remain separate,
and no foreign-device probing or physical substitution was performed.
