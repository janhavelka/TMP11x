# Prompt handoff consistency audit — 2026-11-15

**Audit complete; implementation stages remain unexecuted.** Reviewed all 44
prompts, dependency metadata and shared contracts for missing producers,
premature consumers, overlapping ownership and incomplete integration gates.
There is still no production TMP11x driver in this repository.

Input: clean `main` at `5a0cf90d89843936e573267734d7316db48afc96`, tracking
`origin/main` at `https://github.com/janhavelka/TMP11x.git`. Fetch/check found
no incoming changes. Three independent read-only reviewers covered 1–17,
18–33 and 34–44, then re-reviewed the integrated changes and checker. The lead
owned all edits, integration, report and Git work. No sibling was modified.

## Corrections to stage connections

| Boundary | Gap and resulting handoff |
|---|---|
| 12/13 → 14 | Transport and public identity required binding that did not exist yet. Stages 12/13 now deliver tested private engines; 14 attaches real public bound identity/probe and unbound guards. |
| 14 → 16/17/18 | Binding implicitly needed future profile codecs. Stage 14 validates binding structure; 18 validates the accepted sensor profile before I2C. |
| 5/6/7 → implementation | Research promised executing driver tests before a build existed. Stage 6 produces named source-backed behavior vectors/traces; later owners execute them. Stages 6/7 refine the same ledger and run its validator. |
| 18 → 19/22/23/29/31 | Later stages used synchronous recover without an explicit producer. Stage 18 now delivers real begin/recover, safe apply/readback and minimum desired/dirty state; 19 refines trust, 29 adds jobs and 31 hardens faults. |
| 19 → 20/22 | Several prompts independently claimed the same setting/apply implementation. Stage 19 owns shared trust/readback; 20/22 add typed wrappers over the existing engine; 23 extends it with offset. |
| 8/11/18/21 → 23 | A default-zero offset in an early Config could be ignored or make all initialization fail before offset support. The configurable member/control appears in 23; early successful initialization verifies CONFIG/limits and preserves unknown offset provenance. Stage 23 connects every accepted offset value to actual apply/readback/recovery. |
| 16 → 17; 21 → 23 | CONFIG/profile validation and offset freshness consumed codecs/acquisition outside their dependency closure. Added the two missing prerequisite edges and mirrored prose/index/SEQUENCE. |
| 8/9 → 21/30/38 | One-shot could leave desired continuous versus hardware shutdown inconsistent. One shared library policy now requires desired shutdown before admission and verifies hardware shutdown; owned MOD=11 is transient, cancellation never silently restores/retriggers. This is policy, not a newly claimed silicon limitation. |
| 8/9 → 28/29 | Config jobs could copy model/address/callback/clock fields into a live binding. Freeze immutable binding versus sensor settings; mismatches reject before I2C/token commitment, and deliberate changes require rebind. |
| 14/19 → 28–30 | Early lifecycle/invalidation promises lacked a later executable integration owner. Stage 28 connects active/retained slots; 29/30 repeat the cases against real engines. |
| 23/24 → 26/37/38 | EEPROM/offset stages demanded future raw/dump/CLI tests. They now test existing shared guards; 26 owns optional raw/dump integration and 37/38 the actual CLI routes. Optional raw writes are not forced into the API. |
| 5/10/12/42 → verification/CI | Named tools/headers lacked concrete producers. Assign register validator to 5, CommandTable.h to 12, version generator/checker and host test entrypoint to 10, package/Doxygen checkers to 42. |
| 4 → 34/35 → 42 | Firmware examples referenced an unproduced board guide. Stage 4 creates docs/board-integration.md, independent Arduino/IDF stages extend it, and 42 finalizes it. |
| 34/35 → 37/38 → 39 | Runner identity/session expectations lacked a wire-contract producer. Adapters own firmware/boot identity; 37 freezes protocol/version/schema and golden transcripts; 38 connects final firmware metadata/routes; 39 tests the actual compiled fixture. Library version is separate from firmware identity. |
| 37 → 38 | Complete help and both-framework parity were required before mutation/jobs or possibly SDKs existed. Stage 37 freezes grammar and advertises only implemented routes; 38 completes commands/help and available-backend integration. Host work remains independent of MCU SDK availability. |
| 34/35/38 → 40 | HIL could accept an old pre-CLI example image. Stage 38 owns a concrete firmware handoff and refreshed links; stage 40 requires one current compatible CLI-integrated image plus fixture, with Arduino-or-IDF semantics. |
| 33 → 37–39 → 43 | The early native suite could omit later CLI/Python tests from CI. Each later stage registers its new tests in the standard host entrypoint; 43 explicitly executes them and links final firmware sources. |

Stage 25's earlier dependency on 23 remains. No prompts were reordered,
removed or renumbered. All numbered prompts now expose their handoff IDs;
the [inventory](../prompts/HANDOFFS.md) explains each usable output and boundary.
Shared AGENTS/COMMON/START_HERE/CONVENTIONS/ENGINEERING/CLI/VERIFICATION/index
instructions were reconciled with those owners and conditional gates.

## Persistent handoff validation

[SEQUENCE.json](../prompts/SEQUENCE.json) now declares **60 uniquely produced
artifact IDs** and **226 required-input links** across all 44 stages. Every
numbered prompt repeats its `Produces`/`Consumes` declarations. The checker
also compares the 44-row HANDOFFS inventory and rejects:

- Missing, duplicate, malformed or multiply owned artifact IDs.
- Consuming a future stage, one's own output or an undeclared producer.
- Consuming an earlier producer outside the transitive prerequisite graph.
- Drift between metadata, numbered prose and the handoff inventory.
- Duplicate JSON keys and malformed numeric table cells instead of silently
  overwriting contracts or crashing on non-ASCII digits.

Independent checker review reproduced the non-ASCII numeric-cell crash; it
and duplicate-key loss now have regression coverage. Valid transitive inputs
remain allowed. Mandatory artifact dependencies do not turn conditional MCU
or physical evidence into universal prerequisites. A blocked build record is
not a firmware image; a blocked HIL report is not fixture qualification.

These checks validate declared structure. They do not prove promised files
exist or that an implementation meets their semantics; the stage owner still
checks actual artifacts, source revisions and required tests. A report or a
previous PASS never substitutes for changed source/build evidence.

## Validation and remaining work

Python 3.12.10 checks on the integrated working tree passed. The driver sequence
itself was not invoked, and no driver/MCU/HIL/remote-CI PASS is claimed.

- `python scripts/check_promptset.py`: PASS, all 44 stages, dependencies,
  producer/consumer declarations, prose/inventory parity and local links.
- `python scripts/test_check_promptset.py`: PASS, 27 regression cases,
  including 13 added handoff/parser cases and the 14 existing regressions.
- `python scripts/check_references.py`: PASS, all 14 PDF paths/sizes/SHA-256
  unchanged.
- `git diff --check`, staged whitespace and scoped diff review: PASS.
- Driver C++ tests, MCU links, remote CI and physical HIL: **NOT RUN**.

Stage status remains 1–3 COMPLETE and 4–44 NOT STARTED. Unresolved datasheet
questions and physical evidence from the prior audits remain owned obligations;
this audit fixes how their future work connects. Commit/upstream synchronization
is recorded in the handoff, without inventing a self-referential report SHA.

## Response to each boundary — 2026-11-17

Reviewed each boundary above against the promptset at
`171dbf58251e8606d3b643aa1ceddcc7e921c906`. The historical changes remain
useful, but structural reachability never proved a complete semantic handoff.
The [later audit](../prompts/audit.md) records individual claim/fix decisions.
No stage is executed or promoted to COMPLETE by these corrections.

| Earlier boundary | Review response |
|---|---|
| 12/13 → 14 | Correct and retained: private transport/identity precedes real binding. The shared reusable register fake now starts at 12, so subsequent private tests have an explicit producer. |
| 14 → 16/17/18 | Correct sequencing, incomplete final bind contract. Stage 14 validates available binding structure; stage 18 extends bind to full profile validation with the codecs and preserves the previous binding on rejection. |
| 5/6/7 → implementation | Correct: research vectors are inputs, not executed driver tests. Their validators and regression commands need explicit production/registration ownership; stages 5/10 provide it. |
| 18 → 19/22/23/29/31 | Correct producer, incomplete engine shape. Stage 18 now requires resumable single-callback phases and explicit wait state so stage 29 can reuse actual logic rather than rewrite blocking code. |
| 19 → 20/22 | Correct and retained: shared trust/apply plus typed wrappers. Every CONFIG observation now feeds retained alert evidence; duplicate apply engines remain forbidden. |
| 8/11/18/21 → 23 | Correct staged availability but incomplete default ownership. Offset arrives at 23 as opt-in management; default initialization preserves programmed calibration and managed zero is explicit. Full-profile trust means every owned field, not implicitly owned offset. |
| 16 → 17; 21 → 23 | Correct dependency edges, retained. Current metadata additionally names capability/ledger inputs and the shared fake where relevant rather than relying solely on implicit stage order. |
| 8/9 → 21/30/38 | The desired-shutdown policy is valid; MOD readback as shutdown/completion proof was not. The operation now explicitly establishes guarded shutdown and a stale-readiness baseline, with hardware DATA_READY evidence for completion. Undocumented MOD readback is retained as uncertainty. |
| 8/9 → 28/29 | Correct immutable-binding rule. Accepted bind installs its validated target; first-write target commitment applies to settings changes, not to binding. Invalid rebind preserves prior state. |
| 14/19 → 28–30 | Correct staged integration. Early stages document policy without unused job scaffolding; 28 tests its actual private transition component and 29/30 repeat with real jobs. A private test seam does not inherently require fake production job kinds. |
| 23/24 → 26/37/38 | Correct forward-owner distinction. Shared capability/access policy has one owner; later diagnostic/CLI routes consume it. Optional raw-write surface is not needed to prove feature coverage. |
| 5/10/12/42 → verification/CI | Correct named producers. Clarified validator regressions, one version-generator/check implementation, the tests-OFF option, shared fake and stage 43 ownership of an initial remote workflow. Preserve any pre-existing automatic triggers. |
| 4 → 34/35 → 42 | Correct and retained: board guide begins with exact-part facts, gains backend details, and is finalized for packaging. A guide is not physical qualification. |
| 34/35 → 37/38 → 39 | Correct separation of library version, firmware identity and protocol. Golden transcripts and the compiled fixture must match; a source/build record alone is not a usable image. |
| 37 → 38 | Correct conditional SDK/source integration and honest help. Host grammar tests remain independent of MCU availability; final routes require their real consumer implementation. |
| 34/35/38 → 40 | Correct OR gate and current-image requirement. Fixture identity, wiring, port/image match and authorization are concrete prerequisites; a missing-build record or outdated image cannot satisfy them. |
| 33 → 37–39 → 43 | Correct later-suite registration and exact-final-SHA requirement. A native PASS at 33 never covers subsequent CLI/Python changes or an MCU image that was not relinked. |

The historical 60/226 counts describe this audit's original checkpoint.
Use current SEQUENCE/HANDOFFS for the revised dependency inventory. The checker
still proves declared structure and local links only; source semantics,
datasheet facts, useful tests, MCU linking and physical behavior require their
separate evidence layers. The current corrections do not qualify hardware or
claim an unexecuted implementation test.
