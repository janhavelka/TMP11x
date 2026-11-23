# Prompt 47 — audit of the library foundation and read path

**COMPLETE.** Stage 18 can consume `audit-10-17`. The audit corrected the host test command for CMake 3.16. The final Release suite and extracted source consumer passed. [Phase d acceptance](#phase-d-acceptance).

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

## Phase b — engineering evidence, 2026-11-23

The current source export passed a clean Release consumer build. Its public
identity API classified every 16-bit word correctly for each of the six BOM
selections. The transport, lifecycle, health, latest-read and codec checks
below found no defect requiring a source or test change. The audit remains
**PARTIAL** pending fresh review and final acceptance.

The entry revision was `2dcb90226e45682cc4b21257ec33cb6738cf2e07` on clean
`main`. Git history and the preceding wrapper request agree on the parent
`0feff8a` and the single phase-a report change. The wrapper verified upstream
equality at 20:30:04 Prague time. Two read-only agents traced stages 12–15
and 16–17 in parallel. The lead checked their evidence against the sources,
test cases and a clean build. The selected sibling build, status and source
blobs still match the phase-a pins.

### Source package and public entry points (10–11)

- Managed PlatformIO Core 6.2.0 packed the checkout with
  `scripts/pio.cmd package pack --output build/p47b/package/TMP11x.tar.gz`
  using stage 10's pinned offline SPDX response. The archive contained exactly
  28 regular files. All matched checkout bytes, including seven public headers,
  nine implementation units and four private headers. Against Git blobs, only
  `LICENSE` differed, by its 21 CRLF line endings in this Windows checkout.
  The license text matched. Archive SHA-256
  `27ea0a3ad6b1212abc4a7646bb2c8f60a97dd9ba465c18769cb97e075d964faf`.
  ESP-IDF 5.5.5 Component Manager 2.5.2 loaded version 0.1.0 in component
  upload mode and its file filter selected the same 28 files. [Package manifest](../../library.json), [IDF manifest](../../idf_component.yml).
- The exported example was copied to a separate application directory. Its
  Release build compiled nine sources from the extracted library and one from
  that application. It linked, exercised public health and register policy,
  printed `0.1.0` and registered zero tests. Each exported public header
  compiled alone in strict C++17 with exceptions and RTTI disabled. Both
  C++ examples in the exported README compiled. [Consumer](../../examples/native_consumer/main.cpp).
- In `build/p47b/version`, independent header and IDF version drift each
  caused both check commands to exit 1. Neither check changed bytes or
  timestamps. Sync repaired each mirror and reproduced a missing header byte
  for byte. A subsequent sync left both timestamps intact. [Generator](../../scripts/generate_version.py), [delegated check](../../tools/check_version_sync.py).
- `value_types_cpp` exercised all 256 byte values for model, address and
  selectors with its allocation trap active under `-DNDEBUG`. Invalid models
  preserved the caller's capabilities. The public query and register policy
  both use the descriptor selected by the BOM. The ID engine recognizes the
  observed group from the same table. [Model descriptor](../../src/ModelDescriptor.h), [value cases](../../test/test_value_types.cpp).

### Transport, identity, binding and health (12–15)

| Check | Observed result |
| --- | --- |
| Register callbacks and write permission | For six models and 256 pointers, the suite checked a read and all three write purposes. Each allowed frame carried its own pointer. The byte cases included `0F → A1 17` and `02 12 A5`. On the four valid addresses the short budget clipped the grant to 3 ms and the larger budget to the 50 ms callback cap. A one-byte response left the caller's previous word intact. Prohibited EEPROM writes and missing guards issued zero callbacks. [Register cases](../../test/test_register_transport.cpp). |
| Family identification through the extracted library | A separate public-header caller supplied all 65,536 ID words to each BOM selection. For all 393,216 calls it checked one combined read of 0x0F, the raw word, selected target, recognized group, revision validity/value and returned status. Totals stayed zero and state stayed UNINIT. TMP116/N accepted exactly 0x1116. The other four accepted all 16 revisions of base 0x117. Capability and pointer 0x07 checks distinguished four protected shadows from three shadows plus guarded offset. The named TMP114/TMP118 fixtures also checked that mismatch ends the probe. [ID engine](../../src/Identity.cpp), [negative responders](../../test/test_identity.cpp). |
| Binding and passive health | Rejected rebind preserved previous settings and evidence. Accepted rebind, end and unbind cleared observations with zero callbacks. Independent routes to the same address stayed separate. In each model's five-transfer health trace, the third fault wrapped to tick 0. An ordinary success at tick 1 recovered READY with two successes, three failures and the prior fault retained. The instance remained uninitialized. Saturation, neutral diagnostics and local refusals passed. [Binding cases](../../test/test_binding.cpp), [health cases](../../test/test_health.cpp). |

### Temperature, latest reads and CONFIG (16–17)

The exact-part TEMP, CONFIG and limit pages listed in phase a support the
signed 1/128 °C scale, the MOD=10 continuous alias and active-high POL=1.
The separate calculation used big-endian signed decoding for the 11 literal
words and decimal rounding for the inverse vectors. The native codec
round-tripped all 65,536 words, including 0x8000 at −256 °C. A latest read
of that word carried no freshness claim. [Temperature tests](../../test/test_temperature.cpp).

The deadline fixture exercised 12 wrapped callback cases with complete and
short responses, plus expiry before transfer. At equality after a complete
TEMP read, the engine retained the raw word and physical success, DATA_READY
was consumed, and the caller and last-good cache kept their old samples. A
short read preserved the adapter failure and supplied no complete word.
Expiry before transfer preserved DATA_READY and the callback count.
[Latest-read cases](../../test/test_latest.cpp).

Independent field combinations produced 512 steady CONFIG words per model,
all with `word & 0xF803 == 0`. The native suite checked all 3,072 profiles,
both limit words, every raw MOD value and each bit 9:2 for drift. MOD=11
matched desired shutdown only with a confirmed owned trigger. The CONFIG
observation retained raw status and the mode-dependent read-clear mask.
The nonfactory reset fixture compared against the programmed profile.
[CONFIG cases](../../test/test_configuration.cpp).

### Phase-b verification and handoff

| Commands | Result |
| --- | --- |
| `cmake -S . -B build/p47b/host -G Ninja -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, `cmake --build build/p47b/host --config Release --parallel 4`, `ctest --test-dir build/p47b/host -C Release --output-on-failure` | PASS, 14/14. CMake/CTest 4.0.1, Ninja 1.12.1, GNU C++ 15.1.0, Python 3.12.10. |
| `cmake -S "build/p47b/isolated app" -B build/p47b/consumer -G Ninja "-DTMP11X_SOURCE_DIR=<absolute path to build/p47b/source export>" -DTMP11X_BUILD_TESTS=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Python3=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, build and run `tmp11x_native_consumer.exe`, then `ctest --test-dir build/p47b/consumer -C Release -N` | PASS, both discovery flags unused, linked version 0.1.0, zero tests. Local `build/p47b/public_identity.cpp` also linked against this consumer's `tmp11x/libTMP11x.a` and passed the six-model word sweep. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and clean whitespace. |

Phase c owns a fresh reread of prompts 10–17 and two parallel read-only
reviews of the cumulative change from `cac1b93`. The phase-b checks cover
host behavior and package contents. Stages 34–36 own adapter links and
timing, 40–41 own fixture evidence, and 43 owns the remote CI checkpoint.
Stage 18 still requires closure of audit 47.

## Phase c — independent review, 2026-11-23

**PARTIAL.** Two parallel read-only reviews covered `cac1b93..a2ff94a`.
The lead reread prompts 10–17 in full and checked the findings against the
cumulative diff, current source, tests and original tables. At entry, clean
`main` and local `origin/main` both resolved to
`a2ff94a168c7bbd68df10096236da0da7492137d`. Its parent, subject and sole
changed path matched the phase-b request. The wrapper verified upstream
equality at 20:48:23 Prague time.

| Review scope | Finding and disposition |
| --- | --- |
| 10–13, package through identity | The host guide advertises CMake 3.16 but used `ctest --test-dir`, introduced in 3.20. On 3.16–3.19 that option cannot select the configured test tree. The host guide and verification checklist now use `cmake -E chdir build/host ctest`. The checklist also selects Release for configure, build and test. The 3.16 manual documents `chdir`. [Host guide](../testing.md), [verification checklist](../prompts/VERIFICATION.md), [CMake 3.16 manual](https://cmake.org/cmake/help/v3.16/manual/cmake.1.html#run-a-command-line-tool), [3.20 release notes](https://cmake.org/cmake/help/v3.20/release/3.20.html#ctest). |
| 14–17, lifecycle through CONFIG | Bind validates before replacing state. Diagnostics preserve tracked health, and a complete incompatible identity clears live trust while preserving the historical sample. At deadline equality the latest-read engine retains physical status and any complete word while preserving the caller and cache. The CONFIG comparator accepts MOD=10 for continuous and requires confirmed trigger ownership for MOD=11 with desired shutdown. Source and tests agreed with the contracts. [Binding](../../src/TMP11x.cpp), [deadline cases](../../test/test_latest.cpp), [CONFIG cases](../../test/test_configuration.cpp). |

The lead rechecked the pinned TMP116 pp. 24–26, 30–31, TMP117 pp. 25–27,
31–32 and TMP119 pp. 32–34, 38–39. The register maps, signed TEMP scale,
POL sense and continuous alias agree with the literal test expectations.
The TMP119 map and ID detail still give different reset words. The public
identity sweep accepted every documented base-ID revision for each of the
four TMP117-like selections and only 0x1116 for TMP116/N. Both reviewers
finished. The documentation correction was the only confirmed finding.

| Phase-c check | Result |
| --- | --- |
| `cmake -S . -B build/host -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build/host --config Release --parallel 4` | PASS on the existing MinGW host build with CMake 4.0.1 and GCC 15.1.0. |
| `cmake -E chdir build/host ctest -C Release -R "^register_transport_cpp$" --output-on-failure`, then repeat without the filter | PASS, 1/1 focused and 14/14 full from the checkout root. Execution used CTest 4.0.1. Minimum-version command support was verified against the 3.16 manual. |
| Run `build/p47b/consumer/tmp11x_native_consumer.exe` and `build/p47b/public_identity.exe`. Compare each regular file in the phase-b archive with its checkout bytes using Python `tarfile`. | PASS, consumer printed 0.1.0, all 393,216 model/ID observations passed, and all 28 archive files matched. |
| `python -B scripts/check_references.py`, `python -B scripts/check_promptset.py`, `git diff --check` | PASS, 14 pinned PDFs, 55 stages and clean diff whitespace. |

Phase d owns final acceptance for prompts 10–17, the report index and the
progress checkpoint. Adapter links, wire timing, fixture evidence and the
remote CI checkpoint retain the owners recorded in phase b.

## Phase d acceptance

Final acceptance on 2026-11-23 covered `cac1b93..2a1a19b`. The clean
entry revision was `2a1a19b415c4b001a1d722c0d4d5e0975220fe87` on
`main`. Its parent `a2ff94a`, subject and three documentation paths matched
the phase-c request. The wrapper recorded equality with `origin/main` at
21:01:11 Prague time. Both phase-c reviews are reconciled above.

| Acceptance check | Result on Windows |
| --- | --- |
| `cmake -S . -B build/p47d/host -G Ninja -DTMP11X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, then `cmake --build build/p47d/host --config Release --parallel 4` | PASS. Fresh build with GCC 15.1.0, Ninja 1.12.1 and CMake 4.0.1. All nine library units compiled as C++17 with strict warnings and `-Werror`. |
| `cmake -E chdir build/p47d/host ctest -C Release --output-on-failure` | PASS, 14/14 with CTest 4.0.1. The documented transport filter also passed 1/1. The identity, binding, health, temperature, latest and configuration filter passed 6/6. |
| Compile each public header alone and both README C++ examples with `g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fno-exceptions -fno-rtti`. Run `doxygen -` over `include/TMP11x` with XML output and `WARN_AS_ERROR=YES`. | PASS, seven headers and two examples. Doxygen 1.13.2 generated the public API index, including the binding, health and CONFIG functions. |
| `python -B scripts/generate_version.py check` and `python -B tools/check_version_sync.py`, then independent drift and repair in `build/p47d/interface-check/version` | PASS with Python 3.12.10. Both checks detected each stale mirror and the missing header without changing file bytes or timestamps. Generator `sync` restored the checked-in bytes. A second sync preserved timestamps. |
| `scripts/pio.cmd package pack --output build/p47d/package-check/TMP11x.tar.gz` using the pinned offline SPDX response from stage 10. Compare the archive, checkout and IDF upload selection. | PASS with managed PlatformIO Core 6.2.0. All 28 expected files matched checkout and phase-b payloads byte for byte. ESP-IDF 5.5.5 Component Manager 2.5.2 selected the same paths and version. Phase-d archive SHA-256 `60b627c75a40cad42024da8772f6b042122a4090448df698c5fef42b495e5584`. |
| Repeat the phase-b consumer configure/build under `build/p47d/package-check` with the standalone example and fresh extraction. Run `consumer/tmp11x_native_consumer.exe`, inspect the compile database and run `cmake -E chdir build/p47d/package-check/consumer ctest -C Release -N`. | PASS. With tests off and Python/Git discovery disabled, the build compiled nine extracted library sources and the separate application. The consumer printed `0.1.0` and CTest listed zero tests. The public identity harness linked against this build and passed all 393,216 model/word cases. |

The transport fixture checked the exact three write bytes `02 12 A5` and
the two ID response bytes `A1 17`. Bind and teardown used zero callbacks.
A tracked success restored READY from OFFLINE for all six BOM selections.
At deadline equality, a complete TEMP read counted as a physical success
and retained its raw word while preserving the caller's output and last-good
sample. A short read counted as a physical failure. An expired request
before dispatch left the fake's DATA_READY flag intact. [Transport cases](../../test/test_register_transport.cpp), [health cases](../../test/test_health.cpp), [deadline cases](../../test/test_latest.cpp).

The signed codec round-tripped all 65,536 words. Independent signed-byte
and decimal calculations matched all 11 wire and 14 inverse vectors. All
3,072 steady CONFIG encodings cleared `0xF803`. The pinned CONFIG tables
confirm active-high POL=1 and the MOD=10 continuous alias on TMP116 p. 26,
TMP117 p. 27 and TMP119 p. 34. TMP119 pp. 32 and 39 still give different
default ID words. The ID therefore establishes family compatibility only.
[Literal vectors](../registers/behavior-vectors.json), [CONFIG cases](../../test/test_configuration.cpp), [source manifest](../reference/manifest.json).

With Python 3.12.10, `scripts/check_references.py` verified all 14 pinned
PDFs and `scripts/check_promptset.py` passed for all 55 stages. Their
regression suites passed all nine and 36 cases, respectively.
Stage 18 owns guarded initialization and its public integration tests.
Adapter links and timing (34–36), physical qualification (40–41),
installed-package qualification (42) and remote CI (43) remain open.
The silicon uncertainties listed in phase a remain recorded for those
operations that depend on them. [Stage 18](../prompts/18_safe-initialize.md), [verification layers](../prompts/VERIFICATION.md).
