# Producer/consumer handoffs

This inventory describes **planned deliverables**, not completed work.
`SEQUENCE.json` declares each stage's `produces` and `consumes` IDs; the same
lines appear in every numbered prompt. Each ID has one initial producer.
Later stages may extend that artifact, but must preserve its contract or update
all affected consumers, tests and documentation together.

The checker verifies unique producers, known inputs, dependency reachability
and agreement with this table and the numbered prompts. It cannot prove that
an implementation exists, that a test is meaningful or that a fixture passed.
At execution, inspect actual artifacts and their required evidence.

## How to use a handoff

- Required input producers must be reachable through `depends_on`. Numeric
  order alone is not an artifact dependency. Do not add a later-stage
  prerequisite to hide an early impossible promise; move the actual work or
  assign a clear earlier producer.
- Check inputs at the required evidence level. A stage can have usable source
  while its MCU link is blocked; that source may be consumed independently.
  Do not mark the producer COMPLETE with required checks missing, or treat
  a source handoff as proof of a binary/hardware result.
- Research stages produce documents, literal vectors and policies. Code stages
  produce working scoped behavior with tests. A private seam may be tested
  before its public lifecycle integration; that integration has a named owner.
  Unimplemented public operations cannot return invented success.
- When adding a feature, update the existing shared engine and test entrypoint.
  Report extensions to earlier artifacts explicitly; do not create a second
  register table, replay algorithm, parser, version authority or test runner.
- Each stage report names input revision/path, produced artifacts, executed
  checks, forward integration obligations with owner, and remaining evidence.
  A future-owner obligation does not count as an already executed test.

## All 44 handoffs

| Stage | Produced IDs | Usable output and boundary |
|---:|---|---|
| 1 | `ecosystem-audit` | `docs/library-comparison.md` and `docs/source-adoption.md`; source revisions/license decisions for 2/8. Research, no copied driver. |
| 2 | `tooling-plan` | `docs/toolchain-and-package-plan.md`; selected build/layout/test tools for 10/34/35/42. Installed SDK discovery is not a firmware build. |
| 3 | `ti-archive` | Pinned PDFs/manifest, review notes and public-code catalog; exact sources for 4–7. Optional inaccessible example code does not block intake. |
| 4 | `variant-contract`, `board-guide` | `docs/variants.md` and initial `docs/board-integration.md`; model/grade and board facts for 5/8 and independent 34/35 integration. |
| 5 | `register-ledger`, `feature-ledger`, `register-validator` | `docs/registers/tmp11x.json`, `docs/feature-coverage.md`, `tools/check_register_ledger.py`; one fact/policy inventory for 6–7 and all implementation owners. |
| 6 | `behavior-contract`, `behavior-vectors` | `docs/device-behavior.md` and `docs/registers/behavior-vectors.json`; source-backed traces for 16–23/32. No pre-driver executable test claim. |
| 7 | `safety-policy` | `docs/eeprom-and-reset-policy.md` and refined stage 5 access/reset policy; guards/exclusions for 8/18–26. |
| 8 | `api-contract` | `docs/api-contracts.md` and architecture; one-shot/binding/settings/failure contracts for all code. Declarations alone are not implementations. |
| 9 | `owner-contract` | `docs/owner-operations.md`; immutable binding, clock, admission/result/cancellation rules for 18/21/28–31. |
| 10 | `build-foundation`, `version-tools`, `host-test-entrypoint` | Real CMake library/test skeleton, scripts/generate_version.py, tools/check_version_sync.py and docs/testing.md; later stages add tests to this host entrypoint. |
| 11 | `value-types` | Pure Status/Config/model/value types and single capability table for 12–17. No I2C or device-success placeholders. |
| 12 | `register-transport`, `command-table` | Private checked word transport and include/TMP11x/CommandTable.h for 13/14/17. Public unbound guards wait for real lifecycle 14. |
| 13 | `identity-engine` | Private identity engine for 14/18 using 12's binding view. Public identity/probe integration belongs14. |
| 14 | `binding-api`, `public-identity-api` | Real bus-silent bind/end/unbind and public bound identity/probe for 15/16/18. Job-slot lifecycle tests hand off to28–30. |
| 15 | `transport-health` | Tracked transport health for 16/18/31; public identity/probe stays health-neutral. Full initialization integration lands18. |
| 16 | `temperature-codec`, `latest-read-engine` | Signed codecs plus latest-read engine/cache for 17/18/21. Managed API remains guarded until real initialization 18; private seams are tested now. |
| 17 | `config-codec` | Pure CONFIG codecs, stable comparison and profile-value validation using 16; no duplicate register/capability table. Physical apply belongs18. |
| 18 | `safe-apply-engine`, `sync-initialize-recover` | Real bounded synchronous begin/recover, safe apply/readback and minimum desired/dirty state for 19/22/23/29; offset availability/trust remains explicit until23. |
| 19 | `config-trust` | Observed configuration/snapshots/invalidation/trust around18's engine; typed convenience setters belong20/22 and active-job integration28. |
| 20 | `conversion-settings` | Mode/cycle/AVG wrappers and timing helpers over18/19; execute applicable6 vectors. Shutdown transition and one-shot baseline feed21. |
| 21 | `sync-acquisition` | Real latest/try-fresh/owned-one-shot/blocking acquisition over16; shutdown-only managed one-shot policy. Supplies23/27/30. |
| 22 | `limits-alert` | Typed limit/alert wrappers and observations over existing18/19 apply; no second limits engine. Supplies26/27/30/38. |
| 23 | `offset-feature` | Guarded offset APIs plus extension of the shared full-profile engine; freshness integration uses21. Supplies25/27/29, raw/CLI gates later26/38. |
| 24 | `readonly-eeprom` | Read-only EEPROM-shadow APIs and shared write denials; existing-path tests now. Public optional raw/dump integration belongs26, CLI37/38. |
| 25 | `device-reset` | Guarded addressed reset/refusal and nonfactory reset/recovery tests using 23 offset. Supplies27/31/38. |
| 26 | `diagnostics` | Bound-only diagnostics, ordered snapshots and optional guarded raw route using prior safety policy. No raw-write API is forced by24. |
| 27 | `sync-api` | Complete synchronous public API/snapshots and compile consumer; reconcile feature coverage for 28/33/34–38. |
| 28 | `job-lifecycle` | Real private admission/result machinery and lifecycle/invalidation integration; unsupported unconnected kinds reject. Real public job execution lands29/30. |
| 29 | `configuration-jobs` | Initialize/configure/recover jobs reuse18/19/23 phases with immutable binding; execute lifecycle cases against real jobs. |
| 30 | `measurement-jobs` | Latest/try-fresh/one-shot jobs reuse21, including shutdown-only policy; execute lifecycle and measurement interleaving cases. |
| 31 | `fault-closure` | Adversarial fault closure of existing synchronous/cooperative recovery and transport/result policies; not the first recovery implementation. |
| 32 | `variant-fixtures` | Consolidated independent variant fixtures built incrementally since 12; literal source vectors and explicit unknown assumptions. |
| 33 | `native-qualification` | Native production qualification and existing registered suite;37–39 must extend it when CLI/runner tests are added. |
| 34 | `arduino-adapter-source`, `arduino-build-record` | Arduino source adapter/example and initial build record; app firmware/session identity and board guide extension. Final CLI image integration belongs38. |
| 35 | `idf-adapter-source`, `idf-build-record` | Independent IDF source adapter/example and initial build record; app identity and guide extension. No Arduino SDK prerequisite; final CLI image belongs38. |
| 36 | `p4-consumer-source`, `p4-build-record` | P4 consumer source and build record using 35; no physical board needed for linking. Missing SDK/target remains explicit. |
| 37 | `cli-baseline`, `cli-protocol` | Shared observation CLI, docs/cli-protocol.md and golden transcripts in test/cli; complete grammar but only implemented help/routes. Host tests independent of MCU SDKs. |
| 38 | `cli-complete`, `firmware-cli-handoff` | Complete CLI/jobs/help, standard-host tests and docs/firmware-cli-handoff.md; integrate available34/35 sources and refresh links, or record exact pending target gates. |
| 39 | `hil-runner` | Python runner and unit/subprocess tests against the actual37/38 fixture, registered in standard host checks; uses the frozen protocol and firmware handoff. |
| 40 | `readonly-hil-record` | Read-only physical evidence or explicit blocked record. Requires authorized fixture and one current CLI-integrated image from 34 OR35 after38. |
| 41 | `volatile-hil-record` | Volatile/restore/soak evidence or blocked record. Actual physical execution requires40's valid fixture/read-only qualification, not merely a report file. |
| 42 | `package-qualification`, `package-doc-tools` | Package/relocation/coexistence/docs qualification and tools/check_package.py + tools/check_doxygen.py; consumes available firmware sources separately from target link evidence. |
| 43 | `ci-record` | Exact-SHA remote CI record including current CLI/Python suites and final firmware sources; absent access remains blocked, no physical port by default. |
| 44 | `final-audit` | Final row-by-row software/evidence audit; inspect36/40/41/43 records when available and list missing gates. No unconditional physical/CI dependency. |

## Conditional integration and evidence gates

Stages 34/35 own independent firmware backends. Stage 37 freezes the shared
wire protocol; stage 38 integrates its final commands/identity into available
backends and records full-link results or missing gates in the firmware
handoff. These are conditional source/build integrations, not mandatory SDK
dependencies for host CLI/runner development. Stages 42/43 refresh affected
links if later changes invalidate the recorded images.

Stage 40 has an **OR** gate: one actual current Arduino or IDF image containing
the completed stage 37/38 protocol/routes, plus runner compatibility and an
authorized fixture. A pre-CLI example build, a missing-build record or an
unmatched library version is insufficient. Stage 41 needs actual stage 40
fixture qualification before physical mutation; a blocked report never grants
that qualification.

Stage 44 may review software while P4, hardware or remote CI evidence is absent.
It must name those missing gates. This does not convert missing evidence to
PASS or permit claiming an older green SHA covers later code.

The named post-stage tools are: register validator from 5, version generator/
checker from 10, and package/Doxygen checkers from 42. Stages 37–39 append CLI
and runner tests to the standard host entrypoint established at 10; 43 executes
that current inventory. Preparation checks do not invoke tools before creation.
