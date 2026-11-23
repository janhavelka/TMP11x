# Prompt 47 — audit of the library foundation and read path

## Phase a — brief and inputs, 2026-11-23

**PARTIAL.** All eight prerequisite stages have individual COMPLETE rows and
all 14 consumed artifacts are available. The audit asks whether an application
can bind a selected device, interpret a read and account for its effects before
initialization is implemented. Stage 18 requires the completed `audit-10-17`
report before it can add initialization writes. [Audit mandate](../prompts/47_audit-10-17.md),
[sequence](../prompts/SEQUENCE.json), [handoffs](../prompts/HANDOFFS.md).

The clean entry revision was `0feff8aff639116e4166ba9ce20140a8354760e3`
on `main`, with zero divergence from local `origin/main`. Its parent
`8c39261`, subject and four documentation paths agree with the previous
handoff and commit request. The wrapper recorded upstream equality at
09:49:02 Prague time. Review the cumulative changes from `cac1b93`, the
last accepted revision before stage 10, through this entry revision.

### Requirements and evidence at entry

| Stage and closure commit | Consumed artifacts and implementation paths | What the audit must establish |
| --- | --- | --- |
| [10](prompt_10.md) · `1014fed` | `build-foundation`, `version-tools`, `host-test-entrypoint` in `CMakeLists.txt`, `library.json`, `idf_component.yml`, `scripts/generate_version.py`, `tools/check_version_sync.py` and [host instructions](../testing.md) | Reproduce the generated header, detect drift without modifying either mirror, inspect the package file list and link the extracted source with tests off and Python/Git discovery disabled. |
| [11](prompt_11.md) · `d585d4a` | `value-types` in `include/TMP11x/{Status,Config,TMP11x}.h`, `src/Config.cpp` and `src/ModelDescriptor.h` | Compile public headers in isolation and exercise invalid selectors, static status messages and allocation-free value handling. Trace all six BOM selections to the same capability table used by transport and identity. |
| [12](prompt_12.md) · `29a3257` | `register-transport`, `command-table`, `register-fake` in `src/RegisterTransport.cpp`, `include/TMP11x/CommandTable.h`, `src/CommandTable.cpp` and `test/support/FakeTmp11xDevice.cpp` | Compare literal address, pointer and MSB-first bytes with the ledger. Check one callback per transfer, clipped timeout, preserved output on a short read and protected writes including TMP116 EEPROM3 at 0x07. |
| [13](prompt_13.md) · `d797162` | `identity-engine` in `src/Identity.cpp` and `test/test_identity.cpp` | Check full-word 0x1116 and masked 0x117 with separate revision. A completed mismatch retains the observed word. A failed read preserves prior evidence. Include TMP114/TMP118 negative responders and zero follow-up writes. |
| [14](prompt_14.md) · `1434060` | `binding-api`, `public-identity-api` in `include/TMP11x/TMP11x.h`, `src/TMP11x.cpp` and `test/test_binding.cpp` | Trace accepted and rejected rebinds, end and unbind, and two routes to the same address. Check zero lifecycle callbacks and one health-neutral ID read on a bound, uninitialized instance. |
| [15](prompt_15.md) · `564d30f` | `transport-health` in `src/RegisterTransport.{h,cpp}`, `src/TMP11x.cpp` and `test/test_health.cpp` | Reconcile physical outcomes against callback counts, including saturation, timestamp zero/wrap, neutral diagnostics, local refusals and an ordinary tracked success from OFFLINE. |
| [16](prompt_16.md) · `6d92595` | `temperature-codec`, `latest-read-engine` in `src/Temperature.cpp`, `src/LatestRead.cpp` and `test/test_{temperature,latest}.cpp` | Derive signed counts, limits and rounding independently. At deadline equality and wrap, check physical success, consumed readiness, caller output and the last-good cache separately. A latest read alone supplies no fresh-conversion proof. |
| [17](prompt_17.md) · `0feff8a` | `config-codec` in `include/TMP11x/Configuration.h`, `src/Configuration.cpp` and `test/test_configuration.cpp` | Check all steady profiles against literal masks, polarity and signed limits. Trace continuous MOD aliasing and owned-trigger provenance. Status, reset and reserved bits must stay out of steady writes. |

### Source boundaries

The following one-based pages were checked in the local TI PDFs. For each
model, compare the command table and independent test expectations with the
source. Preserve both TMP119 ID defaults (0x0117 and 0x2117) and the
distinction between family compatibility and a BOM grade. [Manifest](../reference/manifest.json),
[register ledger](../registers/tmp11x.json), [literal behavior vectors](../registers/behavior-vectors.json).

| Datasheet | Signed format / wire figures | Register map / TEMP / CONFIG / limits | EEPROM lock / pointer 0x07 / ID |
| --- | --- | --- | --- |
| TMP116/N · SBOS740A | 11 / 22 | 24 / 25 / 26–27 / 28 | 29 / 30 / 31 |
| TMP117/M/N · SNOSD82D | 13 / 22–23 | 25 / 26 / 27–28 / 29 | 30 / 31 / 32 |
| TMP119 · SNIS236 | 15 / 25 | 32 / 33 / 34–35 / 36 | 37 / 38 / 39 |

TMP114 SNIS214E p. 38 and TMP118 SNIS237A p. 29 place their identity at
0x0B. Keep the single 0x0F probe and family-confidence limits. Open silicon
questions include the flag-clear edge, TEMP byte latching, partial writes,
MOD readback around one-shot, same-value CONFIG writes and TMP116 POR BUSY.
Keep those uncertainties visible in the fake and its tests. [Exclusion evidence](../reference/extracted/excluded-families-audit.md),
[device behavior](../device-behavior.md), [EEPROM policy](../eeprom-and-reset-policy.md).

Across three clean sibling checkouts, 36 selected paths matched their pinned
blobs. TMP1x2 (`afd5088`, local `0791429`) supplies the CMake, version,
callback and lifecycle precedents. OPT4001 (`441ef57`, local `f17d494`)
supplies the isolated consumer and identity examples. ADS1115 (`e07622a`,
local `1aab07b`) supplies transfer guards and mask comparisons. Stage 10
records the layout-only template at `b574041`; its sibling checkout was
unavailable. The exact-part TI tables govern the register and codec tests.
[Source adoption](../source-adoption.md), [source map](../prompts/SOURCE_MAP.md).

### Baseline checks and next slice

- `cmake --build build/host --config Release --parallel 4` and
  `ctest --test-dir build/host -C Release --output-on-failure` passed all
  14 registrations in the existing Release MinGW build (CMake/CTest 4.0.1,
  GCC 15.1.0). This is the entry baseline for the audit.
- `python -B scripts/check_references.py` verified all 14 pinned PDFs by
  path, size and SHA-256 with Python 3.12.10.
- `python -B scripts/check_promptset.py` passed for 55 stages. A separate
  check matched eight closure commits and COMPLETE rows to the report,
  found all 14 input IDs and resolved its 20 local links.

**Next slice, phase b.** Start with stages 10–11. Compare the release source
export with the current public files, run the isolated tests-off consumer,
and trace the capability table against six independent model expectations.
Record commands, paths and discrepancies here. Then cover stages 12–17.
In phase c, reread the eight original prompts and run two parallel read-only
reviews, one on stages 10–13 and one on 14–17. The lead checks findings
against source and repeats affected tests. Phase d repeats the originating
acceptance checks. Adapter timing and electrical evidence belong to the MCU
and physical stages. Stage 43 owns the remote CI checkpoint.
