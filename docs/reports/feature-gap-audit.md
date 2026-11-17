# Datasheet and functionality gap audit — 2026-11-15

**Promptset completeness review: COMPLETE. Sensor implementation: NOT STARTED.**
The repository has no production driver to test. This audit checks the planned
implementation against all five pinned chip datasheets and fills missing
requirements; it does not execute stages 4–44 or qualify hardware.

Input: clean `main` at `8d2ffe385f061575f4fa5402060ef63d2deaa579`, synchronized
with `origin/main`, `https://github.com/janhavelka/TMP11x.git`. Fetch/check at
entry found no incoming changes. Three reviewers owned disjoint per-family
notes; the lead integrated prompt/shared-contract/report changes and Git.
An independent integration pass checked the new requirements and stage order.

## Complete source inventories

| Reviewed source | Audit artifact | Scope and applicability |
|---|---|---|
| TMP116 SBOS740A | [TMP116/N inventory](../reference/extracted/tmp116-coverage-audit.md) | All 47 pages, both grades, every register/field, conversion/alert/interface behavior, electrical/thermal/package conditions and exclusions |
| TMP117 SNOSD82D | [TMP117/M/N inventory](../reference/extracted/tmp117-coverage-audit.md) | All 50 pages, grade/package distinctions, every field and function, accuracy conditions and unspecified behavior |
| TMP119 SNIS236 | [TMP119 inventory](../reference/extracted/tmp119-coverage-audit.md) | All 48 pages, complete map/functionality, electrical/application conditions and unresolved TMP119M orderable scope |
| TMP114 SNIS214E and TMP118 SNIS237A | [Excluded-family inventory](../reference/extracted/excluded-families-audit.md) | Complete register/field/protocol and functional comparison; explicit incompatible-family test obligations, no support expansion |

Original table images were checked for critical field/footnote interpretations;
each note records page references. The 14 archived PDFs remain unchanged.
Technical revision dates and newer appended orderable-table dates are distinct.

The new [mandatory coverage inventory](../prompts/FULL_COVERAGE.md) contains
18 register/field rows, 20 behavior/integration rows and eight exclusion rows.
Stage 5 must preserve these IDs in the actual checked ledger/coverage artifact,
splitting rows where needed by model or field. Every row has owners and expected
evidence; application-owned features and deliberate exclusions remain visible.
Every shared contract and all 44 stage scopes were reconciled against it.

## Gaps filled

| Gap | Added requirement and owning stages |
|---|---|
| A broad feature list could conceal individual omissions | Explicit TEMP/CONFIG field/limits/unlock/shadow/offset/ID/reserved-pointer coverage, model disposition and source/test ownership; 4–8, 12–13, 26, 32–33, 44 |
| Interface limits absent from transport requirements | ADD0 strap mapping, 1–400 kHz, no SCL drive, upper pointer nibble, retained pointer, one-word access, ACK/NACK ending, STOP/START and hardware timeout separate from software deadlines; 4–6, 9, 12, 34–36, 40–42 |
| Recovery could cover CONFIG while forgetting limits/offset | Entire desired/observed/trusted profile, non-atomic apply order, every-field readback, partial failures/cancellation and nonfactory reset values; 8, 18–19, 22–23, 25, 27, 29–33 |
| Averaging/alert edge tests were too broad | Nonoverlapping blocks, partial-average abort, all 32 cycle/AVG combinations, alert latch/re-arm, all eight routing/polarity/mode combinations, offset saturation/no double application; 6, 20–23, 30, 32–33, 41 |
| Future fakes could invent undocumented silicon guarantees | Explicit unknowns for same-value/in-flight writes, shutdown flag retention, partial-word commit, TEMP byte coherence and offset/comparator ordering; conservative policy distinct from source and physical evidence; 6, 19–23, 29–32, 41, 44 |
| Register-level reset metadata missed field differences | CONFIG MOD1 is not EEPROM-stored; stable bits, commands/status and programmed reset values are separate; arbitrary shadow contents do not form a serial-validity test; 5, 17, 24–25, 32 |
| Exact grade and application conditions were underspecified | TMP117M operating range, voltage/temperature envelopes, accuracy test conditions, open-drain ALERT, decoupling, exact-package thermal-pad advice, traffic/self-heating/strain/settling; 4, 34–36, 40–42 |
| Excluded functions could disappear from completeness claims | Visible ARA/arbitration, broadcast synchronization, NVM persistence/shadow-write and board-ownership dispositions; no raw/CLI escape; 7, 22–26, 33, 38, 42 |
| Excluded-family tests were only generic unrelated responders | Named TMP114/TMP118 fixtures, different maps/ID bits/protocols, no foreign ID/UID probing or mutating UID procedure; 4, 13, 26, 32–33 |
| API/CLI coverage could drift from the silicon inventory | Row-by-row mapping to typed API/readback, jobs/commands or justified not-applicable responsibility; 8, 27, 33, 38–39, 44 |

The 32 changed numbered prompts are 4–9, 12–13, 17–27, 29–30, 32–36 and
38–44 excluding 43. Prompts 1–3, 10–11, 14–16, 28, 31, 37 and 43 retained
their existing stage scope; shared COMMON/FULL_COVERAGE obligations still apply.
No prompt was added, removed or renumbered. Stage 25 adds prerequisite 23 so
its offset reset/recovery tests have an implemented feature; prose, index and
SEQUENCE are synchronized. Stage 33 tests existing core paths; CLI negative
tests belong to 38, and cooperative offset replay belongs to 29 after 23's
synchronous implementation. This avoids dependencies on future stages.

## Source contradictions and unresolved questions

Newly recorded discrepancies include TMP116's +150 versus +125 °C conditions,
EEPROM reset-value conflicts on all three families, TMP119 summary accuracy
versus detailed electrical specifications, stale shutdown-current prose and
TMP116's ambiguous ALERT grounding sentence. A TMP119 1-MHz characterization
curve does not authorize operation beyond the 400-kHz interface specification.
See [SILICON](../prompts/SILICON.md) and the original-page citations in each
inventory for both sides and the bounded policy.

Earlier TMP116 POR-busy, TMP119 ID-default, EEPROM-word-count and reset/one-shot
wording conflicts remain. TMP119M appears in an accuracy table but still lacks
an established exact orderable mapping; no model was silently added. The
inaccessible E2E DATA_READY race lead remains an evidence question, not a
verified workaround. No guessed answer or simulated behavior closes these
questions. Owners must bound claims or obtain further primary/physical evidence.

## Reference design checks

Re-read TMP1x2's `src/TMP1x2Configuration.cpp` apply/readback/recover engine,
`src/TMP1x2Operations.cpp` staged desired-profile handling and public contracts,
plus ADS1115's desired-profile recovery handling. Their relevant source
revisions and licensing were pinned in the [previous audit](promptset-audit.md)
and [library comparison](../library-comparison.md). Full-profile replay is a
useful ownership precedent; their register behavior is not copied into TMP11x.
The device-specific requirements come from the exact TI PDFs. Siblings were
read-only; no sibling build, fetch, edit, CI dispatch or Git mutation occurred.

## Validation and limits

Python 3.12.10 checks on the integrated working tree validate prompt structure,
source integrity and local tooling, not silicon behavior:

- `python scripts/check_promptset.py`: PASS, all 44 stages, ordered dependency
  DAG, prose/index parity, shared contracts and local links.
- `python scripts/test_check_promptset.py`: PASS, 14 regression cases.
- `python scripts/check_references.py`: PASS, all 14 PDF paths/sizes/hashes.
- `python scripts/test_check_references.py`: PASS, nine integrity/restore cases.
- Inventory check: PASS, exactly 46 unique ordered R/F/X rows and 32 changed
  numbered prompts; no source/SDK/test implementation introduced.
- `git diff --check`, staged whitespace and diff review: PASS.
- C++ driver tests, MCU links, remote CI and physical HIL: **NOT RUN**;
  no driver exists and this task is a promptset audit.

No implementation stages were marked complete. EEPROM programming, general
call and ARA remain excluded under the existing release scope. The final
handoff records the resulting commit and verified upstream synchronization;
the input SHA above is not a self-referential claim about this report's commit.

## Response to each finding — 2026-11-17

Rechecked against `171dbf58251e8606d3b643aa1ceddcc7e921c906`, the current
promptset, pinned TI PDFs and the clean TMP1x2 source at `afd5088`. The table
above records historical work; it is not evidence that those planned features
are implemented. No driver exists and stages 4–44 remain unexecuted. The
[later audit](../prompts/audit.md) carries individual adjudications; this
response states what remains valid in each earlier gap claim.

| Earlier gap | Review response and necessary correction |
|---|---|
| Broad feature list | Relevant and retained. FULL_COVERAGE enumerates facts, scope and owners; its existence does not prove all rows implemented or all promises correct. Stage 5 validates fields/policies and later owners supply actual tests. |
| Interface limits | Relevant. The independent source review confirms explicit pointer/word transfer and 1–400 kHz scope; core callback traces cannot prove electrical timing or repeated START on real hardware. Stages 12/34/35 keep adapter enforcement separate. |
| Recovery forgetting limits/offset | Correct for every **owned** field. The earlier wording overreached for default offset ownership: stages 8/18/19/23/29 now preserve unmanaged calibration, with offset writes/replay explicitly opt-in. TMP117 p.25 Table 7-3 and TMP119 p.32 Table 8-2 footnote 1 permit programmed offset defaults. |
| Averaging/alert edge tests | Relevant but incomplete. The newer corrections add retained alert evidence from every CONFIG read and a guarded shutdown/application/readiness baseline before attributing results to a new profile. Routing/polarity tests remain distinct from comparator behavior and physical pin evidence. |
| Fakes inventing silicon guarantees | Correct principle; the previous unknown list was incomplete. MOD readback during/after one-shot and non-command software-reset effects remain unknown. Do not treat all shutdown flag behavior as unknown where therm-mode HIGH has a documented conversion-end update rule. Stages 6/19/21/25/32 retain these distinctions. |
| Reset metadata field differences | Relevant. Preserve bit-level EEPROM domains and programmed defaults. Soft_Reset cannot be asserted to relock EEPROM or reload every field solely from generic reset prose; post-reset guarded readback determines observations. |
| Grade/application conditions | Relevant. Exact-part electrical/package evidence governs. The TMP116 example drawing leaves ALERT open but also SCL/SDA, so it does not establish a general unused-ALERT instruction; retain the conflicting grounding prose and application-owned wiring decision. |
| Excluded functions | Relevant release policy, not silicon incapability. No EEPROM programming, shadow-data writes, general-call or ARA path is added. Typed operations and diagnostics must enforce the same exclusions. |
| Incompatible-family fixtures | Relevant. TMP114/TMP118 remain exclusion fixtures; no mutating UID procedure or foreign-pointer discovery is permitted. The initial shared fake now has a named producer at stage 12, extended before its stage 32 consolidation. |
| API/CLI coverage drift | Relevant. Coverage includes opt-in offset ownership, distinct acquisition requests, state gates and result provenance. Mapping a route does not prove its implementation; stages 27/33/38/39/44 still inspect actual consumers and evidence. |

The source-contradiction paragraph also remains an evidence ledger, not a
resolution. In particular, TMP119's summary table cannot override TMP116's
own electrical supply range; summary accuracy/current comparisons must retain
their exact competing citations. TMP119M orderable scope and the inaccessible
E2E lead remain unresolved. Original PDF bytes and revision attribution are
unchanged. The review corrects prompts and source commentary only; native
driver tests, MCU linking and physical qualification are still not performed.
