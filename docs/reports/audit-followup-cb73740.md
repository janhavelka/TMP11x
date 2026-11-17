# Follow-up finding review of cb73740

Review date: 2026-11-17. Input: clean `main` at
`cb737407d6c3e878be4b5af288b46ba971557478`, synchronized with origin/main.
The submitted review is retained below; individual responses follow it.
No driver implementation, physical operation or stage completion is implied.

Attachment SHA-256: `f5a4fe7e6630b2da045c7e92428d23d8a4841492706d8e072c72d8da5e029389`.

## Submitted review

Follow-up review of cb73740 ("Review audit findings and reconcile promptset safety contracts")

Four independent reviewers verified each of your 68 responses and the applied edits against the
pinned PDFs, TMP1x2 afd5088 and the files at cb73740. Most verdicts and edits are correct; do not
re-open them. Fix the items below. Line numbers refer to cb73740. Afterwards rerun
check_promptset.py, test_check_promptset.py, check_references.py, test_check_references.py and
git diff --check; update the affected Response rows; commit and push.

## A. Contradictions introduced or left by the fixes (do first)

1. Managed one-shot order is inconsistent. 21:19, 30:19/:33 and ENGINEERING:112-114 do the
   preflight compare before any CONFIG write; 8:21 does not ("Establish the shared 0x04 guard,
   explicitly write shutdown, consume old readiness, then trigger"), and 8:28's "pre-trigger CONFIG
   observation" can be met after the SD write has already rewritten AVG/CONV. CONVENTIONS:46-49
   puts an I2C read before the local desired-mode check (contradicting "rejects before I2C") and
   omits the 0x04 guard. Use one sequence in 8:21, 8:28 and CONVENTIONS:46-49:
   local desired-shutdown check (zero I2C) -> 0x04 guard -> read CONFIG and compare owned stable
   fields (mismatch or unowned MOD=11 latches dirty, zero writes) -> guarded SD write -> CONFIG
   baseline -> one trigger -> readiness polls -> TEMP.

2. MOD compatibility during an in-flight owned one-shot was lost. 19:23 and ENGINEERING:118-121
   accept MOD 01/11 only "after owned DATA_READY completion". Readiness polls during the owned
   shot, and reads after cancel/timeout, are not covered, so stage 19's live-read drift rule (19:20)
   can latch dirty mid-acquisition. 21:19 and 30:31 already allow these cases. State in 19:23 and
   ENGINEERING:120: from trigger ACK until the next explicit SD write or invalidation (including
   after timeout/cancel), observed MOD 01 or 11 is compatible by owned-trigger provenance and is
   never completion evidence. Unowned 11 is still drift.

3. Startup timing origin (API-7; 8:30, ENGINEERING:179-182, 18:28). Reads are ACKed during the
   power-up window (SNOSD82D p.18; SBOS740A p.17), so a successful read at time T proves power
   was present at or before T. A wait counted from the first successful read in the operation is
   therefore conservative under the same no-reset assumption you already require. Anchoring at
   bind/recovery additionally assumes the sensor was powered at bind.
   - Anchor the default wait at the later of bind/recovery start and the first ACKed read,
     unless the owner supplies a timestamp.
   - Decide whether the no-reset assumption is always required: 8:30 reads either/or, while
     ENGINEERING:179-180 and 6:27 require it always.
   - Define the origin's default value, whether it is a binding field (8:22), whether
     invalidateDeviceState() clears it, and the API that sets a new origin after a reported
     power loss.
   - Carry the origin into 11:21 (Config), 14:18 (bind validation), 31:20 (power-interruption
     tests), the CLI bind/begin/recover arguments and the stage 41 power-cycle flow.

4. Fixture authorization (TOOL-2; COMMON:110-116, VERIFICATION:47-52, CLI_CONTRACT:68-72,
   40:25, START_HERE:62). "A fixture record" has no stated author, and "Existing authorization
   persists" has no bound. As written, an agent-written stage 39 report can authorize stage 40
   flashing in an unattended run that says "Do not ask for routine approval".
   - Valid sources are only the current user instruction, or a user-authored record at a named
     path (e.g. untracked hil/fixture.local.json).
   - Agent-written reports and other projects' records never grant authorization.
   - Authorization is scoped to the current task and the named fixture; re-verify ID and address
     before any write.
   - START_HERE:62: "No available physical TMP11x fixture" -> "no user-authorized fixture".

5. SDK tick threshold (TOOL-5; 34:25, 35:24) is not computable as written ("positive SDK wait",
   "otherwise insufficient", "documented adapter margin"). A 1-tick FreeRTOS wait can expire after
   any fraction of a tick, so a 2-tick floor is necessary, not sufficient. Rule:
   - After the SDK's truncating ms->tick conversion, require ticks >= 2.
   - Also require (ticks-1) x tick_period >= worst-case transfer time at the configured SCL.
   - Also require ticks x tick_period <= remaining budget minus lock/route time.
   - Otherwise refuse with a typed error and zero I2C.
   - Test the 0/1/2-tick edges.
   - Also fix docs/toolchain-and-package-plan.md:172: i2c_master.c line 1011 is the bus-lock
     take before the transfer, not a wait "after starting work".

6. EEPROM guard rule wording.
   - ENGINEERING:185 says "each mutating or freshness-producing acquisition operation", which can
     be read as excluding apply/recover, limits, offset, reset and SD writes; 7:21 has no such
     qualifier. Use: "each mutating operation (apply/recover, limits, offset, shutdown, one-shot,
     reset) or freshness-producing acquisition".
   - "Read 0x04 once" (ENGINEERING:185, 18:27, 7:21) conflicts with the bounded busy waits in
     18:28 and 29:21. State: EUN=1 refuses immediately; busy=1 permits bounded re-reads of 0x04
     only, within the deadline and work cap; at least one EUN=0/busy=0 word must be observed
     before the first write.

7. COMMON:66 "Research stages update existing evidence documents only" contradicts stages 4-7
   (which create docs/variants.md, docs/registers/*.json, docs/feature-coverage.md,
   tools/check_register_ledger.py, docs/device-behavior.md, docs/eeprom-and-reset-policy.md) and
   HANDOFFS:25. Use: "Research stages need not update README/CHANGELOG/guides; they create or
   update only their named evidence artifacts."

## B. Response rows that claim edits not present in the files

- CON-6: 5:24 still says only "register its regression tests". Add "Create
  tools/test_check_register_ledger.py, run as python tools/test_check_register_ledger.py".
- CON-15: 17:20 still says encoders "may canonicalize" MOD=10. Use "must emit MOD=00 for
  continuous; decoders accept 10 as continuous".
- SIL-10: 12:20 still says "EEPROM/0x07 alias". Use "model-dependent 0x07 (TMP116 EEPROM3 /
  TMP117-TMP119 Temp_Offset)".
- SIL-6: 12:27 still applies "STOP resets interface state" to all parts. STOP-reset and
  ignore-until-START are explicit only for TMP117 p.20 / TMP119 p.23; TMP116 documents only the
  p.21 timeout reset. STOP never implies a register-pointer reset.
- CON-12: these HANDOFFS consumer lists still contradict SEQUENCE. Replace each with "consumers:
  see SEQUENCE".
  - :45 tooling-plan says "10/34/35/42"; SEQUENCE has 3, 8, 10, 34, 35.
  - :46 ti-archive says "4-7"; SEQUENCE has 4, 5, 6.
  - :50 safety-policy says "8/18-26"; SEQUENCE has 8, 9, 17, 18, 19, 23-26.
  - :52 owner-contract says "18/21/28-31"; SEQUENCE has 10, 11, 12, 14, 18, 21, 25, 28.
- CON-11: 5:27 "(bit11)" and "bits9:2", and SILICON:64 "bits9:2", remain. Use "bit 11",
  "bits 9:2".
- CON-8: FULL_COVERAGE:35 R04 still lists owner 19, but stage 19 has no EEPROM/busy/0x04 text.
  FULL_COVERAGE:73 F13 lists 4-7, but 4 and 6 have no shadow/NIST obligation. Remove those
  owners, or add the obligation text to those stages.
- CON-7: 44:30 still uses its own evidence list ("host tested, ESP builds, CI tested SHA and
  physical trials"). Refer to the six FULL_COVERAGE evidence columns.
- SAF-5 consumer: 38:19 "Do not invent a reset command until stage 25 source proof permits it"
  is stale. Use: "Route reset only through stage 25's typed guarded operation (TMP117/TMP119);
  TMP116 rejects before I2C."

## C. Smaller leftovers

- 18:48: readiness "must survive until the owner explicitly consumes them" has no consumer and
  conflicts with 18:34 and 20:20. Limit the sentence to alert evidence; initialization readiness
  is a baseline observation only.
- 9:20 calls nowMs "legacy", but 9:21 makes the hook mandatory and 31:23 still requires
  "mismatched caller nowMs" tests. Decide: drop nowMs, or document it as ignored; make 31:23
  conditional on that choice.
- Stale references to the instance's own diagnostics, which are now BUSY-gated:
  - 30:22: restrict "raw diagnostic interference" to outside masters/instances.
  - 30:36: "admission harness" -> "private state-transition component".
  - 26:27: "while a cooperative job is active or its result is retained".
- 8:29: define isOnline(), which ENGINEERING:45 references.
- 23:20: "any raw route" -> "diagnostic and typed routes" (no raw write route exists; X04).
- Polling cadence:
  - 6:25 / ENGINEERING:94-97 cover one-shot polling only. Add a policy-labelled default for
    continuous fresh polling, or delegate it with the TMP119 p.23 caveat.
  - State that the policy limits only this driver's own bus traffic.
  - ENGINEERING:94 "no earlier than" -> "by default".
- 6:34 contradicts itself on the try-fresh baseline. Use: "if no baseline exists under a trusted
  generation, the first read only establishes it".
- ENGINEERING:124 says all continuous profile changes use guarded SD. State whether limit-only
  and routing-only changes in CC go through SD (stage 22's write-order rule implies they may not).
- 30:34 "both CONFIG reads" -> "every CONFIG read".
- Therm HIGH flag retention (22:29, 6:33, SILICON:162, ENGINEERING:159): condition the flag on
  T/nA and no reset only. Routing and polarity affect only the pin. Label pin persistence in SD as
  an inference.
- 41:27-28: add the optional, NOT RUN-if-unsuitable observations your response promised:
  - TMP116 AVG=64 flag change only together with DATA_READY;
  - DATA_READY and alert-mode flags after SD entry.
- README:99 "Remote CI is reserved for prompt 43" -> "runs at stage 43; stage 44 reruns that
  procedure for CI-covered fixes".
- 10:23: say stages 1-42 add no .github/workflows/*.
- 36:20 forward-references "the shared coexistence sample that stage 42 finalizes", but no
  earlier stage creates it. Name the initial producer, or have 36 reuse only 35's adapter.
- CLI:
  - 38:25 "shutdown, if retained" conflicts with CLI_CONTRACT:26. Drop "if retained".
  - CLI_CONTRACT:26: "alias defined at 37, wired at 38, over stage 20's setter".
  - CLI_CONTRACT:16 "model/address setter" competes with `bind <model> <addr>`. Forbid it or
    define it as an alias of bind.
  - 37:23: add the mux-route counter required by CLI_CONTRACT:32 and 34:24.
- docs/reference/extracted/tmp119-coverage-audit.md:24 still calls CONFIG[12] a "Read-only
  preflight". Use: "Observed via CONFIG reads already performed; the write guard reads 0x04[14]."
- 32:21 "Model model-dependent 0x07" -> "Model the model-dependent 0x07".
- SILICON:107 footnote pages -> "pp.25,27-28 / pp.32,34-35". The footnote text is on pp.28/35.
- excluded-families-audit.md:47: p.24 names neither 0x0F nor the CRC state. 0x0F returning zero
  is an inference from its absence in the p.29 map.
- Unify "byte-I/O" (34:22, CLI_CONTRACT:93, HANDOFFS:77) and "line-I/O" (35:20, 37:10/32).
- scripts/pio.cmd: print the selected pio.exe path to stderr. If PLATFORMIO_CORE_DIR is absent
  from the process environment, it silently falls back to the 6.1.19 Core.
- docs/reports/index.md:14: "COMPLETE, prompt/tooling scope" is not a canonical status.

## D. audit.md itself

- After applying B, make the CON-6, CON-11, CON-12, CON-15, SIL-6 and SIL-10 response rows true.
- Relabel API-15 as PARTIAL: the stage 14 half was accepted and applied. BEH-7 is arguably
  PARTIAL too.
- The Validation bullet implies check_promptset.py prints "61 producer IDs, 263 inputs". It
  prints only PASS; say the counts were computed separately.
- Add a short "Errata to original findings" list for auditor errors your response did not
  mention:
  - SAF-4: "EUN changes only through a write to 0x04" -> "EUN can only be set by a write to 0x04;
    POR and general-call reset clear it".
  - SIL-1: TMP114 SCL/SDA I/O range is 0-1.98 V (SNIS214E p.4), not 1.08-1.98 V.
  - SIL-9: TMP116 SBOS740A p.25 Table 4 has no RC legend (only TMP117/TMP119 do).
  - SIL-10: 13:14 did not use Linux to "confirm" the revision nibble; only 4:19 did. Also,
    tRESET was already cited at 19facbe in tmp117-coverage-audit.md:104.
  - API-9: the HANDOFFS rows are :49-50, not :51-52.
  - API-10: the AGENTS.md:58-60 quote continues "...caller or test".
  - API-8: "a rebind discards the old target's dirty state" is a deviation from TMP1x2
    (TMP1x2.h:149), not its precedent.
  - TOOL-5: the Wire timeout rule is at 34:19, not 34:18.

## Response — 2026-11-17

The attachment targets the actual input HEAD, so it is relevant. All seven A
items, nine B bullets, 22 C bullets and four D bullets were checked, including
their nested requests and eight original-auditor errata. IDs below follow that
order. ACCEPT means the correction is supported; PARTIAL identifies a valid
gap with a qualified remedy. These are prompt/tooling changes: driver stages
4–44 remain NOT_STARTED, and future test requirements are not executed tests.

Three disjoint reviews covered silicon, state/API and tooling. The lead
reconciled shared contracts and each consumer. Primary evidence includes the
pinned [TMP116](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf),
[TMP117](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf),
[TMP119](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf)
and [TMP114](../reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf)
datasheets. Page references are printed one-based PDF pages. TMP1x2 was
inspected read-only at `afd50889312f6e218a6a34eca687e3e8547d8b40`, with relevant
historical files checked at the auditor's baseline. No sibling was modified.

### A. Contract contradictions

| ID | Verdict | Evidence and disposition |
|---|---|---|
| A1 | ACCEPT | Stages 8/21/30, CONVENTIONS and ENGINEERING now agree: local desired-SD/initialized/clean checks → 0x04 guard → CONFIG stable-field comparison before any CONFIG write → explicit SD → CONFIG baseline → one trigger → readiness polls → TEMP. Local rejection performs no I2C; mismatch or unowned MOD=11 makes trust dirty without writes. The preflight cannot be deferred until after SD has overwritten fields. |
| A2 | ACCEPT, qualified | TMP116 p.26, TMP117 pp.27–28 and TMP119 pp.34–35 describe physical shutdown, not reliable MOD readback. Stages 17/19/21/30 and ENGINEERING retain the owned MOD=01/11 compatibility exception from terminal physical trigger success through active/completed/cancelled/timed-out states until successful SD or invalidation/rebind. Late physical success preserves provenance without logical success; ambiguous trigger/SD invalidates trust. MOD never proves completion. |
| A3 | ACCEPT, qualified | TMP116 p.17, TMP117 p.18 and TMP119 p.20 permit reads during startup. A successful compatible read proves power existed by its completion, so waiting from that point is conservative under uninterrupted power. FIRST_ACK defaults to the first such read in the current initialization/recovery operation, inherently later than operation start, not an old bind time. It does not prove POR complete or turn typical startup timing into a maximum. CALLER_TIMESTAMP is optional; both sources always require stable power/no outside reset. |
| A4 | PARTIAL | A fixture record needed explicit authorship/approval and bounded scope. COMMON, VERIFICATION, CLI_CONTRACT, START_HERE and stages 39–41 now accept a user instruction or named user-authored/explicitly approved record. Agent reports may cite but never create authority; another fixture's record cannot authorize this target. Rejected automatic current-task expiry and mandatory renewed confirmation: existing permission remains valid within its granted fixture/actions/constraints. Verify target/route/address/compatible identity before a campaign and after reconnect/reset/rebind/mismatch; check continuity before writes without repeating identity reads before every low-level transfer. Compatible ID cannot prove the exact BOM model. No physical operation was performed. |
| A5 | PARTIAL | Stages 34/35 now specify actual tick conversion, a conservative floor of two ticks, `(ticks-1)*P >= Ttransfer`, `ticks*P <= remainingMs` after lock/route, named numerical allowances and 0/1/2-tick tests. This is a chosen admission policy, not a universal sensor rule. The suggested inequalities alone do not bound a complete SDK call: repeated waits, NACK loops, recovery, scheduling and cleanup also require a finite source-derived whole-path bound. Precheck before routing and recheck afterward; refusal performs zero sensor I2C, with any completed mux traffic separately counted. Missing bound/terminal-buffer evidence leaves qualification BLOCKED. Corrected the line-1011 citation as described below. |
| A6 | ACCEPT | Stages 7/18/19/21/29/30 and ENGINEERING now apply the guard to every mutation, explicitly including apply/recover/limits/offset/SD/one-shot/reset, and fresh acquisition. EUN=1 refuses immediately. EUN=0/busy=1 permits paced successful 0x04 re-reads within deadline/work cap; no transport-failure retries or writes before a safe word. Untimed try-fresh returns busy/not-ready instead of entering a wait. Guard reuse is restricted to one uninterrupted exclusive operation. |
| A7 | ACCEPT | COMMON now permits research stages to create/update their named evidence artifacts, including stage 5's ledger checker/regressions, while omitting unrelated README/CHANGELOG/user-guide churn. This matches stages 4–7 and HANDOFFS. |

A3 is carried through stages 8/11/14/18/19/29/31/37/38/41 and the shared
contracts. `StartupOrigin` is an initial Config seed and mutable runtime
evidence, not immutable transport binding or a replayable setting. Bind
validates its structure; timed use validates a nonfuture/unambiguous timestamp
in the hook epoch. The existing `invalidateDeviceState(nextOrigin = FIRST_ACK)`
clears startup/initialized/config/freshness/owned-trigger knowledge and installs
the new seed while preserving desired settings, transport history and
historical sample/alert provenance. Invalid input or active/retained ownership
rejects without changing state. After power loss the owner stops using stale
trust, cancels/consumes any job, invalidates and explicitly recovers under stable
power. Established startup evidence need not be re-waited on every setter.
Before stage 19 exists, stage 18 tests use fresh bind/private engine seams.

CLI bind/begin/recover/invalidate carry `--startup first-ack` or
`--startup at:<firmware_monotonic_ms>`. Explicit begin/recover options use the
same bus-silent invalidation transition; bare operations retain the seed.
Scalar timestamps cannot independently prove their clock domain: the owner
must supply firmware time. Malformed/future/out-of-range timestamps and
foreign-session requests reject before I2C. Settings-only configure cannot
replace startup evidence; a changed seed in full Config input is rejected.

For A5, inspected ESP-IDF v5.5.5 at
`b774170ff46c393eeb5e495ea37936038d3f4f4f`. In its
[official I2C source](https://github.com/espressif/esp-idf/blob/b774170ff46c393eeb5e495ea37936038d3f4f4f/components/esp_driver_i2c/i2c_master.c),
line 1011 computes `ticks_to_wait` and line 1012 takes the bus semaphore before
starting the transfer; lines 543/589 and 597–603 show additional waits/NACK
handling. Thus the review correctly rejects the old “after starting work”
description, but its own “line 1011 is the take” is one line imprecise. The
toolchain plan now distinguishes those operations. No SDK qualification is
claimed from this source review alone.

### B. Edits missing behind prior response claims

| ID | Verdict | Applied correction |
|---|---|---|
| B1 / CON-6 | ACCEPT | Stage 5 explicitly creates `tools/test_check_register_ledger.py` and runs it with Python, then stage 10 registers it. No future tool was falsely reported as existing now. |
| B2 / CON-15 | ACCEPT | Stage 17 requires continuous encoders to emit MOD=00; decoders also accept MOD=10. |
| B3 / SIL-10 | ACCEPT | Stage 12 calls 0x07 model-dependent: TMP116 EEPROM3 versus TMP117/TMP119 Temp_Offset. |
| B4 / SIL-6 | ACCEPT | Stage 12 limits explicit STOP-reset/ignore-until-START claims to TMP117 p.20/TMP119 p.23; TMP116 p.21 supports timeout reset. None implies register-pointer reset. |
| B5 / CON-12 | ACCEPT | HANDOFFS consumer prose now refers to SEQUENCE, including tooling-plan, ti-archive, safety-policy and owner-contract. Other ad hoc lists were reconciled at the same boundary; concrete integration-owner explanations remain. |
| B6 / CON-11 | ACCEPT | Stage 5 and SILICON now use bit 11/bits 9:2 spacing consistently. |
| B7 / CON-8 | ACCEPT | Retained the valid owners and added their obligations: stage 19 observes CONFIG busy only in existing reads and invalidates/re-establishes the 0x04 guard; stages 4/6 explicitly cover F13 EEPROM shadow/NIST provenance. No redundant CONFIG preflight is introduced. |
| B8 / CON-7 | ACCEPT | Stage 44 now refers to FULL_COVERAGE's six exact evidence columns. |
| B9 / SAF-5 | ACCEPT | Stage 38 routes reset through stage 25's typed guarded TMP117/TMP119 operation; TMP116 rejects before I2C. Undocumented reset effects remain unknown. |

The corresponding original response rows have been updated. This follow-up
acknowledges missing edits instead of claiming that every correction was already
present at cb73740.

### C. Remaining prompt and tooling details

| ID | Verdict | Applied correction and limits |
|---|---|---|
| C1 | ACCEPT | Stage 18 retains alert evidence for explicit consumption; initialization readiness is only a baseline observation, not a pending sample/result. |
| C2 | ACCEPT | Stage 9 explicitly retains peer-shaped `nowMs` arguments as ignored compatibility inputs. The authoritative clock hook controls scheduling/deadlines/timestamps; stage 31's mismatched-argument tests now have a definite purpose. Untimed finite operations retain their documented optional-clock behavior. |
| C3 | ACCEPT | Stage 30 limits raw interference to outside masters/instances and tests a real private state-transition component. Stage 26 states active job or retained result ownership, matching the BUSY gate. |
| C4 | ACCEPT | Stage 8 and ENGINEERING define passive `isOnline()` as initialized and READY/DEGRADED. Stage 15 owns health behavior and stage 18 integrates initialized cases. It proves no live response, freshness or config trust. |
| C5 | ACCEPT | Stage 23 uses diagnostic/typed routes; no arbitrary raw writer is implied. |
| C6 | ACCEPT, policy | Stage 6 and ENGINEERING set bounded continuous-fresh default polling to one nominal CONV×AVG cycle, rounded up, from baseline/last poll, within the operation deadline. Untimed try-fresh leaves repeated-call pacing to the caller. One-shot timing says “by default” and keeps explicit earlier-poll tradeoffs. These policies bound only this driver's own polling and do not guarantee silence during conversions; TMP119 p.23 remains the accuracy caveat. |
| C7 | ACCEPT | Stage 6 and ENGINEERING say the first read only establishes a missing baseline under an otherwise trusted generation. It does not establish profile trust or retroactively prove freshness. |
| C8 | ACCEPT, policy | ENGINEERING and stage 22 apply guarded SD/shared apply/baseline/desired-mode/final-readback to all managed profile changes, including limits/routing/polarity in continuous mode. Limit ordering still preserves the requested therm policy; separate writes do not promise atomic/glitch-free alerts. |
| C9 | ACCEPT | Stage 30 preserves evidence from every CONFIG read, not an assumed pair. |
| C10 | ACCEPT, qualified | Stages 6/22, SILICON, ENGINEERING and TMP116/TMP117 extracted audits condition therm HIGH retention on T/nA staying therm and no reset. Routing/polarity affect the pin only. Persistence of the pin through SD is labeled an inference under unchanged routing/polarity, not a separate explicit datasheet guarantee. |
| C11 | ACCEPT | Stage 41 explicitly includes optional TMP116 AVG=64 flag/DATA_READY correlation and DATA_READY/alert-mode observations after SD. Unsuitable fixtures produce NOT RUN with a reason; no unsafe EEPROM/startup experiment or fabricated simulator proof is required. |
| C12 | ACCEPT | Prompt README now describes stage 43 CI plus stage 44's rerun procedure for its CI-covered fixes. |
| C13 | ACCEPT, scoped | Stage 10 explicitly prohibits initial workflow creation during stages 1–42. Existing automatic triggers/protection remain intact. |
| C14 | ACCEPT | Stage 36 reuses stage 35's adapter and owns its concrete P4 consumer. It no longer consumes a nonexistent sample supposedly produced later by stage 42. |
| C15 | ACCEPT | Shutdown is mandatory: alias grammar at 37, route at 38, setter at 20. `model`/`addr` are query-only; binding changes use `bind`. Stage 37 includes the separately counted mux-route attempts. |
| C16 | ACCEPT | TMP119 audit now treats CONFIG[12] as an observation in existing CONFIG reads; mutation guard reads 0x04[14]. Its follow-up response records the correction. |
| C17 | ACCEPT | Stage 32 now says “Model the model-dependent 0x07”. |
| C18 | ACCEPT | SILICON pairs map/config citations with the actual EEPROM field footnotes: TMP117 pp.25,27–28 and TMP119 pp.32,34–35. |
| C19 | ACCEPT | Excluded-family audit separates TMP114 p.24's generic unsupported-register rule from the p.29 map-based 0x0F inference. Page 24 does not state CRC mode; omitted-CRC behavior remains adverse/unknown rather than invented. Its response addendum records that distinction. |
| C20 | ACCEPT | Stages 34/35 provide bounded byte hooks; stage 37 owns line assembly/overflow/parser behavior once. CLI_CONTRACT and HANDOFFS match. |
| C21 | ACCEPT | `scripts/pio.cmd` prints the selected executable to stderr before invocation. Configured, unset-override fallback and missing-path cases were exercised; the existing default Core is now visible without installing another Core. |
| C22 | ACCEPT | Report-index status is COMPLETE; prompt/tooling scope belongs in its evidence column. Stage 3's older qualified status was normalized in the same way, retaining the scope correction in evidence. |

### D. Original response and auditor errata

| ID | Verdict | Disposition |
|---|---|---|
| D1 | ACCEPT | Updated the affected CON-6/11/12/15 and SIL-6/10 response claims alongside their now-present edits. Other changed rows also point to the refined startup/guard/flag/SDK/authorization rules. Original submitted findings remain historical text. |
| D2 | ACCEPT | API-15 is PARTIAL: unused early scaffolding was removed, while the producer move remains rejected. BEH-7 is PARTIAL: sparse-polling clarification accepted, fallback/time-only/GPIO freshness claims rejected. |
| D3 | ACCEPT | Validation now distinguishes the checker's PASS output from independently computed 61 producer IDs/263 required-input links. |
| D4 | ACCEPT, qualified | All eight errata were checked and added to audit.md. SAF-4 EUN can only be set by writing 0x04 but POR/general call clear it; TMP114 supply and I/O limits differ; TMP116 lacks the claimed RC legend; original stage 13 did not claim Linux confirmed revision bits; the API-9/API-10/TOOL-5 citations were corrected; TMP1x2 preserves dirty across bind, unlike the intentional TMP11x policy. The earlier TMP117 audit already cited startup timing and p.7, but did not literally name the tRESET symbol at that line. |

No general-call implementation is introduced by correcting the SAF-4 source
claim. General-call reset remains excluded. No additional public fallback,
EEPROM maintenance route or repeated approval requirement was introduced.

## Validation and remaining evidence

Executed with Python 3.12.10 against the integrated working tree:

- `python scripts/check_promptset.py`: PASS for 44 stages, dependency DAG,
  handoffs, coverage IDs/owners, prose/index parity and links/PDF pages.
- `python scripts/test_check_promptset.py`: PASS, 34 regression cases.
- `python scripts/check_references.py`: PASS, all 14 original pinned PDFs;
  sizes and SHA-256 match. No archive bytes or manifest entries changed.
- `python scripts/test_check_references.py`: PASS, nine regression cases.
- PlatformIO launcher cases: configured `C:\pio` selects Core 6.2.0;
  absent override selects the existing default Core 6.1.19; missing configured
  path containing spaces fails without fallback. Selected existing paths
  appear on stderr, while version output stays on stdout. All three PASS.
- Separate SEQUENCE.json count: 61 producer IDs, 263 required-input links.
  Response inventory: A1–7, B1–9, C1–22 and D1–4, exactly once each; PASS.
- `git diff --check`: PASS. Working/staged changes reviewed before commit.

There is no driver/runtime implementation to test at this stage; MCU links,
remote CI and physical HIL remain NOT RUN. Source uncertainty remains a future
implementation/evidence obligation, not a fabricated pass. Result commit and
upstream state belong in the final task handoff rather than a self-referential
commit hash inside this report.
