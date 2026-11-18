# Second follow-up review of 6b173cc

Review date: 2026-11-18. Input: clean `main` at
`6b173ccfed94d8de4f682bf0d08a91d6454d99f4`, synchronized with origin/main.
All 17 submitted findings and their conditional remedies were reviewed.
No driver implementation or physical qualification is implied.

Submitted-text SHA-256: `ffb4cb433c01450d646827a0a32451948ba8f28b95449bc9fe4cdd466df2c81b`. Hash only the text between
the Submitted review and Response headings, UTF-8 without BOM, LF endings,
trailing blank lines removed and exactly one final LF.

## Submitted review

Second follow-up review of 6b173cc ("Reconcile follow-up audit and prompt execution contracts")

Three independent reviewers checked all 42 responses (A1–A7, B1–B9, C1–C22, D1–D4) against
6b173cc, the pinned PDFs, TMP1x2 afd5088 and local ESP-IDF v5.5.5 (b774170). Most edits are
present and correct; do not reopen them. Your corrections of the previous request were right:
i2c_master.c 1011 computes ticks and 1012 takes the bus lock; the earlier inequalities were
necessary but not sufficient; the tRESET erratum needed your qualification.

Fix the items below. Line numbers refer to 6b173cc. Record your responses as before (for example
docs/reports/audit-followup-6b173cc.md). Rerun the four checkers and git diff --check, then commit
and push.

## Medium

1. Fixture authorization still lets an agent report carry a grant forward.
   COMMON:112 accepts authorization "traceable to the user's instruction"; 41:19 calls an
   agent-written report "evidence of the grant"; 40:25 verifies "stage 39's authorization record"
   (no such artifact exists in SEQUENCE/HANDOFFS). In a later unattended session ("Do not ask for
   routine approval"), the agent's own report that "the user authorized COM7/TMP117@0x48/upload"
   still satisfies this, although the agent cannot see the grant. "Explicitly user-approved record"
   has no evidence criterion either.
   Keep your no-expiry, no-reconfirmation policy, but accept only sources the agent can read
   directly:
   (a) a user instruction visible in the current task context, or
   (b) a record at a user-named path that the user wrote, or approved in a message visible in the
       current context (e.g. untracked hil/fixture.local.json).
   Add: "An agent report, summary or prior-session paraphrase is never a source. If neither (a)
   nor (b) can be read directly, physical work is BLOCKED. A valid (b) persists across sessions
   within its scope."
   Mirror this in VERIFICATION:47-55, CLI_CONTRACT:79-87, START_HERE, 39:18/25, 40:25 (verify the
   instruction or record itself) and 41:18-19. In 41:18, "Obtain explicit active-fixture
   authorization" -> "Confirm the cited authorization covers...". In 41:19, "evidence of the
   grant" -> "may point to the grant; verify the grant itself per COMMON".

2. StartupOrigin. FIRST_ACK is sound (reads are allowed during POR: SBOS740A p.17, SNOSD82D p.18,
   SNIS236 p.20) and fixes the original problem by itself. The optional CALLER_TIMESTAMP, which the
   previous request asked for, adds most of the complexity and two defects:
   a. Configure trap. 8:32 and 29:20 reject a full Config whose seed differs from the installed
      one. After invalidateDeviceState(nextOrigin) installs a new seed, a caller who reuses the
      original Config in startConfigure is rejected, although the seed is "not a CONFIGURE setting".
   b. Power-loss order. CLI_CONTRACT:60-61 and 41:22 say "invalidate with a new seed, restore
      stable power and explicitly recover". A nonfuture timestamp given before power is restored
      predates POR, which violates the stable-power rule (CLI_CONTRACT:59, ENGINEERING:197-198). On
      TMP116 the startup wait is the main protection, because busy==0 alone does not prove startup.
   Preferred fix (simplest, per AGENTS.md "Prefer small, direct changes"): keep FIRST_ACK as the
   only origin.
   - invalidateDeviceState() takes no origin argument.
   - Remove the Config startup seed, the configure seed check and the CLI --startup option.
   - Record CALLER_TIMESTAMP as a deferred extension.
   This also removes items 7 and 8 below.
   If you keep CALLER_TIMESTAMP instead:
   - Configure takes settings-only input, or ignores the seed field and says so in Doxygen. It
     never compares against the installed runtime seed.
   - A timestamp may be supplied only at or after restoring stable power (e.g. recover --startup
     at:<ms> after restore). FIRST_ACK may be installed at any time.

## Low

3. A6 not carried into stage 21, although the response says it was.
   21:19 still reads "At the start of each mutating or freshness-producing acquisition, read
   0x04...". Use the ENGINEERING:221-226 wording. Add "untimed tryReadFresh returns busy/not-ready
   on busy=1; no wait" to 21:19 and 7:21 (today it appears only at ENGINEERING:232-233). Correct
   the A6 row.
4. C8 leftovers still imply that limit-only changes skip shutdown:
   - ENGINEERING:73-75 -> "Limit writes occur inside the shared guarded SD apply phase; keep
     intermediate pairs ordered when the current pair is known (defensive). Unknown or inverted
     current pairs need full-profile verification before therm operation."
   - 22:18: align with 22:28.
   - 20:20 "For a managed change that must enable current-profile freshness" -> "Every managed
     change".
5. ENGINEERING:71-72 "Before a one-shot trigger, compare all stable CONFIG fields" -> "Before the
   one-shot's first CONFIG write (including SD)...".
6. CONVENTIONS:50-51: the MOD 01/11 window lists "during the shot and after cancellation/timeout"
   but omits normal completion. Add "completion" (matches ENGINEERING:129 and 21:20).
7. If CALLER_TIMESTAMP stays:
   - CLI_CONTRACT:54-55 "use the existing valid seed or establish missing proof from FIRST_ACK"
     reads as a silent fallback -> "consume the installed seed; an invalid caller timestamp
     rejects before I2C; no fallback".
   - Use one placeholder, `firmware_monotonic_ms`, in 37:22 (now `device_monotonic_ms`), 38:26,
     31:21 and 41:22 ("device" reads as the sensor).
   - 37:22: add stage 19, which owns invalidateDeviceState -> "stage 8/14/18/19 lifecycle".
8. Seed lifecycle gaps:
   - 14:19 "Unbind/end clear the seed/evidence", but end() keeps the binding (TMP1x2.cpp:78-88),
     so the seed for a later begin() is undefined -> "end clears startup proof (and resets the seed
     to FIRST_ACK); unbind clears both".
   - ENGINEERING:217 and 8:31: state that a driver-issued or reported reset replaces the seed with
     FIRST_ACK.
9. FULL_COVERAGE:65 (F05) owners "6–9, 18, 21, 29–32" omit 11, 14, 19 and 41, which now carry
   startup-origin obligations.
10. Stage 10 uses an artifact it never declares.
    10:20 registers "the stage 5 register-validator regression runner", and 5:24 / COMMON:91 say
    stage 10 registers both ledger commands. But stage 10's consumes are only [tooling-plan,
    api-contract, owner-contract], and the consumers of register-validator are 6, 7 and 43.
    - Add register-validator to stage 10 in SEQUENCE.json and in 10_package-foundation.md:6.
    - 10:20 -> "the stage 5 register validator and its regression runner".
    - Update every 263 -> 264: HANDOFFS:40, audit.md (CON-12 row and Validation), reports/index.md
      and both report Validation sections.
11. SILICON:143-144 states as documented fact that "Identity at 0x0F reads zero on these excluded
    parts". It is an inference -> "expected to read zero under the ordinary protocol, inferred from
    the non-indexed-read rule (TMP114 p.24; TMP118 p.23) and the register maps (pp.29/26);
    CRC-disabled TMP114 is fixture policy".
12. audit.md SAF-4 erratum (about :2163-2164) cites only the register pages for "POR and
    general-call reset clear it". Add the general-call pages: TMP116 p.17, TMP117 p.18,
    TMP119 p.20.
13. audit-followup-cb73740.md:270 and :305 say the affected response rows were updated. In fact
    only the CON-6, CON-8, CON-11 and SIL-6 rows changed. The CON-12, CON-15, CON-7, SIL-10 and
    SAF-5 rows are byte-identical to cb73740 and are true now only because of the file edits.
    Reword accordingly.
14. audit-followup-cb73740.md:8 "Attachment SHA-256" is not reproducible from the committed LF
    file. It equals lines 12–192 with CRLF endings and a trailing CRLF. Either state that scope and
    normalization, or remove the line.
15. 36:20 "which stage 42 may package or extend": stage 42 has no dependency on 36 and does not
    consume p4-consumer-source, while 42:29 still asks for one coexistence sample. Drop the clause,
    or make 42's reuse explicitly conditional, so that two TMP1x2-coexistence samples cannot result.
16. docs/reference/extracted/tmp117-coverage-audit.md:72 "bit11" and :96 "bits10:2": add the spaces.
17. Optional precision for 34/35 and the toolchain plan (TOOL-5):
    - On S2/S3, i2c_master.c reaches a fixed 50 ms bus-clear loop (I2C_CLR_BUS_TIMEOUT_MS,
      lines 51 and 95-98) from 554, 602 and 686-688. That loop is independent of the caller's
      timeout and dominates the recovery allowance.
    - Non-SMP pdMS_TO_TICKS multiplies in 32-bit TickType_t, so the adapter must clip the budget
      before converting.

## Response - 2026-11-18

The review targets the actual input HEAD and is relevant. ACCEPT denotes a
supported correction; PARTIAL means the gap is valid but its remedy needs the
qualification recorded below. Conditional findings 7/8 were assessed against
the selected FIRST_ACK-only design, rather than implementing a second origin
interface merely to repair it. Existing correct findings were not reopened.

Three independent reviews checked startup/state/silicon, dependency handoffs
and SDK details; the lead integrated shared contracts and reports. The primary
sources were pinned [TMP116](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf),
[TMP117](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf),
[TMP119](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf),
[TMP114](../reference/source/TMP114-Digital-Temperature-Sensor-Datasheet-SNIS214E.pdf)
and [TMP118](../reference/source/TMP118-Digital-Temperature-Sensor-Datasheet-SNIS237A.pdf)
PDFs, clean TMP1x2 at `afd50889312f6e218a6a34eca687e3e8547d8b40`, and immutable
ESP-IDF v5.5.5 `b774170ff46c393eeb5e495ea37936038d3f4f4f` source. The installed
IDF working tree now has modifications, so its uncommitted files were not
treated as evidence for that commit. No sibling or SDK source was changed.

| Finding | Verdict | Evidence and disposition |
|---|---|---|
| 1 | ACCEPT, qualified | The original-source requirement was still ambiguous. COMMON, VERIFICATION, CLI_CONTRACT, START_HERE and stages 39–41 now require a directly readable user instruction in current context, or a record at a user-named path written by the user or approved in a visible user message. Reports/summaries/prior-session paraphrases may point to a grant but never substitute for it. Unreadable sources leave physical work BLOCKED; valid scoped records persist without expiry/reconfirmation. Stage 40 checks the original source, and stage 41 confirms coverage rather than obtaining repeated approval. Stage 39 can record inputs in its existing runner evidence; no new authorization artifact in SEQUENCE is needed. No physical action occurred. |
| 2 | ACCEPT; simpler remedy selected | The full-Config seed comparison could reject ordinary reused settings after invalidation. Installing a caller timestamp before restoring power could also consume a pre-POR interval, violating the stable-power assumption. Removed the public StartupOrigin type, Config seed, configure comparison, invalidation argument and CLI options. FIRST_ACK is the sole internal policy; CALLER_TIMESTAMP is explicitly deferred at stage 8. No current consumer needs the extra API. Stages 6/8/11/14/18/19/29/31/37/38/41 and shared contracts now agree. |
| 3 | ACCEPT | Stage 21 really retained narrower wording despite the A6 response. It now names all mutating operations/fresh acquisition and the timed EUN/busy rule. Stages 7/21 explicitly make untimed tryReadFresh return busy/not-ready without waiting. Timed paths may repeat successful guard observations within deadline/work cap, never retry failed transfers. Stage 21/30 test obligations distinguish busy-through-deadline refusal from successful busy-to-safe progression. Corrected the old A6 row to acknowledge the delayed propagation. |
| 4 | ACCEPT | ENGINEERING, stage 20 and stage 22's first bullet now consistently require the shared guarded SD apply phase for every managed settings change, including limits/routing-only changes. Intermediate limit ordering is defensive inside that phase; unknown/inverted initial pairs require full-profile verification before therm operation. The final mode is the desired mode, not unconditionally continuous. No atomic/glitch-free comparator guarantee is added. |
| 5 | ACCEPT | ENGINEERING's lifecycle paragraph now places stable-CONFIG comparison before the one-shot's first CONFIG write, explicitly including SD. It matches the detailed sequence and zero-write drift rejection. |
| 6 | ACCEPT | CONVENTIONS includes normal completion as well as active/cancelled/timed-out owned shots in the MOD=01/11 provenance window. MOD itself still proves neither completion nor quiescence. |
| 7 | RESOLVED BY 2 | No caller timestamp exists, so fallback ambiguity and timestamp placeholder drift disappear. Stage 37 now names the 8/14/18/19 lifecycle explicitly; stage 38 wires argument-free invalidation. Unsupported startup options have zero-I2C parser test obligations. |
| 8 | ACCEPT, adapted to 2 | Verified TMP1x2.cpp lines 78–88: end retains binding/Config; unbind clears them. TMP11x stage 14 and shared contracts now explicitly preserve binding/desired settings on end while clearing initialization/internal startup proof; unbind also removes binding. No seed remains undefined. Driver-issued or reported reset discards the old ACK origin/proof. Stage 25 invalidates before the reset attempt, waits at least its separate documented 2 ms, then establishes fresh post-reset FIRST_ACK evidence before further writes. Failure cannot preserve pre-reset proof. |
| 9 | PARTIAL | F05 omitted real lifecycle/HIL owners. Added 14, 19 and 41, plus 25 for the explicit reset proof obligation. Stage 11 no longer owns a removed StartupOrigin value type, so adding it after simplification would create a false owner. COVERAGE's summary follows the updated obligations. |
| 10 | ACCEPT dependency; qualified count remedy | Added register-validator to stage 10's prompt/SEQUENCE inputs and explicitly registered both ledger commands. The existing dependency path 10 → 8 → 5 already reaches its producer; no new prerequisite is needed. Current metadata is 61 producer IDs/264 inputs, reflected in HANDOFFS, PROGRESS, current audit rows/index and this validation. Historical 263-input runs and quoted submissions are retained with dates/supersession notes; rewriting their measured result as 264 would falsify history. Both older validation sections now point to the current 264-input review. |
| 11 | ACCEPT | SILICON now labels the excluded-parts 0x0F zero expectation as an inference from generic non-indexed-read rules (TMP114 p.24; TMP118 p.23) plus maps (pp.29/26). Those rule sentences do not name 0x0F. CRC-disabled TMP114 remains fixture policy, not a quoted condition. |
| 12 | ACCEPT | Added TMP116 p.17, TMP117 p.18 and TMP119 p.20 to SAF-4's erratum alongside reset-value register pages 29/30/37. The added pages explicitly describe general-call clearing of EUN. This citation correction does not permit general-call traffic in the driver. |
| 13 | ACCEPT | Comparing cb73740 with 6b173cc confirms the named rows stayed byte-identical. Reworded the former report: B-related rows actually changed were CON-6/8/11 and SIL-6; CON-12/15/7, SIL-10 and SAF-5 became true through prompt edits. The report no longer claims every response row itself was edited. |
| 14 | ACCEPT | Reproduced the old digest exactly from Submitted review text encoded as UTF-8 without BOM, CRLF line endings, no trailing blank lines and one final CRLF. Its header now states that scope/normalization. This report instead explicitly hashes its submitted section with LF endings; both are reproducible without access to an attachment directory. |
| 15 | ACCEPT | Removed stage 36's unconditional forward packaging claim. Stage 42 reuses actual stage-36 coexistence application source if available; otherwise it creates the shared sample independently of P4/SDK availability. A later/resumed stage 36 reuses that source with P4 glue. HANDOFFS records conditional reuse, not a new required backward dependency or duplicate sample. |
| 16 | ACCEPT | Corrected bit 11 and bits 10:2 spacing in the TMP117 extracted audit. No register mask or PDF bytes changed. |
| 17 | ACCEPT, qualified | Immutable v5.5.5 source confirms the S2/S3 fixed bus-clear loop and default-kernel multiplication hazard. Stages 34/35 and the toolchain plan now require the separate recovery allowance and pre-SDK conversion cap, with boundary tests. “50 ms” is a fixed setting, not an exact wall-time maximum: strict `>` adds a tick, and setup/scheduling plus reachable call multiplicity remain bounded separately. CONFIG_FREERTOS_SMP=n identifies the default kernel source, not necessarily single-core execution; default dual-core S3 is affected. |

FIRST_ACK remains a conservative library policy. TMP116 p.17, TMP117 p.18 and
TMP119 p.20 permit reads during POR, so completion of a compatible read proves
power was present by then. It does not prove POR finished or provide a specified
startup maximum. Require stable power/no outside reset from that read through
all operations using its proof. Power-loss recovery is stop/cancel/take,
invalidate, restore stable power, explicitly recover. Since invalidation
accepts no timestamp, it may safely precede power restoration. Existing valid
proof is reused during ordinary settings changes; end/reset/invalidation cannot
reuse a stale anchor. Historical sample/alert provenance and transport history
remain distinct from current initialization/trust.

Finding 17's source details are recorded in the
[toolchain plan](../toolchain-and-package-plan.md#second-follow-up-sdk-timeout-evidence-2026-11-18).
The pinned [I2C source](https://github.com/espressif/esp-idf/blob/b774170ff46c393eeb5e495ea37936038d3f4f4f/components/esp_driver_i2c/i2c_master.c)
defines the 50-ms setting at line 51, polls it at 95–98 and reaches it from
554/602/686–688 under S2/S3 hardware-clear capability. Reserve the tick term
`(pdMS_TO_TICKS(50)+1)*P` per reachable clear plus bounded overhead, rather than
assuming the caller timeout caps recovery. The default 32-bit-product macro
requires the argument cap `UINT32_MAX/configTICK_RATE_HZ` before SDK conversion,
in addition to API/deadline bounds; alternate macro widths must be inspected.
These local inequalities do not replace whole-path/terminal-buffer proof.

## Validation and remaining evidence

Executed locally with Python 3.12.10:

- `python scripts/check_promptset.py`: PASS; all 44 stages, dependency DAG,
  handoffs, coverage IDs/owners, prose/index parity and links/PDF pages.
- `python scripts/test_check_promptset.py`: PASS, 34 regression cases.
- `python scripts/check_references.py`: PASS; all 14 pinned PDFs retain their
  original sizes and SHA-256. No archive bytes or manifest entries changed.
- `python scripts/test_check_references.py`: PASS, nine regression cases.
- Independent SEQUENCE count: 61 producer IDs and **264 required-input links**.
  Exactly one response row for each finding 1–17: PASS.
- Recomputed both follow-up submitted-text digests from committed-file-format
  text using their documented normalizations: PASS. Checked local report link
  targets and historical row comparisons for finding 13: PASS.
- SDK review independently checked tick-overflow boundaries and fixed-clear
  tick terms at 100/1000 Hz: PASS arithmetic/source checks, not a firmware test.
- `git diff --check` and staged whitespace check: PASS. Reviewed owned changes
  before commit; no sibling/SDK mutation, hardware upload or port access.

Driver source, MCU links, remote CI and physical HIL remain NOT RUN; stages
4–44 remain NOT_STARTED. Result commit/upstream synchronization belongs in
the task handoff, not a self-referential SHA inside this report.
