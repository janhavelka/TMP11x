# 16 — Implement signed temperature and threshold codecs

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [5](5_register-ledger.md), [6](6_timing-status.md), [11](11_status-values.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md), [14](14_binding-lifecycle.md), [15](15_passive-health.md).
Produces: `temperature-codec`, `latest-read-engine`.
Consumes: `register-ledger`, `behavior-vectors`, `value-types`, `register-transport`, `binding-api`, `transport-health`.

## Objective and boundaries

Deliver exact 16-bit two's-complement °C conversion and pure arithmetic tests independent of physical freshness.

## Source audit

Check TI TEMP/limit tables; examine TMP1x2 `src/TMP1x2Measurement.cpp` for public sample structure but discard 12/13-bit shifted and extended-mode decoding. Compare ADS1115 signed code tests.

## Complete implementation work

- Implement raw/count to degrees Celsius using `counts / 128.0` and inverse encoding with explicit rounding, finite input checking, representable limits and a stable error policy. Decode two's-complement bits using representable signed arithmetic; C++17 conversion of out-of-range uint16_t to int16_t is implementation-defined. The exact arithmetic range is -256 to +255.9921875 degrees Celsius, distinct from the model's specified measurement/accuracy range. Check the rounded range before any potentially overflowing float-to-integer conversion.
- Separate pure conversion from `Sample` trust: `0x8000` is physically reset sentinel until confirmed conversion, yet remains a valid signed 16-bit code arithmetic representation.
- Implement `readLatestSample()`/`readTemperature()` or agreed names as distinct latest-register observations; do not populate `freshConversion=true` on a simple read. Use prompt 14's real binding state and prompt 15's tracking policy; any normal API requiring initialization remains guarded until prompt 18 exists, with private transport/codec tests here rather than a fake initializer.
- Add timestamp and last-sample cached copies only after successful read/decoding. Failed calls leave output arguments untouched or follow documented partial-output semantics.
- Validate signed extremes, negative fractional values, NaN/Inf, out-of-range input, rounding tie conditions, and no clock availability.

## Verification and exit criteria

Run independently derived vectors: `0x0C80=25°C`, `0xF380=-25°C`, `0xFF80=-1°C`, `0xFFFF=-0.0078125°C`, `0x8000=-256°C raw`. Verify latest-read freshness remains unproven.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run unrelated sibling suites or remote CI (except stage 43).

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_16.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on `main`
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
