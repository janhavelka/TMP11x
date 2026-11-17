# Sibling source map — read-only GitHub references

These are **review targets**, not downloaded files or claims of identical
behavior. The preparation [comparison](../library-comparison.md) and
[toolchain audit](../toolchain-and-package-plan.md) record reviewed revisions.
At execution, check remote HEAD and compare with the pinned source paths and
implementations/callers/tests. For source ports, preserve LICENSE and
attribution. A copy of a driver file without its semantics is not reuse.

| Priority | GitHub repository and exact source locations | Adopt / adapt / reject |
|---|---|---|
| 1 | https://github.com/janhavelka/TMP1x2 — `include/TMP1x2/{Config,Status,CommandTable,TMP1x2,Version}.h` | Copy/adapt `Status` shape, injected I2C callbacks, strongly typed configuration, owner operation vocabulary. Rewrite chip constants/model validation. |
| 1 | TMP1x2 — `src/TMP1x2.cpp`, `src/TMP1x2Configuration.cpp`, `src/TMP1x2Measurement.cpp`, `src/TMP1x2Operations.cpp` | Inspect state, health, dirty evidence, config replay, one-shot and retained results. Adapt only matching behavior; rewrite temperature encoding, timing and status semantics. |
| 1 | TMP1x2 — `test/{test_core.cpp,variants/test_variants.cpp,jobs/test_jobs.cpp,bus/test_bus.cpp}`; `CMakeLists.txt` | Adapt register model, native tests, exhaustive signed vectors, cooperative deadlines and framework-free consumer gates. |
| 1 | TMP1x2 — `examples/common/Tmp1x2Cli*`, `examples/esp_idf/basic/*`, `examples/01_basic_bringup_cli/*` | Reuse parser/framework adapter patterns and CLI checks; rewrite commands/read-to-clear status behavior. |
| 1 | TMP1x2 — `docs/{feature-coverage.md,integration.md,owner-operations.md,testing.md,code-structure.md}`, `tools/check_contracts.py`, `tools/hil_tmp1x2_runner.py`, `scripts/pio.cmd`, `scripts/generate_version.py`, `.github/workflows/ci.yml` | Source for contracts and verification; no blind CLI/HIL/CI copy. |
| 2 | https://github.com/janhavelka/OPT4001 — `include/OPT4001/{Config,OPT4001,CommandTable}.h`, `src/OPT4001.cpp`, `test/test_basic.cpp` | Device-ID handling, read-to-clear freshness, `Sample`, conversion-ready and one-shot semantics. Use TMP11x-specific silicon. |
| 2 | https://github.com/janhavelka/ADS1115 — `include/ADS1115/Config.h`, `src/ADS1115.cpp`, `examples/02_owner_safe_poll/main.cpp` | Managed configuration, readback, partial-write trust and owner integration. |
| 2 | https://github.com/janhavelka/24Cxx — `README.md`, `include/EEPROM24Cxx/*`, `docs/integration.md` | EEPROM caution/explicit maintenance precedent. **No EEPROM programming for initial TMP11x.** |
| 2 | https://github.com/janhavelka/MB85RC — `README.md`, `include/MB85RC/*` | Variant sizes, operation/result retention, isolated package checks. |
| 2 | https://github.com/janhavelka/TCA9548A — `AGENTS.md`, `examples/common/*` | Multiplexer ownership/route sequencing; do not integrate mux control into the sensor driver. |
| 3 | https://github.com/janhavelka/esp32-platformio-library-template — `AGENTS.md`, `include/YourLibrary/*`, `CMakeLists.txt` if present, `library.json`, `scripts/generate_version.py`, `.github/workflows/ci.yml` | Scaffolding and metadata only. |
| 3 | https://github.com/janhavelka/FieldCore-node — relevant I2C owner, module, service and tests identified by search | Read-only consumer reference; **never** a compiled dependency. |
| Workflow only | https://github.com/janhavelka/DP8382x — `docs/prompts/*` | Source-ledger, numbered prompts, shared contracts, single remote-CI stage and hardware qualification gating. |
| Workflow only | https://github.com/janhavelka/MotorControl-CAN — `docs/prompts/{START_HERE,COMMON,CONVENTIONS,SEQUENCE,PROGRESS}.md` or `.json`, numbered prompts | Prompt execution/closure, incremental reports, agents and no-false-evidence policy. |

Reviewed workflow/toolchain source revisions are recorded in the
[toolchain plan](../toolchain-and-package-plan.md); runtime API/adoption
revisions are in the [library comparison](../library-comparison.md) and
[adoption record](../source-adoption.md). These are dated observations;
refresh the relevant sources before coding. Older audit SHAs describe their
own inputs, not competing claims about the current HEAD.
Record the *actual* fetched revision, license and source path at each adoption;
HEAD state and working-tree changes cannot be inferred from an old hash.

## GitHub access procedure for a fresh checkout

- `git remote -v` identifies the **new TMP11x** remote; do not assume it exists.
- `git ls-remote https://github.com/janhavelka/TMP1x2.git HEAD` checks public read;
  authenticated GitHub connector may be needed for private repositories.
- Clone sibling repositories to a **temporary external directory**, never
  inside `src/` or the published package, only if the needed revision is not
  already available. Existing sibling worktrees remain read-only, including
  their Git metadata: no fetch, checkout or pull there. Use `git ls-remote`
  for the remote and `git -C <sibling> rev-parse HEAD`/`status` locally.
- For borrowed MIT code: inspect the source LICENSE and header attribution,
  copy selected code only, adapt thoroughly, preserve original copyright,
  add local provenance in `docs/source-adoption.md` and test new behavior.
- Do **not** copy TI application code or Linux kernel GPL code into the MIT
  driver simply because it is accessible. Treat those as reference evidence.
