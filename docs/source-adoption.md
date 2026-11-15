# Source adoption and attribution

Prepared 2026-11-15. This records actual imports separately from design
precedents. All referenced sibling checkouts remain read-only. Source
revision and implementation evidence are in [library-comparison.md](library-comparison.md).

## Files adopted during preparation

| TMP11x file | Source at pinned revision | Treatment |
|---|---|---|
| `scripts/pio.cmd` | `janhavelka/TMP1x2`, `afd50889312f6e218a6a34eca687e3e8547d8b40`, `scripts/pio.cmd` | Copied unchanged. Finds the current user's existing VS Code-managed PlatformIO; does not install another Core. |
| `LICENSE` | Same TMP1x2 revision, `LICENSE` | Copied unchanged: MIT, copyright (c) 2026 THYMOS Solution s.r.o., Jan Havelka, with original website/contact and permission notice. |
| `.gitignore` | Same TMP1x2 revision, `.gitignore` | Adapted development-artifact exclusions to this repository. This is not a runtime source import. |
| `AGENTS.md` | Same TMP1x2 revision, `AGENTS.md`; compact structure also compared with `janhavelka/24Cxx` at `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | Reused general CI/Git conventions; adapted scope/navigation and project guidance to TMP11x. Detailed stage policy remains in the promptset. |
| `CLAUDE.md` | Same TMP1x2 revision, `CLAUDE.md` | Standard shared-instruction import, matching TMP1x2, OPT4001, ADS1115 and 24Cxx. No second rulebook. |

`.gitattributes` is repository-specific. Agent entry conventions were aligned
with the siblings on 2026-11-15; TMP11x-specific guidance and promptset revisions
retain this repository's own contracts. `CLAUDE.md` imports `AGENTS.md` rather
than maintaining another independent copy. No production C++ headers,
implementation, sibling reference archive or hardware logs have been copied
during preparation.

The entry-file comparison used clean local checkouts, each matching its observed
remote HEAD: TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`, 24Cxx
`1bd9f969d1df55ca5819b0d290ff331869e7d5bc`, OPT4001
`441ef579d7fa3f7bca7665f52632fdaddd306300` and ADS1115
`e07622a99fe9f13c5d6a5a2497a8112a14185b9d`. Broader sibling guidelines were
checked for ownership, framework boundaries and documentation practice; their
chip behavior, lifecycle differences, release commands and machine-specific
tool paths were not copied into TMP11x instructions.

The MIT notice covers original/adapted library and tooling code. Vendor PDFs
and any separately licensed reference material retain their own notices;
placing them in this repository does not relicense them under MIT. Their
source URLs, revisions and hashes belong in `docs/reference/manifest.json`.

## Selected future reuse

These are decisions for later stages, not claims that the corresponding
TMP11x files already exist.

| Source | Decision | Required adaptation and evidence |
|---|---|---|
| TMP1x2 `include/TMP1x2/Status.h` and `Config.h` | Adapt | Preserve compact typed status, static strings, terminal injected callbacks and user contexts. Change namespace, models, errors and configuration fields after prompt 8. Record exact copied source and retain the MIT notice. |
| TMP1x2 `src/TMP1x2.cpp` health/lifecycle concepts | Adapt | Preserve passive OFFLINE, bus-silent binding/end and health-neutral preconditions. Test callback count, zero-bus APIs, failure outputs, counter saturation and recovery explicitly. |
| TMP1x2 `TMP1x2Configuration.cpp` | Adapt concept; rewrite chip behavior | Preserve desired/observed/trusted distinctions and evidence after a write may have committed. Replace masks, ordering, state transitions and verification reads from exact TMP11x evidence; account for read-to-clear flags and EEPROM safety. |
| TMP1x2 `TMP1x2Operations.cpp` | Adapt bounded owner/result model | One operation, no hidden task or retry; clip callback budgets and retain terminal result. Rewrite phases for TMP11x EEPROM checks, model gates and conversion readiness. |
| TMP1x2 `test/test_core.cpp`, `test/jobs/test_jobs.cpp`, `test/variants/test_variants.cpp` | Adapt test structure | Keep independent fake silicon, failure-at-each-transfer tests and registered cases. Derive TMP11x literals from TI tables, not copied production constants. |
| OPT4001 readiness bookkeeping | Adapt concept | Test diagnostic reads consuming DATA_READY and stale/unknown conversion evidence. Do not import FIFO/counter/CRC assumptions. |
| ADS1115 owner example and dirty-state tests | Adapt concept | Application owns lock, repeated START, controller timeout and recovery attempts. Keep platform APIs out of portable core. |
| TCA9548A ownership/route guidance | Adopt integration boundary | Application serializes route selection through STOP and sensor transfer. No mux dependency or bus recovery code inside TMP11x. |
| MB85RC retained-result and ambiguous-write tests | Adapt concept | Preserve observable possible hardware effect and result consumption. Do not import memory chunks, caller-buffer lifetime model or writable storage API. |
| Build/version/CLI/HIL tooling | Select individually in prompt 2/10 and later stages | Inspect current files, license and real registrations. Rename package identity, restrict exports and validate behavior; never copy a sibling's entire tool/reference tree. |

## Explicitly rejected imports

- TMP1x2 register map, 12/13-bit temperature format, extended-mode settling,
  conversion-rate values and TMP112D address selection: different silicon.
- TMP1x2 `BusOperations.h/.cpp` general-call reset and SMBus alert-response
  helpers: excluded from the first TMP11x interface.
- OPT4001 offline latch and power-down write from `end()`: conflict with
  TMP11x passive health and bus-silent teardown.
- Automatic model selection based only on a secondary driver's Device ID
  convention: exact TMP11x identity/capability evidence must decide it.
- EEPROM/FRAM write, unlock, persistence, retry or reconciliation machinery
  from memory libraries: first-release TMP11x nonvolatile access is read-only.
- Framework adapters, scheduler tasks, logging, GPIO or mutex ownership in
  the portable driver: application/example responsibility.
- Linux GPL source or vendor reference snippets without compatible license:
  comparison evidence only, not production MIT source.

## Updating this record

For every later source import, record repository, full commit, original path,
destination, license/copyright, modifications, and the test that establishes
the new semantics. Distinguish a reused design idea from copied text/code.
No package consumer may require a sibling checkout or the TI PDF archive.
