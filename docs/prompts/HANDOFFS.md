# Producer/consumer handoffs

This inventory describes **planned deliverables**, not completed work.
`SEQUENCE.json` declares each stage's `produces` and `consumes` IDs; the same
lines appear in every numbered prompt. Each ID has one initial producer.
Later stages may extend that artifact, but must preserve its contract or update
all affected consumers, tests and documentation together.

The checker verifies unique producers, known inputs, dependency reachability
and agreement with this table's Produced IDs column and numbered declarations.
Coverage-ID/owner and PDF-page checks are structural only. It cannot prove that
an implementation exists, that a test is meaningful or that a fixture passed.
At execution, inspect actual artifacts and their required evidence.

## How to use a handoff

- Required input producers must be reachable through `depends_on` and precede
  the consumer in the manifest. Do not add a later-stage
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

## Handoffs in execution order

Exact consumers are in SEQUENCE; descriptions below explain boundaries rather
than duplicating every transitive consumer list. There are 61 produced IDs and
264 declared input links after the 2026-11-18 follow-up review.

| Stage | Produced IDs | Usable output and boundary |
|---:|---|---|
| 1 | `ecosystem-audit` | `docs/library-comparison.md` and `docs/source-adoption.md`; source revisions/license decisions; consumers: see SEQUENCE. Research, no copied driver. |
| 2 | `tooling-plan` | `docs/toolchain-and-package-plan.md`; selected build/layout/test tools; consumers: see SEQUENCE. Installed SDK discovery is not a firmware build. |
| 3 | `ti-archive` | Pinned PDFs/manifest, review notes and public-code catalog; exact sources; consumers: see SEQUENCE. Optional inaccessible example code does not block intake. |
| 4 | `variant-contract`, `board-guide` | `docs/variants.md` and initial `docs/board-integration.md`; model/grade and board facts; consumers: see SEQUENCE. Stages 34/35 extend the guide independently. |
| 5 | `register-ledger`, `feature-ledger`, `register-validator` | `docs/registers/tmp11x.json`, `docs/feature-coverage.md`, `tools/check_register_ledger.py` and `tools/test_check_register_ledger.py`; one fact/policy inventory, with standalone regression command before stage 10 registers it. |
| 6 | `behavior-contract`, `behavior-vectors` | `docs/device-behavior.md` and `docs/registers/behavior-vectors.json`; source-backed traces; consumers: see SEQUENCE. No pre-driver executable test claim. |
| 7 | `safety-policy` | `docs/eeprom-and-reset-policy.md` and refined stage 5 access/reset policy; guards/exclusions; consumers: see SEQUENCE. |
| 45 | `audit-1-7` | `docs/reports/prompt_45.md` records independent findings, scoped fixes and repeated checks for prompts 1–7. |
| 8 | `api-contract` | `docs/api-contracts.md` and architecture; one-shot/binding/settings/failure contracts for all code. Declarations alone are not implementations. |
| 9 | `owner-contract` | `docs/owner-operations.md`; immutable binding, clock, admission/result/cancellation rules; consumers: see SEQUENCE. |
| 46 | `audit-8-9` | `docs/reports/prompt_46.md` records independent findings, scoped fixes and repeated checks for prompts 8–9. |
| 10 | `build-foundation`, `version-tools`, `host-test-entrypoint` | Real CMake library/test skeleton, scripts/generate_version.py, tools/check_version_sync.py and docs/testing.md; registers stage 5's validator and regressions, then later stages add tests to this host entrypoint. |
| 11 | `value-types` | Pure Status/Config/model/value types and single capability table; consumers: see SEQUENCE. No I2C or device-success placeholders. |
| 12 | `register-transport`, `command-table`, `register-fake` | Private checked transport, include/TMP11x/CommandTable.h and initial reusable test/support/FakeTmp11xDevice.{h,cpp}; literal ledger-derived test behavior, extended by later owners. Public unbound guards integrate at stage 14. |
| 13 | `identity-engine` | Private identity engine using stage 12's binding view. Public identity/probe integration belongs to stage 14. |
| 14 | `binding-api`, `public-identity-api` | Real bus-silent bind/end/unbind and public bound identity/probe. Stage 18 extends bind profile validation; job-slot lifecycle tests belong to stages 28–30. |
| 15 | `transport-health` | Tracked transport health independently of initialized state; identity/probe stays health-neutral. Initialization integration belongs to stage 18. |
| 16 | `temperature-codec`, `latest-read-engine` | Signed codecs plus latest-read engine/cache; consumers: see SEQUENCE. Managed API remains guarded until real initialization 18; private seams are tested now. |
| 17 | `config-codec` | Pure CONFIG codecs, stable comparison and profile validation using stage 16; no duplicate capability table. Physical apply belongs to stage 18. |
| 47 | `audit-10-17` | `docs/reports/prompt_47.md` records independent findings, scoped fixes and repeated checks for prompts 10–17. |
| 18 | `safe-apply-engine`, `sync-initialize-recover` | Real bounded begin/recover over resumable apply/readback phases, full available bind validation and desired/dirty state; offset management arrives at stage 23. |
| 19 | `config-trust` | Shared observed configuration/trust and retained destructive-read evidence around stage 18's engine; setters extend it, stage 28 adds job ownership. |
| 48 | `audit-18-19` | `docs/reports/prompt_48.md` records independent findings, scoped fixes and repeated checks for prompts 18–19. |
| 20 | `conversion-settings` | Mode/cycle/AVG wrappers, explicit shutdown and timing helpers over stages 18/19; execute stage 6 vectors and provide a verified continuous baseline. |
| 21 | `sync-acquisition` | Resumable latest/continuous-fresh/owned-one-shot acquisition; explicit blocking kinds, guarded shutdown baseline and no MOD-only freshness proof. |
| 49 | `audit-20-21` | `docs/reports/prompt_49.md` records independent findings, scoped fixes and repeated checks for prompts 20–21. |
| 22 | `limits-alert` | Typed limit/alert wrappers over stages 18/19; bus-silent consumption of retained alert evidence, no duplicate limits engine. |
| 23 | `offset-feature` | Opt-in guarded offset management extends the full-profile engine; unmanaged calibration remains observed only. Freshness uses stage 21; diagnostics/CLI integrate at stages 26/38. |
| 24 | `readonly-eeprom` | Read-only EEPROM-shadow APIs and shared write denials; diagnostics integrate at stage 26 and CLI at stages 37/38. |
| 25 | `device-reset` | Guarded addressed reset/refusal with unresolved effects retained; nonfactory reset/recovery tests use stage 23's managed/unmanaged offset policy. |
| 26 | `diagnostics` | One bound-only diagnostic route and identity-gated ordered dumps; all CONFIG reads retain evidence. No public raw register-data write API. |
| 27 | `sync-api` | Complete synchronous public API/snapshots and compile consumer; reconcile feature coverage; consumers: see SEQUENCE. |
| 50 | `audit-22-27` | `docs/reports/prompt_50.md` records independent findings, scoped fixes and repeated checks for prompts 22–27. |
| 28 | `job-lifecycle` | Real private admission/result component and lifecycle integration; unrelated I2C is BUSY while active/result pending. Unconnected public kinds reject; stages 29/30 connect real engines. |
| 29 | `configuration-jobs` | Initialize/configure/recover reuse stages 18/19/23 phases with immutable binding; execute lifecycle cases against real jobs. |
| 30 | `measurement-jobs` | Latest/try-fresh/one-shot reuse stage 21; execute lifecycle/interleaving cases and operation-local guards. |
| 31 | `fault-closure` | Adversarial fault closure of existing synchronous/cooperative recovery and transport/result policies; not the first recovery implementation. |
| 32 | `variant-fixtures` | Consolidated independent variant fixtures built incrementally since 12; literal source vectors and explicit unknown assumptions. |
| 33 | `native-qualification` | Native correctness and source-consumer qualification; stages 37–39 extend the registered host suite with CLI/runner tests. |
| 51 | `audit-28-33` | `docs/reports/prompt_51.md` records independent findings, scoped fixes and repeated checks for prompts 28–33. |
| 34 | `arduino-adapter-source`, `arduino-build-record` | Arduino fixed demo/byte-I/O adapter, identity, attempt counters and build record. Stage 38 attaches final CLI; resumed backend work refreshes the current handoff. |
| 35 | `idf-adapter-source`, `idf-build-record` | Independent IDF fixed demo/byte-I/O adapter, identity, counters and build record. No Arduino dependency; stage 38 attaches CLI, resumed backend work refreshes it. |
| 36 | `p4-consumer-source`, `p4-build-record` | P4 consumer source and build record using 35; no physical board needed for linking. Reuses an existing stage-42 coexistence sample if resuming later; missing SDK/target remains explicit. |
| 52 | `audit-34-36` | `docs/reports/prompt_52.md` records independent findings, scoped fixes and repeated checks for prompts 34–36. Retains any missing P4 source or target link evidence. |
| 37 | `cli-baseline`, `cli-protocol` | Shared observation CLI, docs/cli-protocol.md and golden transcripts in test/cli; complete grammar but only implemented help/routes. Host tests independent of MCU SDKs. |
| 38 | `cli-complete`, `firmware-cli-handoff` | Complete CLI/jobs/help, host tests and docs/firmware-cli-handoff.md; attach available stage 34/35 adapters and refresh links, retaining pending target gates. |
| 39 | `hil-runner` | tools/hil_tmp11x_runner.py and host unit/subprocess tests against the actual stage 37/38 fixture; concrete physical target/scope validation, optional physical-mode pyserial. |
| 53 | `audit-37-39` | `docs/reports/prompt_53.md` records independent findings, scoped fixes and repeated checks for prompts 37–39. |
| 40 | `readonly-hil-record` | Read-only physical evidence or blocked record. Requires authorized fixture and one current CLI-integrated stage 34 or 35 image after stage 38. |
| 41 | `volatile-hil-record` | Volatile/restore/soak evidence or blocked record; physical work requires stage 40's actual fixture qualification, not merely a report. |
| 54 | `audit-40-41` | `docs/reports/prompt_54.md` records independent findings, scoped fixes and repeated checks for prompts 40–41. Requires actual stage 40/41 physical evidence; final closure can proceed while this audit is blocked. |
| 42 | `package-qualification`, `package-doc-tools` | Package/relocation/coexistence/docs qualification and tools/check_package.py + tools/check_doxygen.py; reuses actual stage-36 coexistence source if available, otherwise creates the shared sample independently. Target links remain separate evidence. |
| 55 | `audit-42` | `docs/reports/prompt_55.md` records independent findings, scoped fixes and repeated checks for prompt 42. |
| 43 | `ci-record` | Exact-SHA remote CI record including current CLI/Python suites and final firmware sources; absent access remains blocked, no physical port by default. |
| 44 | `final-audit` | docs/reports/prompt_44.md audits all rows and available stage 36/40/41/43 evidence. After CI-covered fixes, run stage 43's procedure within this stage before closing; missing gates remain explicit. |

## Conditional integration and evidence gates

Stages 34/35 own independent firmware backends. Stage 37 freezes the shared
wire protocol; stage 38 integrates its final commands/identity into available
backends and records full-link results or missing gates in the firmware
handoff. These are conditional source/build integrations, not mandatory SDK
dependencies for host CLI/runner development. Stages 42/43 refresh affected
links if later changes invalidate the recorded images.
If stage 34/35 resumes after stage 38, the resumed backend owner integrates the
current shared CLI and updates `docs/firmware-cli-handoff.md` and its link record.

Stage 40 has an **OR** gate: one actual current Arduino or IDF image containing
the completed stage 37/38 protocol/routes, plus runner compatibility and an
authorized fixture. A pre-CLI example build, a missing-build record or an
unmatched library version is insufficient. Stage 41 needs actual stage 40
fixture qualification before physical mutation; a blocked report never grants
that qualification.

Stage 52 audits the required Arduino and ESP-IDF sources and records whatever
P4 evidence exists. Stage 54 audits actual physical evidence from both hardware
stages. Stage 44 may review software while the P4, hardware or remote CI evidence
is absent, and reviews the physical audit record when available.
It must name those missing gates. This does not convert missing evidence to
PASS or permit claiming an older green SHA covers later code.

The named post-stage tools are: register validator from 5, version generator/
checker from 10, and package/Doxygen checkers from 42. Stages 37–39 append CLI
and runner tests to the standard host entrypoint established at 10; 43 executes
that current inventory. Preparation checks do not invoke tools before creation.
