# TMP11x promptset audit — 2026-11-16

**Input:** `main` at `19facbe`, clean and synchronized with `origin/main`.
**Scope:** prompts 1–44 and every shared contract in `docs/prompts/`. Also root
`AGENTS.md`, `README.md` and `START_HERE.md`; `docs/reference/` (manifest and
extracted audits); the preparation documents in `docs/`; `docs/reports/`; and
`scripts/`.
**Nature:** this file reviews the instructions only. It executes no stage,
changes no stage status and is not implementation evidence. `PROGRESS.md`
stays authoritative for stage state.

## Method

- Six independent read-only reviewers covered:
  - TMP117 registers and interface;
  - TMP116/TMP119 deltas and the excluded families;
  - timing and behaviour;
  - cross-document structure;
  - API and state design;
  - tooling, CLI, HIL and CI.

  Each claim was checked against the pinned PDFs. Ambiguous tables were
  rendered and viewed, not only text-extracted.
- The lead re-checked every High finding and a sample of the others against
  the sources: the TMP117 pages 7 and 12–32, the prompt lines cited, the
  sibling TMP1x2 at `afd5088`, and `scripts/pio.cmd` on the development host.
  Where a reviewer's claim needed correction, the corrected version is the one
  below.
- Checks run, all PASS:
  - `python scripts/check_promptset.py`
  - `python scripts/test_check_promptset.py` (27 tests)
  - `python scripts/check_references.py` (14 PDFs)
  - `python scripts/test_check_references.py` (9 tests)

  The checkers prove structure only (see CON-9).
- Earlier audits (`docs/reports/promptset-audit.md`, `feature-gap-audit.md`,
  `handoff-audit.md`) are not repeated here. Where their corrections are
  incomplete or wrong, this file says so.

### Conventions

- **File references.** A bare file name such as `21_fresh-one-shot.md:18` is
  in `docs/prompts/`. Other paths are relative to the repository root.
  `:N` means line N at `19facbe`.
- **Datasheet references.** Pages are printed page numbers, which equal
  one-based PDF pages for these documents.

| Doc ID | Document |
|---|---|
| SBOS740A | TMP116/TMP116N datasheet |
| SNOSD82D | TMP117/TMP117M/TMP117N datasheet |
| SNIS236 | TMP119 datasheet |
| SNIS214E | TMP114 datasheet (excluded) |
| SNIS237A | TMP118 datasheet (excluded) |
| SNOU161 | TMP117EVM user guide |

### Severity

| Level | Meaning |
|---|---|
| Critical | Following the prompt damages hardware/EEPROM or makes a stage impossible. None found. |
| High | Wrong assumption or contradiction that will produce a bug, a datasheet violation or a failed stage. |
| Medium | Missing rule, ambiguity or inconsistency that a careful coder can still get wrong. |
| Low | Wrong citation, stale record, wording, redundancy. |

## Result

| Area | High | Medium | Low |
|---|---:|---:|---:|
| SAF: EEPROM and write safety | 1 | 4 | 1 |
| BEH: device behaviour and timing | 2 | 6 | 2 |
| SIL: datasheet facts and citations | 0 | 1 | 9 |
| API: architecture, state, ownership | 2 | 11 | 2 |
| CON: promptset structure and consistency | 1 | 9 | 6 |
| TOOL: tooling, CLI, HIL, CI | 2 | 4 | 5 |
| **Total (68)** | **8** | **35** | **25** |

The promptset is structurally sound:

- The dependency DAG, artifact IDs, prerequisite prose, README index and the
  counts it claims all check out.
- Most silicon facts are correct.

The remaining defects cluster in these areas:

1. **The driver destroys its own alert evidence (BEH-2).** Its own CONFIG reads
   clear the alert-mode flags, and no prompt says to keep them.
2. **The datasheets leave some behaviour undocumented, but the prompts treat it
   as settled:**
   - MOD readback around a one-shot (BEH-1);
   - soft-reset effects (SAF-5).
3. **The EEPROM guard has a hole and an unspecified read path:**
   - the synchronous one-shot trigger write is unguarded (SAF-1);
   - no prompt says which register the guard reads (SAF-2).
4. **The offset default would overwrite an EEPROM-programmed calibration
   (API-2).**
5. **The synchronous engines are not required to be resumable, which forces a
   rewrite at stages 29/30 (API-1).**
6. **The CI policy contradicts itself (CON-1).**
7. **The repository's PlatformIO launcher runs the wrong Core (TOOL-1).**
8. **Fixture and upload authorization is undefined for unattended runs
   (TOOL-2).**

## Fix order

Apply the fixes as one promptset correction before stage 4 starts. Update
the affected prompts, SEQUENCE, HANDOFFS, FULL_COVERAGE and the checker
together (COMMON.md:95-97), then rerun the four checks above.

1. **Silicon and safety rules that stages 5–7 must record:**
   BEH-1, BEH-6, SAF-2, SAF-4, SAF-5, SAF-6, SIL-2, SIL-3, BEH-5, BEH-10,
   SIL-5, SIL-6.
2. **Contracts that stage 8 freezes:**
   API-1, API-2, API-3, API-5, API-6, API-7, API-8, API-11, API-12, BEH-2,
   API-4, API-13.
3. **Prompt-level guards and tests:**
   SAF-1, SAF-3, BEH-3, BEH-4, BEH-7, BEH-8, BEH-9.
4. **Structure, made before stage 10:**
   CON-1, CON-4, CON-5, CON-6, CON-7, CON-8, CON-9, CON-12, CON-14.
5. **Tooling and physical-work rules:**
   TOOL-1 (immediately; it affects any PlatformIO use), TOOL-2, TOOL-3,
   TOOL-4, TOOL-5, CON-3, TOOL-8.
6. **Housekeeping, any time:**
   CON-2, CON-10, CON-11, CON-13, CON-15, CON-16,
   TOOL-6, TOOL-7, TOOL-9, TOOL-10, TOOL-11, SIL-1, SIL-4, SIL-7, SIL-8,
   SIL-9, SIL-10, API-9, API-10, API-14, API-15.

---

## 1 · EEPROM and write safety

### SAF-1 · High · The synchronous one-shot trigger bypasses the EEPROM-lock guard

**Where**
- `21_fresh-one-shot.md:18` (work list) and `:31-37` (exit criteria).
- `18_safe-initialize.md:26`: "Keep its trigger out of stable configuration
  application".
- Contrast `30_cooperative-measurement.md:19`, which does require an "EUN/busy
  guard for a trigger write".

**Wrong**

The one-shot trigger is a CONFIG write: MOD=11 plus the stable
CONV/AVG/T/nA/POL/DR bits. Stage 18 deliberately keeps it out of the guarded
apply engine. Stage 21 neither requires nor tests the 0x04 check. Only the
cooperative path has it.

**Why it matters**
- SNOSD82D p.30 Table 7-10, EUN=1: "any writes to programmable registers
  program the respective location in the EEPROM".
- CONFIG bits 10:2 are EEPROM-storable (pp.27-28, footnote 1).
- p.18: "Avoid using the device to perform temperature conversions when the
  EEPROM is unlocked".
- p.7: EEPROM endurance is 1,000 writes minimum.
- SBOS740A p.17 and SNIS236 p.20 say the same.

If an external tool left EUN=1, a 1 Hz one-shot loop reprograms the CONFIG
EEPROM word (including MOD0 as the start-up mode). It exhausts the minimum
endurance in about 17 minutes.

**Fix**
- In 21's work list add: "Before the trigger write, establish startup readiness
  and read 0x04 in the same exclusive-owner phase. Require EUN=0 and busy=0;
  otherwise return `EEPROM_UNSAFE` with zero CONFIG writes."
- Add an exit test: "EUN=1, busy=1 or unestablished startup at trigger → zero
  CONFIG writes, no freshness claim."
- Change 18:26 to: "Keep its trigger out of stable configuration application;
  the trigger uses the same startup/EUN/busy guard."

### SAF-2 · Medium · The guard register is unspecified, and ENGINEERING misstates the busy relationship

**Where**
- `ENGINEERING.md:101`
- `18_safe-initialize.md:25`
- `7_eeprom-reset-policy.md:20`
- `29_cooperative-config.md:14`
- `30_cooperative-measurement.md:19`

**Wrong**
- No prompt says which register the "EUN=0/busy=0" check reads.
- ENGINEERING calls the 0x04 busy bit "distinct from CONFIG's busy bit 12".
  `5_register-ledger.md:21` correctly says it mirrors that bit.

**Why it matters**
- SNOSD82D p.30: bit 14 "is the mirror of the EEPROM busy flag (bit 12) in the
  configuration register".
- A guard that reads CONFIG bit 12 is a destructive read. It clears DATA_READY
  (p.27), clears alert-mode HIGH/LOW and releases ALERT (p.16) — before every
  write.

**Fix**
- ENGINEERING:101 → "its busy flag is bit 14, a mirror of CONFIG bit 12".
- Add to the ENGINEERING EEPROM section and to 18:25: "The pre-write guard
  reads only register 0x04 (EUN bit 15 and busy bit 14 in one word). Never use
  CONFIG bit 12 as the guard."

### SAF-3 · Medium · The offset write has no guard test, and R16 omits its EEPROM domain

**Where**
- `23_offset-feature.md:18` ("safe guarded 16-bit encoding") and `:24` ("a
  volatile setting in this library").
- `23_offset-feature.md:31-32` (exit criteria).
- `FULL_COVERAGE.md:42` (R16).

**Wrong**

0x07 is EEPROM-backed, but stage 23's tests cover only sign, rounding, the
TMP116 refusal and ambiguous writes. Nothing tests refusal under EUN=1 or
busy=1. R16 does not list the EEPROM reset domain.

**Why it matters**
- SNOSD82D p.25 Table 7-3 shows `07h R/W 0000h(1)`. Footnote (1): "stored in
  EEPROM … The device reset value can be changed by writing the relevant code
  in the EEPROM cells". SNIS236 p.32 is the same.
- With EUN=1, an offset write programs that cell, and the value persists across
  power cycles.

**Fix**
- 23:18 → "…write through the shared startup/EUN/busy guard (0x07 is
  EEPROM-backed)".
- Add to the exit criteria: "EUN=1, busy=1 or unestablished startup → zero 0x07
  writes".
- R16 evidence: add "EEPROM-backed reset domain; guarded write".

### SAF-4 · Medium · The EUN-check cadence and the "no fresh claim while unlocked" rule are inconsistent

**Where**
- `18_safe-initialize.md:25` ("before each mutating transaction or explicitly
  justified exclusive-device phase").
- `25_per-device-reset.md:24`.
- `30_cooperative-measurement.md:19`.
- `ENGINEERING.md:108-109`.
- `7_eeprom-reset-policy.md:20`.

**Wrong**
- The prompts alternate between three cadences:
  - check before every write;
  - check once per phase;
  - check before every freshness claim (implied by ENGINEERING).
- No read path ever reads 0x04. So "[do not] claim that a latest register read
  is a valid new conversion in that state" cannot be enforced as written.
- In continuous mode with EUN=1 the library cannot stop conversions: the MOD=01
  write would itself program EEPROM.

**Why it matters**
- EUN changes only through a write to 0x04 (SNOSD82D pp.18, 30).
- The owner contract already excludes other writers during an operation.
- 7:20 itself admits that a pre-write read cannot stop a second master.
- Per-write checks therefore add traffic without any time-of-check benefit, and
  the coder has to pick one of three variants.

**Fix** — replace ENGINEERING:108-109 and add to 7 a single rule:

> Read 0x04 once per mutating synchronous call or job, before its first write
> and in the same exclusive phase. Cache the result. Invalidate it on rebind,
> invalidate, reset, any ambiguous transfer failure, or an observed CONFIG
> busy=1. With EUN=1 write nothing — not even shutdown — and return
> `EEPROM_UNSAFE`. Read paths do not poll 0x04; samples carry the last observed
> lock state and its age as provenance. This guards against stale state, not
> against concurrent writers.

### SAF-5 · Medium · Soft-reset write encoding and effects are unspecified, yet stage 25 states the reload as fact

**Where**
- `25_per-device-reset.md:22`, `:24-25`, `:29`
- `ENGINEERING.md:111-112`
- `SILICON.md:108-109`
- `CLI_CONTRACT.md:23`
- `docs/reference/extracted/tmp117-coverage-audit.md:105`

**Wrong**
- 25:22 allows reset only if "the exact datasheet proves a safe … write
  encoding". Yet 25:25 states "The sensor reloads its EEPROM-backed state" as
  fact.
- The datasheets define only bit 1. They do not specify:
  - the other 15 bits of the reset write, or whether those bits take effect;
  - whether Soft_Reset reloads EEPROM or clears EUN;
  - whether the device ACKs during the reset.
- The coverage audit lists "clears EUN" for all reset domains.

**Why it matters**
- SNOSD82D p.28 Table 7-6 and SNIS236 p.35 Table 8-5 say only: "When set to 1
  it triggers software reset with a duration of 2 ms. This bit will always read
  back 0."
- Clearing EUN is attributed only to the general-call reset (SNOSD82D p.18,
  SNIS236 p.20).
- The reload is implied only by the generic sentence "On reset, the device goes
  through a POR sequence…" (SNOSD82D p.18).
- The likely outcomes are bad either way. One coder gates reset off
  permanently, leaving the CLI `reset` command dead. Another invents an
  encoding. The fakes end up encoding "soft reset clears EUN".

**Fix** — freeze in stage 25:
- The reset write word is the last verified stable CONFIG with MOD ∈ {00, 01},
  bits 15:12 = 0 and bit 0 = 0, ORed with 0x0002. Never use MOD=11. Write it
  only after the SAF-4 guard.
- Record as UNKNOWN:
  - whether the non-reset bits apply;
  - EEPROM reload by Soft_Reset (expected, but not stated);
  - the effect on EUN;
  - bus availability during reset.
- After a policy wait of at least 2 ms, require 0x04 busy=0 and a full
  readback. Treat every register as unknown until it is read.
- Never describe soft reset as a relock.
- Correct coverage-audit:105 to "general-call reset clears EUN (SNOSD82D p.18);
  Soft_Reset effect on EUN undocumented".
- The fakes parametrize these outcomes.

### SAF-6 · Low · No recovery is stated after an observed EUN=1, and "factory locked" is imprecise

**Where**
- `ENGINEERING.md:99-100`, `:107`
- `7_eeprom-reset-policy.md:20`
- `FULL_COVERAGE.md:39` (R13)

**Wrong**

EUN is volatile and resets to 0 at power-on reset and general-call reset
(SNOSD82D pp.18, 30). The library has no in-band way to clear it:

- it may not relock (R13 denies every 0x04 write);
- it may not issue a general call;
- it may not soft-reset (a CONFIG write under EUN=1 programs EEPROM).

The only recovery is an application power cycle, and no prompt says so.

**Fix**
- Add to ENGINEERING and 7: "EUN powers up 0. An observed EUN=1 is fail-closed
  until the application power-cycles the sensor or an authorized external tool
  relocks it. The library offers no relock, and Soft_Reset is not a relock
  path."
- Replace "EEPROM is factory locked" with "EUN powers up locked (0)".

---

## 2 · Device behaviour and timing

### BEH-1 · High · MOD readback during and after a one-shot is undocumented but used as evidence

**Where**
- Prompts that rely on it:
  - `8_api-architecture.md:21`
  - `19_config-trust.md:23`
  - `21_fresh-one-shot.md:18-19`
  - `30_cooperative-measurement.md:28-31`
- Shared contracts that rely on it:
  - `ENGINEERING.md:71-75`
  - `CONVENTIONS.md:43-46`
  - `FULL_COVERAGE.md:31` (R05, "OS returns SD")
  - `CLI_CONTRACT.md:41-45`
- UNKNOWN lists that omit it:
  - `SILICON.md:135-137`
  - `ENGINEERING.md:78-80`
  - `6_timing-status.md:31`
  - `32_variant-model-fixtures.md:30`

**Wrong**

Three mechanisms assume that MOD reads 01 after a completed one-shot and that
MOD=11 means "in progress":

- the pre-trigger "verify physical shutdown" check;
- stage 19 drift detection ("completion matches desired shutdown");
- campaign restore.

**Why it matters**
- All three datasheets say only that after completion the device "goes to" or
  "returns to the low-power shutdown mode" (SBOS740A p.14 §7.4.1.5; SNOSD82D
  p.15 §7.4.3; SNIS236 p.17).
- The MOD field tables (SBOS740A p.26 Table 6; SNOSD82D p.27 Table 7-6;
  SNIS236 p.34 Table 8-5) give only "10 … (reads back = 00)".
- Nothing states:
  - the MOD value while a one-shot runs or after it ends;
  - whether MOD can read 01 while a residual conversion is still running;
  - what writing MOD=11 during a one-shot does.

If the silicon keeps MOD=11 after completion, every back-to-back managed
one-shot is refused as "unowned MOD=11", and stage 19 reports drift after each
one-shot. Separately, a residual conversion after a cancel or timeout can set
DATA_READY after the baseline read. It would then be credited to the new
trigger as fresh.

**Fix**
1. Add both of these to every UNKNOWN list above:
   - "MOD readback during/after a one-shot";
   - "MOD=11 write during a running one-shot".
2. **Config trust.** After an owned one-shot whose DATA_READY was observed,
   accept MOD ∈ {01, 11} as matching desired shutdown.
3. **Baseline before the trigger.** If both of these hold, one CONFIG read
   (which consumes DATA_READY) is the baseline:
   - this instance's last owned trigger is older than N_avg × t_conv,max;
   - no unowned access has occurred since.

   Otherwise:
   - write the desired CONFIG with MOD=01 (guarded; SNOSD82D p.14 §7.4.2:
     shutdown "instantly aborts the currently running conversion");
   - then read CONFIG;
   - then trigger.
4. Never use MOD alone as evidence of completion or abort.
5. **Evidence.**
   - Fakes model both readback outcomes.
   - HIL in stages 40/41 records MOD before the trigger, mid-conversion and
     after DATA_READY, per exact part.

### BEH-2 · High · The driver's own CONFIG reads silently acknowledge alert-mode flags, with no retention rule

**Where**
- Prompts that read CONFIG:
  - `19_config-trust.md:22` ("Verification must record these effects")
  - `21_fresh-one-shot.md:22`
  - `22_limits-alert.md:19` (`AlertStatus` from its own source read)
  - `30_cooperative-measurement.md:19-20`
  - `18_safe-initialize.md:24` (readback)
  - `26_diagnostic-access.md` (diagnostic CONFIG reads)
- Shared contract: `ENGINEERING.md:85-90`, `:141-142`.
- Only `9_owner-deadline-contract.md:22` asks to "retain read-to-clear status in
  an owner-visible observation", and no implementing stage defines that.

**Wrong**
- Many paths read CONFIG:
  - readiness polls;
  - the one-shot baseline;
  - try-fresh;
  - begin/recover readback;
  - diagnostics;
  - campaign snapshots.
- No prompt requires the HIGH/LOW bits captured by those reads to reach the
  alert API.
- A last-read snapshot is overwritten by the next read, which returns 0.

**Why it matters**
- Flags are evaluated "At the end of every conversion, including averaging"
  (SNOSD82D p.15 §7.4.4). That is the same instant DATA_READY is set (p.27).
- HIGH_Alert and LOW_Alert are "Cleared on read of configuration register"
  (p.27).
- p.16: a CONFIG read "also clears the status flags" and deasserts ALERT.
- SBOS740A pp.15/26 and SNIS236 pp.18/34 say the same.

In alert mode, the read that observes DATA_READY for a conversion erases that
conversion's out-of-range event. With DR/Alert=0 it also releases the ALERT pin
within one poll. Managed measurement and alert monitoring destroy each other's
evidence. A failed composite operation that leaves "outputs unchanged on
failure" can also drop flags it has already consumed.

**Fix**
- **Stage 19 bookkeeping.** In the central destructive-read bookkeeping, add one
  sticky per-instance `AlertEvidence` record:
  - OR-accumulate HIGH/LOW from every successful CONFIG read while T/nA=0, on
    every path: managed, diagnostic, job, and failed composite;
  - store first and last observation time, T/nA at capture and profile
    generation;
  - set a "possibly lost" bit on failed CONFIG reads.
- **Stage 22.** The alert read returns pending plus live flags and clears the
  record exactly once.
- **Stage 30.** Results carry the record.
- **Symmetric case.** A DATA_READY=1 seen by an alert or diagnostic read feeds
  the same-generation readiness bookkeeping.
- **Doxygen (21/22).** Document that CONFIG polling acknowledges alert-mode
  flags and releases ALERT. When the pin drives an interrupt, recommend
  DR/Alert=1 with a GPIO, or therm mode.
- **Test.** An out-of-range one-shot in alert mode yields HIGH exactly once to
  the application.

### BEH-3 · Medium · The continuous try-fresh baseline and the "first completion after a change" are undefined

**Where**
- `21_fresh-one-shot.md:18` (the rule is written for one-shot only).
- `20_conversion-settings.md:20`, `:25`.
- `23_offset-feature.md:23`.
- `30_cooperative-measurement.md:18-19`, `:24`.
- `CLI_CONTRACT.md:19`.

**Wrong**

No rule says when DATA_READY=1 in continuous mode proves that a sample belongs
to the current profile:

- after bind;
- after outside access;
- after a CONV/AVG/offset write.

Whether a CONFIG write restarts the averaging block in flight is undocumented,
and correctly retained as unknown. As a result some coders will accept a
straddling conversion, and others will make try-fresh never succeed.

**Fix** — define in 6 and 21:
- Try-fresh accepts DATA_READY=1 only after a baseline. The baseline is this
  instance's own successful CONFIG or TEMP read under the current profile
  generation, with no unowned access since.
- After bind, after outside access, or after any CONV/AVG/offset write in
  continuous mode:
  - the first read only establishes the baseline;
  - the first following completion is discarded (straddle rule).
  - Alternatively, use the documented boundary: write SD, then continuous mode.
- Apply the same rule to stage 23's "subsequent confirmed conversion".

### BEH-4 · Medium · The continuous cycle has no specified maximum, so deadlines would be built from nominal values

**Where**
- `ENGINEERING.md:57-58` ("applicable maximum timing for bounds").
- `6_timing-status.md:23-24`.
- `20_conversion-settings.md:18`, `:21`.
- `21_fresh-one-shot.md:18` (`readBlocking`).
- `CLI_CONTRACT.md:19`.

**Wrong**
- MIN/MAX exist only for a single conversion:
  - SBOS740A p.5: 13.5/15.5/17 ms;
  - SNOSD82D p.6: 13/15.5/17.5 ms, with the condition "One-shot mode";
  - SNIS236 p.7: 13/15.5/17.5 ms.
- The CONV×AVG tables (SBOS740A p.27 Table 7; SNOSD82D p.28 Table 7-7; SNIS236
  p.35 Table 8-6) give one nominal value per cell.
- In active-limited cells the real period is N × t_conv. For example,
  CONV=000/AVG=11 is 64 × (13…17.5 ms) = 832–1120 ms on TMP117/TMP119, against
  a table value of "1 s".
- SNOSD82D p.28: "If the time to complete the conversions … is higher than the
  conversion setting cycle time, there will be no stand by time."

**Fix** — state in 6 and 20:
- Cycle values are nominal; no maximum is specified.
- Deadline = max(nominal × (1 + labelled policy margin), N_avg × t_conv,max) +
  policy slack.
- Active-limited cells use N_avg × [t_conv,min, t_conv,max].
- The margin is library policy. The typical standby-drift curves (SBOS740A p.9
  Fig 14; SNIS236 p.11 Fig 6-10) are rationale, not guarantees.
- Record the TMP117 row's "One-shot mode" condition wherever that row is
  applied to continuous sub-conversions.

### BEH-5 · Medium · Therm mode with TLOW > THIGH is undefined, and the limit write order is left free

**Where**
- `22_limits-alert.md:18`, `:27`
- `ENGINEERING.md:46-48`
- `FULL_COVERAGE.md:38` (R12)
- `41_hil-volatile-soak.md:24`

**Wrong**
- Stage 22 says any low/high ordering restriction is "explicit library policy".
- That holds for alert mode, which makes two independent comparisons (SNOSD82D
  p.16). It does not hold for therm mode:
  - therm mode sets HIGH when the result "exceeds" THIGH and clears it when the
    result "goes below" TLOW (p.17);
  - behaviour for THIGH < result < TLOW is not specified;
  - non-atomic limit writes can pass through an inverted pair.

**Fix**
- Add "therm mode with TLOW > THIGH" to the UNKNOWN list.
- Policy: in therm mode, reject an inverted pair with zero I2C.
- Order the writes so TLOW ≤ THIGH holds at every intermediate step:
  - when raising both limits, write THIGH first, then TLOW;
  - when lowering both limits, write TLOW first, then THIGH.
- The fakes model the overlap as unspecified.

### BEH-6 · Medium · "Shutdown flag retention" is listed as wholly unknown, but therm-mode retention is documented

**Where**
- `ENGINEERING.md:78-80`
- `SILICON.md:135-137`
- `6_timing-status.md:31`
- `32_variant-model-fixtures.md:30`
- `docs/reference/extracted/tmp116-coverage-audit.md:89-92`

**Why**
- SNOSD82D p.17 §7.4.4.2: "The HIGH_Alert status flag is only set or cleared
  at the end of conversions based on the value of the temperature result
  compared to the high and low limits." SBOS740A p.16 and SNIS236 p.19 say the
  same.
- The only documented clear path for the alert-mode flags is a CONFIG read
  (p.27).
- An "adverse" simulator that clears therm HIGH on entry to shutdown would
  therefore contradict the datasheet.

**Fix**
- State as fact: therm-mode HIGH_Alert, and the ALERT pin, persist through
  shutdown and aborted conversions until a later conversion falls below TLOW.
- Narrow the unknown to "whether entering SD clears DATA_READY or the
  alert-mode HIGH/LOW flags".
- Restrict stage 32's adverse shutdown outcomes to those bits.

### BEH-7 · Medium · One-shot completion rests on a single piece of evidence, with no fallback

**Where**
- `21_fresh-one-shot.md:21`, `:25`, `:33-37`
- `30_cooperative-measurement.md:35`
- `ENGINEERING.md:59`, `:69-70`
- `6_timing-status.md:14-17`, `:26`

**Wrong**
- Completion is proven only by observing DATA_READY. Retrigger and time-based
  completion are both forbidden.
- An E2E lead suggests that DATA_READY can be cleared incorrectly
  (`docs/reference/extracted/intake-notes.md:66-70`). It is unverified: the
  page returned HTTP 403.
- If that race is real, some one-shots never show DATA_READY, and their
  completed results are discarded as timeouts.
- The datasheets do not specify what happens when the set and the
  clear-on-read coincide (SNOSD82D p.27).

**Fix**
- Stage 6 sets a poll-spacing policy: first poll no earlier than
  N_avg × t_conv,min after the acknowledged trigger, then sparse polling (see
  also BEH-8).
- Define a distinct terminal outcome, e.g. `READY_NOT_OBSERVED`.
  - It may return the TEMP word read after N_avg × t_conv,max as an unconfirmed
    one-shot (`freshConversion=false`), with provenance.
- Prefer DR/Alert=1 GPIO completion when the application supplies it.
- HIL measures the missed-ready rate.
- Keep "no automatic retrigger".

### BEH-8 · Medium · TMP119's "avoid I2C during an active conversion" never reaches the polling stages

**Where**
- `ENGINEERING.md:57-58` ("Use minimum/typical timing for polling schedules").
- `6_timing-status.md:23`.
- `21_fresh-one-shot.md:18`.
- `30_cooperative-measurement.md`.
- `docs/reference/extracted/tmp119-coverage-audit.md:57`, which routes the
  recommendation only to stages 34–36 and 40–42.

**Why**
- SNIS236 p.23 §7.5.3.1.1: "For best precision, TI recommends to avoid I2C
  communication during an active conversion".
- SNIS236 p.29 and SBOS740A p.35 warn against heavy bus traffic.
- A MIN-based earliest poll (13 ms × N) puts reads inside the typical active
  window (15.5 ms × N).

**Fix**
- Stage 6 sets the default earliest DATA_READY poll to at least
  N_avg × 15.5 ms (typical) for all models; this is mandatory for TMP119.
- A MIN-based schedule becomes an opt-in latency policy.
- Record polling during a conversion as an accuracy caveat in
  `docs/device-behavior.md`.
- Stages 21 and 30 consume that policy.

### BEH-9 · Low · Some test obligations are impractical or redundant

| Where | Problem | Fix |
|---|---|---|
| `FULL_COVERAGE.md:61` (F06 lists all 32 CONV×AVG combinations for owners 21/30) | One-shot duration depends on AVG only (SNOSD82D p.15). | One-shot tests: 4 AVG values plus a CONV-independence property. Keep the 32 combinations for continuous timing in stages 20/32. |
| `FULL_COVERAGE.md:64` (F09 "equality", owner 41) | Exact equality with a limit code cannot be produced on hardware. Repeatability is ±3 LSB without averaging and ±1 LSB with 8 (SNOSD82D p.12). | Close equality by simulator plus the datasheet wording ("exceeds", "lower than"). HIL equality is optional. |
| `23_offset-feature.md:27`, `41_hil-volatile-soak.md:19` | Positive offset saturation is testable at room temperature (offset ≥ +231 °C). Negative saturation needs an ambient below 0 °C. | Add positive saturation as a stage 41 trial. Mark negative saturation NOT RUN without a chamber. |
| `40_hil-readonly.md:25` ("status-flag read-to-clear checks") | Factory THIGH 0x6000 = +192 °C and TLOW 0x8000 = −256 °C (SNOSD82D p.29) never trip at room temperature without limit writes. | Stage 40 qualifies DATA_READY clearing only. Alert-flag clearing moves to 41. |

### BEH-10 · Low · TMP116 does not state that the comparison waits for the full average

**Where**
- `20_conversion-settings.md:24`
- `6_timing-status.md:30`
- `32_variant-model-fixtures.md:28`
- `docs/reference/extracted/tmp116-coverage-audit.md:84-85`

**Why**
- SNOSD82D p.15 and SNIS236 p.17: "At the end of every conversion, including
  averaging … compares".
- SBOS740A p.14 §7.4.2 says only "At the end of every conversion, the TMP116
  compares". Its p.11 updates TEMP "at the end of every conversion or averaging
  cycle".

**Fix**
- Record this as a TMP116 wording gap.
- Apply block-end comparison to TMP116 as a labelled inference in fakes and
  docs.
- HIL check: AVG=64 with a threshold near ambient; the flag should change only
  together with DATA_READY.

---

## 3 · Datasheet facts, source conflicts and citations

### SIL-1 · Medium · TMP114's electrical limits are missing from the coexistence and fixture guidance

**Where**
- `SILICON.md:20`
- `docs/reference/extracted/excluded-families-audit.md:30-32`, `:103-111`
- `4_family-identity.md:27`
- `13_identity-selection.md:24`
- `32_variant-model-fixtures.md:29`

**Why**
- **TMP114 voltage limits.** SNIS214E p.4 gives an SCL/SDA absolute maximum of
  −0.3 to 2.1 V and a supply/I/O range of 1.08–1.98 V.
- **TMP114 addresses.** SNIS214E p.3: only TMP114A–D use 0x48–0x4B; TMP114NA–ND
  use 0x4C–0x4F.
- **TMP116 supply.** TMP116 needs V+ ≥ 1.9 V (SBOS740A p.4).
- **TMP118.** Its I/O tolerates 6 V (SNIS237A p.6), and its A–D/MA–MD variants
  use 0x48–0x4B (p.23).
- **Consequence.** A TMP114 on a 3.3 V TMP11x bus exceeds its absolute maximum,
  so a "same-bus" TMP114 fixture is electrically possible only below 1.98 V.

**Fix**
- State that a TMP114 can share a bus only with pull-ups ≤ 1.98 V, and that only
  TMP114A–D alias 0x48–0x4B.
- TMP118 is the realistic same-bus or wrong-part collision.
- Frame the TMP114 negative fixture as "wrong part populated on a low-voltage
  bus".

### SIL-2 · Low · Retained-conflict rows are missing for EEPROM-stored fields and EEPROM word count

**Where**
- `SILICON.md:65-68`, `:102-104`
- `5_register-ledger.md:22`, `:27-28`
- `docs/reference/extracted/tmp117-coverage-audit.md:193-211`
- `docs/reference/extracted/tmp119-coverage-audit.md:36`

**Why**
- **Stored fields.**
  - SNOSD82D p.18 §7.5.1.1 and SNIS236 p.20 list the stored power-on values as
    limits, cycle, averaging, mode, alert mode and polarity. They omit DR/Alert
    and Temp_Offset.
  - The detailed tables mark both as storable: SNOSD82D p.27 footnote (1) on
    DR/Alert; p.25 `07h 0000h(1)`; SNIS236 pp.32/34.
- **Word count.** The four-word references are not only prose:
  - SNOSD82D p.20 Fig 7-8 says "EEPROM1 to 4", and p.30 says "EEPROM[4:1] are
    preprogrammed".
  - SNIS236 p.22 Fig 7-9 and p.37 say the same.

**Fix**
- Add paired-citation conflict rows.
- Policy: the detailed register tables and footnotes govern persistence domains,
  so DR/Alert and the offset are treated as EEPROM-loaded.

### SIL-3 · Low · "Writes ignored during the power-on load" is generalized across models and registers

**Where**
- `18_safe-initialize.md:19-20` ("all describe writes ignored during the
  initial load interval").
- `FULL_COVERAGE.md:60` (F05).
- `32_variant-model-fixtures.md:21`.
- `docs/reference/extracted/tmp119-coverage-audit.md:52`.

**Why**
- SBOS740A p.17: "Any I2C writes performed during this initial period to the
  registers are ignored" — that is, all registers.
- SNOSD82D p.18 and SNIS236 p.20: "…to the limit registers or the configuration
  register are ignored". The offset register is not mentioned.

**Fix**
- Make the ledger rows per model.
- On TMP117/TMP119, an offset write during the load window is unknown: it may
  be ignored, applied then overwritten, or applied. The fakes must not
  hard-code "ignored".
- Keep the mandatory readback after the startup guard.

### SIL-4 · Low · TMP117 grade differences are misdescribed

**Where:** `SILICON.md:18`, `:122`

**Why**
- SNOSD82D p.46: TMP117, TMP117M and TMP117N are all orderable in both DRV
  (WSON) and YBG (DSBGA).
- p.6: the grades differ in their accuracy rows. The TMP117M orderable range is
  0–85 °C (p.46).
- The "Thermal Pad unsoldered (DRV Package)" condition cell spans the rows of
  every grade. YBG has no pad.

**Fix**
- SILICON:18 → "Same programming model and packages (DRV/YBG); accuracy rows
  differ; TMP117M orderable range 0–85 °C."
- SILICON:122 → "The unsoldered-pad accuracy condition applies to the DRV
  package of every grade."

### SIL-5 · Low · The TMP116 "p.36 unused ALERT is unconnected" claim does not exist in the datasheet

**Where**
- `SILICON.md:127`
- `docs/reference/extracted/tmp116-coverage-audit.md:178`

This was added by the feature-gap audit.

**Why**
- SBOS740A p.36 prose covers only the RC filter and the thermal pad.
- TMP116's only qualifier is p.3: "requires a pullup resistor if used".
- The sentence "The ALERT pin can be left floating or connected to ground when
  not in use" is in SNOSD82D p.33 (and SNIS236), not in SBOS740A. The lead
  confirmed this by text search.

**Fix:** replace the clause with:

> p.3 requires a pull-up if used. TMP116 gives no explicit disposition for an
> unused pin, and the p.32 grounding sentence has no "unused" qualifier. The
> TMP117/TMP119 floating-or-ground text is not TMP116 evidence.

### SIL-6 · Low · The F04 "STOP/START behaviour" is applied to TMP116 without evidence

**Where:** `FULL_COVERAGE.md:59` (F04)

**Why**
- SNOSD82D p.20 and SNIS236 p.23 state that the device ignores traffic until a
  START and resets its state machine on every STOP.
- SBOS740A pp.19-23 have no equivalent; they document only the timeout reset
  (p.21).

**Fix**
- Make F04 per model.
- Do not assert STOP-reset behaviour in the TMP116 adapter or HIL criteria.

### SIL-7 · Low · The TMP119 summary table misstates other parts

**Where**
- `SILICON.md:125`
- `docs/reference/extracted/tmp116-coverage-audit.md:130-131`
- `4_family-identity.md:19`, `:29`

**Why**
- SNIS236 p.4 Table 4-1 lists TMP116/TMP116N VDD as 3.0–3.6 V and the TMP119
  shutdown current as 0.25 µA.
- SBOS740A p.4 gives 1.9–5.5 V for TMP116.
- SNIS236 p.8 gives 0.15 µA typical for TMP119.

**Fix:** extend the SILICON row: "Never source any model's metadata from
SNIS236 Table 4-1; use the exact-part datasheets."

### SIL-8 · Low · The excluded parts' 0x0F response is documented, the CRC-enabled TMP114 case is missing, and bound-only dumps are not identity-gated

**Where**
- `docs/reference/extracted/excluded-families-audit.md:107-111`
- `8_api-architecture.md:23`
- `26_diagnostic-access.md:18`, `:22`, `:25`, `:30`
- `CLI_CONTRACT.md:20`

**Why**
- SNIS214E p.24 §8.5.4.3: "Reading from a non-indexed register location will
  return 00h". SNIS237A p.23 says the same. A TMP114 or TMP118 therefore
  returns 0x0000 at pointer 0x0F. This is documented behaviour, not a fixture
  assumption.
- With CRC enabled, a TMP114 "will NACK after the restart … and send FFh"
  (SNIS214E p.26).
- TMP114 register 0x02 (Alert_Status) is read-to-clear (pp.29, 32). A
  bound-only dump on a mis-bound TMP114 clears its flags and decodes foreign
  data.

**Fix**
- Cite the documented 0x0000 for both parts.
- Add a CRC-enabled TMP114 fixture: an address NACK or 0xFFFF yields a
  transport error or mismatch, with no retry.
- Multi-register bound-only routes read 0x0F first. They stop on a transport
  failure or family mismatch, unless an explicit override marks the output
  "unverified/foreign".

### SIL-9 · Low · The ledger conflates the access type with the read side effect

**Where:** `5_register-ledger.md:20` ("Record CONFIG R/C DATA_READY …")

**Why**
- SNOSD82D p.27, SBOS740A p.26 and SNIS236 p.34 type HIGH_Alert, LOW_Alert and
  Data_Ready as "R".
- An "RC" code exists in the access-type legends but is not used for these
  fields.
- The clearing behaviour is stated only in prose.

**Fix:** the ledger keeps two fields, each with its page:
- the documented access code ("R");
- the documented read side effect: CONFIG/TEMP reads clear DATA_READY; CONFIG
  reads clear bits 15:14 in alert mode only.

### SIL-10 · Low · Citation and wording corrections

| Location | Current | Replace with |
|---|---|---|
| `SILICON.md:47-49` | "(PDF page index 23/24/31)", zero-based | The one-based page numbers used everywhere else |
| `docs/reference/extracted/tmp119-coverage-audit.md:76` | "p.35 labels CONV as standby-related" | p.34 Table 8-5 |
| `7_eeprom-reset-policy.md:18`, `12_i2c-register-transport.md:20`, `24_readonly-eeprom.md:23`, `26_diagnostic-access.md:14`, `:29`, `32_variant-model-fixtures.md:21` | "0x07 alias" | "model-dependent 0x07 (TMP116 EEPROM3 / TMP117–TMP119 Temp_Offset)". Nothing aliases; "alias" invites address-aliasing logic, which R18 forbids. |
| `CLI_CONTRACT.md:22` | "Explicit persistent volatile-setting changes" | "Explicit volatile-register setting changes that remain the desired profile (never written to NVM)" |
| `4_family-identity.md:19`, `13_identity-selection.md:14` | Linux code is used to "confirm" revision-nibble semantics | "Record how Linux matches IDs as secondary evidence; revision semantics come from SNOSD82D p.32 Table 7-15 and SNIS236 p.39 Table 8-14." |
| `docs/reference/manifest.json:55` vs `docs/reference/extracted/tmp117-coverage-audit.md:15-16` | The reviewed-page lists disagree: [25, 27, 28, 32] vs pp.6, 7, 23, 27, 28, 31, 46 | Reconcile into one list. |
| `docs/reference/manifest.json:269-276` (SDAA311) | Pinned as the alert application note | Mark it "generic portfolio guidance, not authoritative for TMP11x". Its p.10 says active-high ALERT "drives high", and p.5 lists TMP119 with a hysteresis register. TMP11x ALERT is open-drain and therm hysteresis uses TLOW. |
| Startup timing (not cited anywhere) | — | Add SNOSD82D p.7: tRESET 1.5 ms is a TYP value with no MAX. This is a third non-maximum startup figure, alongside the p.12 and p.18 prose. |

---

## 4 · API, state and ownership

### API-1 · High · The synchronous engines are not required to be resumable

**Where**
- `18_safe-initialize.md:24`, `:27`
- `21_fresh-one-shot.md:18`, `:24`
- `29_cooperative-config.md:18-19`
- `30_cooperative-measurement.md:18-19`
- `HANDOFFS.md:28-30`

**Wrong**
- Stages 18 and 21 ask for bounded synchronous engines with waits and a clock.
- Stages 29 and 30 must then "reuse … do not duplicate" them as separately
  budgeted poll steps.
- A straight-line synchronous implementation with internal waits cannot be
  driven by `poll(nowMs, maxTransfers)`. Stages 29/30 then either rewrite the
  engine or duplicate it, and duplication is forbidden by HANDOFFS.

**Why it matters**

TMP1x2 solves exactly this with one private step engine shared by the
synchronous path and the jobs:

- `include/TMP1x2/TMP1x2.h:376-377`: `beginApply`/`stepApply`;
- `src/TMP1x2Operations.cpp:47`, `:134`.

**Fix** — add to 18 and 21:

> Implement apply/readback and acquisition as a private resumable step engine.
> Each step performs at most one callback, and waits are explicit states with a
> due time. The synchronous API drives it in a bounded clock loop. Stages 29/30
> add only admission and poll glue. Test the step engine directly, with
> transfer counts.

### API-2 · High · The offset default and ownership are undefined, so begin() would overwrite an EEPROM-programmed calibration

**Where**
- `8_api-architecture.md:30-31`
- `11_status-values.md:21`
- `18_safe-initialize.md:31`
- `23_offset-feature.md:19`, `:26`
- `FULL_COVERAGE.md` rows R16 and F11
- `HANDOFFS.md:61`

**Wrong**
- 23:19 requires connecting "every accepted value, including zero" to
  apply/replay, so that final-profile trust includes the offset.
- No prompt defines the default value, or any "offset not managed" state.

**Why it matters**
- Temp_Offset is EEPROM-loaded at reset (SNOSD82D p.25 footnote 1; SNIS236
  p.32). A user may have programmed a calibration there; 23:26 itself
  anticipates a "nonfactory EEPROM-loaded offset".
- With a plain `offsetC = 0` default, every `begin()`/`recover()` on
  TMP117/TMP119 silently replaces that calibration with 0 in the volatile
  register.
- On TMP116, the same Config field is either silently ignored (forbidden) or
  makes the default Config invalid.

**Fix** — freeze in stage 8 that offset control is opt-in, e.g.
`bool manageOffset = false; float offsetC;`:
- **Unmanaged (default).**
  - Never write 0x07.
  - On TMP117/TMP119, read 0x07 as an observation so the samples' offset
    provenance is known.
  - On TMP116, never touch 0x07.
- **Managed on TMP116.** `manageOffset = true` returns `UNSUPPORTED` at
  validation, with zero I2C.
- **Managed on TMP117/TMP119.** Full replay, readback and dirty handling, as
  already specified.

### API-3 · Medium · Health-state semantics before initialization conflict with the cited precedent

**Where**
- `CONVENTIONS.md:15`
- `15_passive-health.md:18-22`
- `14_binding-lifecycle.md:21`
- `HANDOFFS.md:53`

**Wrong**
- CONVENTIONS calls `UNINIT/READY/DEGRADED/OFFLINE` "passive transport health"
  and cites TMP1x2.
- In TMP1x2 the state moves only while initialized: `src/TMP1x2.cpp:126` and
  `:133-135`, `if (_initialized) _state = …`.
- Stage 15 runs before any initializer exists. It must test "device
  disappearance, first tracked success, an ordinary tracked success from
  OFFLINE" without a fake begin.
- Under the TMP1x2 semantics, those transitions cannot be reached before
  stage 18.

**Fix** — freeze in stage 8 and CONVENTIONS:
- `DriverState` is transport health only.
- UNINIT means no tracked outcome since bind, unbind or invalidate.
- Tracked outcomes drive the state whether or not the instance is initialized.
- `initialized` and `isOnline()` are separate fields.
- Record this as a deliberate deviation from TMP1x2.

Alternative: move the state-transition tests to stage 18 and limit stage 15 to
the counters.

### API-4 · Medium · The instance's own destructive reads are not blocked while a job is active

**Where**
- `28_cooperative-admission.md:19`
- `30_cooperative-measurement.md:22`
- `21_fresh-one-shot.md:21`, `:26`
- `ENGINEERING.md:23-30`

**Wrong**
- Stage 28 rejects only "conflicting mutating operations/rebinds" while a job
  is active.
- A synchronous TEMP, CONFIG, identity, EEPROM or diagnostic read during a
  one-shot or try-fresh job consumes the job's DATA_READY.
- 30:22 tries to tolerate this after the fact.

**Why**

TMP1x2 returns BUSY for managed calls, raw access and probe while a job or
result is pending (`docs/owner-operations.md:104-106`; `src/TMP1x2.cpp:113`).

**Fix**
- While a job is active, every method of that instance that performs I2C
  returns `BUSY` with zero I2C.
- Getters, `cancel`, `poll` and `takeResult` remain available.
- Decide explicitly whether a pending (not yet taken) result also blocks I2C.
- Restrict 30:22 to outside access.

### API-5 · Medium · There is no gating table for dirty, invalidated or cancelled state, so the one-shot trigger can silently reapply unverified fields

**Where**
- `18_safe-initialize.md:27`
- `19_config-trust.md:19`, `:21`
- `21_fresh-one-shot.md:18`
- `25_per-device-reset.md:24-25`
- `29_cooperative-config.md:20`
- `31_failure-recovery.md:21`

**Wrong**
- The prompts define when dirty is set and cleared, but not which operations
  refuse to run in these states:
  - while dirty;
  - after `invalidateDeviceState()`;
  - after a cancelled configure or recover;
  - after a reset.
- The trigger write carries every stable CONFIG field, but stage 21 verifies
  only MOD.
- A mismatched AVG, CONV or T/nA is therefore silently rewritten, and the
  "fresh" result is attributed to an unverified profile.

**Why**

TMP1x2 requires a clean configuration for one-shot and READ (`TMP1x2.h:278`;
`TMP1x2.cpp:115-116`).

**Fix** — add a gating table to stage 8:
- **Latest read:** allowed; the Sample is marked untrusted.
- **Try-fresh, one-shot, blocking reads and fresh jobs:** reject with zero I2C
  (e.g. `CONFIG_DIRTY`).
- **Recover:** always allowed.
- The pre-trigger verification read compares all stable fields. On a mismatch
  it marks the profile dirty and refuses the trigger.

### API-6 · Medium · The `nowMs` arguments conflict with the authoritative clock hook

**Where**
- `9_owner-deadline-contract.md:18`, `:20`
- `ENGINEERING.md:131-135`
- `CONVENTIONS.md:18`, `:31-33`
- `27_sync-helpers-cache.md:19`

**Wrong**
- Stage 9 requires a clock hook for every deadline-bearing operation.
- Yet `start*` and `poll` keep `nowMs` arguments that "must share one epoch",
  which the core cannot verify.
- Behaviour when the argument and the hook disagree is undefined. The same
  applies to age checks.
- No per-API table says which synchronous operations require the clock.

**Why**

TMP1x2 ignores the argument whenever the hook exists
(`src/TMP1x2Operations.cpp:33`, `:194`; `src/TMP1x2.cpp:93`).

**Fix**
- State: "The clock hook is authoritative; `nowMs` is ignored for deadlines
  when the hook is set." Alternatively, drop `nowMs` as a documented deviation.
- Add a per-API clock-requirement table.
- A missing required clock returns `INVALID_CONFIG` before any I2C.

### API-7 · Medium · The startup timing origin is not an API input

**Where**
- `6_timing-status.md:25`
- `8_api-architecture.md:18`
- `14_binding-lifecycle.md:20`
- `18_safe-initialize.md:24`, `:26`, `:35-36`
- `31_failure-recovery.md:20`
- `FULL_COVERAGE.md:60` (F05)

**Wrong**
- Stage 18 must refuse writes while startup readiness is unestablished.
- Readback cannot prove that a write escaped the load window. Reads are allowed
  during the window, and TMP116's busy flag is contradictory.
- No Config input gives the timing origin (power-applied time, or "powered for
  at least X ms").
- Nothing says how `recover()` re-establishes the origin after a power cycle
  detected through a readback mismatch.
- On TMP116 this can mean permanent refusal.

**Fix**
- Stage 8 freezes an origin source, e.g. `UNKNOWN` / `AT_BIND` /
  `CALLER_TIMESTAMP`. When the origin is unknown, apply a conservative policy
  wait counted from the first observation.
- After a detected loss, `recover()` starts a new conservative wait before
  writing: the job stays pending, and a synchronous call runs a bounded loop.
- On TMP116, never rely on busy==0 alone.

### API-8 · Medium · bind() accepts invalid profiles, and "commit on first write" conflicts with bind

**Where**
- `14_binding-lifecycle.md:18`
- `18_safe-initialize.md:24`, `:27`, `:42`
- `HANDOFFS.md:52`, `:56`

**Wrong**
- 18's exit text freezes "profile validation failures after a structurally
  valid bus-silent bind". So bind accepts, for example, NaN limits, and
  `begin()` fails until a rebind.
- "Commit a staged sensor target on the first mutating write attempt; before
  that … preserves the old target" cannot hold for `begin(Config)`: bind has
  already replaced the target.
- Dirty state across a rebind to another model or address is unspecified.

**Why**

TMP1x2 validates the full Config at bind (`src/TMP1x2.cpp:31`).

**Fix**
- Stage 18 extends bind validation to the full accepted profile. It stays
  bus-silent and zero-I2C, and keeps the prior binding on rejection.
- `bind` and `begin(Config)` set the target directly. The first-write commit
  rule applies only to setters and `startConfigure`.
- Define: a rebind discards the old target's dirty and observed state.

### API-9 · Medium · Capability and register-access facts are owned in three places

**Where**
- `11_status-values.md:19-20`
- `12_i2c-register-transport.md:19-20`
- `17_config-codecs.md:18`
- `23_offset-feature.md:20`
- `24_readonly-eeprom.md:22`
- `26_diagnostic-access.md:21`
- `HANDOFFS.md:51-52`

**Wrong**
- Three stages own overlapping facts:
  - Stage 11 owns "the single pure model/capability table".
  - Stage 12's `CommandTable.h` adds field/access metadata and variant
    distinctions.
  - Stages 23/24 add a "shared capability/write-policy seam".
- The TMP116 0x07 = EEPROM3 fact is encoded three times, with no derivation
  rule.
- Stage 11 does not consume `variant-contract` or `register-ledger`.

**Fix**
- Stage 12 derives per-model access from stage 11's capability rows.
- One write-policy function in stage 12 is the only gate later stages extend.
- A native test cross-checks `CommandTable.h` against
  `docs/registers/tmp11x.json`.
- Add `variant-contract` and `register-ledger` to stage 11's Consumes.

### API-10 · Medium · Some API surface has no concrete caller

AGENTS.md:58-60 says: "Add abstractions only for a concrete current caller."

**Where**
- `21_fresh-one-shot.md:18`
- `26_diagnostic-access.md:19`, `:21`
- `27_sync-helpers-cache.md:18`
- `HANDOFFS.md:64`
- `CLI_CONTRACT.md:25`

**Wrong**
1. **Raw volatile writes.** An optional raw write facility, although typed
   setters cover every writable field and the CLI forbids a low-level bypass.
2. **Two diagnostic read routes:** health-neutral bound-only reads, plus an
   "optional tracked readback".
3. **A public manual one-shot trigger and readiness API.** It sits beside
   `readBlocking` and the one-shot job, with no specified interlocks against
   admission, rebind, recover, invalidate and end. TMP1x2 needed those
   interlocks (`TMP1x2.h:151`, `:195-196`; `TMP1x2.cpp:21`, `:80-83`).
4. **Aliases.** Stage 27 invites them.

**Fix**
- Exclude raw writes and add an X row recording the exclusion.
- Keep one diagnostic read route.
- Either drop the public manual trigger/readiness API, or specify its
  interlocks.
- Add aliases only for a named consumer.

### API-11 · Medium · "Blocking read" is ambiguous against the requirement for distinct acquisition requests

**Where**
- `8_api-architecture.md:20`
- `21_fresh-one-shot.md:18`, `:24`
- `CONVENTIONS.md:18`
- `CLI_CONTRACT.md:18-19`, `:23`

**Wrong**
- Stage 8 requires distinct latest, try-fresh and one-shot requests.
- Stage 21 adds an unspecified `readBlocking`.
- CONVENTIONS lists a single `startRead`.
- TMP1x2's `readBlocking` dispatches on mode: continuous returns the latest
  value, shutdown runs a one-shot (`TMP1x2.h:286-291`;
  `TMP1x2Measurement.cpp:137`).
- Copying it makes a "read" mutate hardware depending on the desired mode.

**Fix**
- Name the requests:
  - `readLatest`;
  - `tryReadFresh` / `readFreshBlocking` (continuous mode, no write);
  - `readOneShotBlocking`;
  - `startRead(ReadKind::{LATEST, FRESH, ONE_SHOT}, …)`.
- No mode-based dispatch.

### API-12 · Medium · The frozen `Sample` fields cannot carry the required provenance

**Where**
- `CONVENTIONS.md:17`
- `21_fresh-one-shot.md:20-21`
- `23_offset-feature.md:23`
- `27_sync-helpers-cache.md:19`
- `30_cooperative-measurement.md:20`, `:24`

**Wrong**

`CONVENTIONS.md:17` fixes `Sample{raw,counts,celsius,timestampMs,timestampValid,freshConversion}`.
Later stages require each sample to carry more:

- acquisition kind;
- profile generation;
- configuration trust at acquisition;
- offset provenance.

**Fix**
- Add `source` (latest/fresh/one-shot), `profileGeneration`, `configTrusted`
  and `offsetProvenance` to `Sample`, or nest them in a `SampleProvenance`.
- Restrict `freshConversion = true` to try-fresh and one-shot results with
  hardware evidence.

### API-13 · Medium · State updates after a late but physically successful transfer are undefined

**Where**
- `ENGINEERING.md:139-142`
- `9_owner-deadline-contract.md:20`
- `29_cooperative-config.md:22`
- `30_cooperative-measurement.md:20`

**Fix**
- **What a late observation updates.** Late observations update only the
  observed-value and flag records, including the BEH-2 alert evidence.
- **What it must not do.** It does not clear dirty, set initialized, publish a
  sample or update the cache.
- **What the result carries.** The physical transport status and
  `hardwareEffectPossible`.
- **Precedent.** TMP1x2 `src/TMP1x2Operations.cpp:97-102`, `:224-229`.

### API-14 · Low · Wording that cannot be implemented or tested

| Location | Problem | Replace with |
|---|---|---|
| `31_failure-recovery.md:22` "overflowed time domain … explicitly rejected" | A gap of 2^32 ms or more is undetectable with a 32-bit clock. | "Unsigned elapsed ≥ timeout (including apparent backward steps) expires; gaps ≥ 2^32 ms are the owner's obligation." |
| `12_i2c-register-transport.md:21`, `ENGINEERING.md:9` "timeout including lock acquisition" | This is an adapter contract; the core can only pass the clipped value. | "The core passes the clipped remaining budget; stages 34/35 enforce it, including the lock wait." |
| `21_fresh-one-shot.md:22` "capture status before it clears" | The read returns the word and clears the bits in the same transaction. | "Use the returned CONFIG word." |
| `14_binding-lifecycle.md:19` "stage 15 connects their health-neutral accounting" | There is nothing to connect. | "Remain health-neutral; stage 15 asserts it." |

### API-15 · Low · Stage 28 uses a private test harness, and stages 14/19 add guard seams with no caller

**Where**
- `28_cooperative-admission.md:18`, `:23`, `:31`
- `14_binding-lifecycle.md:18`
- `19_config-trust.md:21`

**Wrong**
- The full synchronous API exists by stage 27, yet stage 28 tests the job
  lifecycle through a "private lifecycle harness". That needs a test-only job
  kind or a friend hook inside the core.
- Stages 14 and 19 build job-slot guard seams before any job exists.

**Fix**
- Stage 28 connects the single-transfer LATEST read job as the first real job
  kind and tests the lifecycle against it.
- Stages 14 and 19 document the policy only.

---

## 5 · Promptset structure and consistency

### CON-1 · High · The remote-CI policy contradicts itself, and the CI workflow has two owners

**Where**
- `AGENTS.md:11-22` vs `:24-26`
- `COMMON.md:114-119`
- Every prompt's boilerplate, e.g. `10_package-foundation.md:38`: "Do not run
  unrelated sibling suites or remote CI (except stage 43)".
- `README.md:96-97`
- `VERIFICATION.md:36-38`
- `10_package-foundation.md:23`
- `43_ci-checkpoint.md:18`

**Wrong**
- AGENTS requires CI for major changes ("use existing CI … dispatch it if
  needed") and defers to COMMON only for "the comprehensive remote check".
- The prompts forbid remote CI except at stage 43.
- Stage 10, "Set up test/CI scaffolding … without remote-CI dispatch until
  stage 43", invites creating `.github/workflows/ci.yml` early.
- Stage 43 then says "Create or complete" the workflow and preserve its push
  triggers.
- No workflow exists today: `.github/` is absent on HEAD and on `origin/main`.

**Impact**

One of two things happens:

- A push-triggered workflow runs red through stages 10–42, because its
  Arduino, IDF and Doxygen jobs target things that don't exist yet, while the
  prompts forbid watching it.
- Agents dispatch CI ad hoc at stages they consider "major".

**Fix**
- AGENTS:24-26 →

  > During numbered-prompt execution, COMMON's checkpoint policy overrides the
  > major-change CI rule. Stages 1–42 use local checks only and do not
  > dispatch, poll or wait on remote CI. Stage 43 creates the workflow and runs
  > the checkpoint. The major-change rule applies from stage 43 on, including
  > stage 44 fixes.

- 10:23 → "local test scaffolding; do not add `.github/workflows/*` (stage 43
  creates it)".
- Boilerplate → "Do not dispatch, poll or wait on remote CI (except stage 43)."

### CON-2 · Medium · Stage 44 reopens stage 43 after the final audit

**Where:** `COMMON.md:117-119`; `44_final-independent-audit.md:22-23`

**Wrong**
- Stage 44 is the last stage, so a reopened stage 43 would run after the final
  audit.
- The final report's SHA and residual list would then predate the CI result
  they depend on.

**Fix:** 44:22 →

> After CI-covered fixes, execute the stage 43 checkpoint procedure for the
> resulting SHA within this stage. Record that SHA's terminal CI status (or
> BLOCKED) in the final report before closing 44.

### CON-3 · Medium · Firmware CLI ownership: early consoles, a double attach, and no owner after stage 38

**Where**
- `34_arduino-examples.md:22`
- `35_idf-examples.md:20`
- `37_cli-baseline.md:10`, `:18`
- `38_cli-device-operations.md:24-25`
- `HANDOFFS.md:72-77`, `:89-93`
- `COMMON.md:106-108`

**Wrong**
- **Early consoles.**
  - Stage 34 allows an "initial console harness" before the parser exists, but
    37:18 forbids "a second command parser".
  - Stage 35 must share "prompt-37's processor" before that processor exists.
- **Double attach.** Both 37:10 ("Attach the stage 34/35 adapters") and 38:24
  ("Integrate the shared processor …") claim the wiring.
- **No owner after stage 38.**
  - Stages 34/35 may resume after stage 38 is already COMPLETE, for example
    once an SDK is installed later.
  - In that case no stage integrates the final CLI or updates
    `docs/firmware-cli-handoff.md`.
  - Stage 40's gate, "one current CLI-integrated image from 34 or 35 after 38",
    then has no producer.

**Fix**
- **Stages 34/35** deliver a fixed non-interactive demo (bind, observe, print),
  byte I/O hooks, and identity and observation-startup hooks. They do no
  command parsing.
- **Stage 37** defines the processor and the backend line-I/O interface, host
  only.
- **Stage 38** alone wires the processor into both backends.
- **Add to 34/35:** "If stage 38 is already COMPLETE, integrate the final CLI
  and update `docs/firmware-cli-handoff.md` in this stage."

### CON-4 · Medium · "TMP117 first, then TMP116/TMP119" contradicts every stage's all-model obligations

**Where**
- `PROGRESS.md:4`; `START_HERE.md:46-48`.
- Contrast `11_status-values.md:19-20`, `13_identity-selection.md:22`,
  `23_offset-feature.md:20`, `24_readonly-eeprom.md:20` and
  `27_sync-helpers-cache.md:21`, which all require every model.

**Wrong**

No stage adds TMP116/TMP119 later. A coder who follows the entry text
implements TMP117 only in stages 11–31. That fails the exit criteria of stages
13, 23, 24 and 27.

**Fix:** replace both with: "TMP117 is the primary and physically intended
model; every stage implements and tests the capability gates of all six BOM
models."

### CON-5 · Medium · The shared fake register device has no producer before stage 32

**Where**
- `32_variant-model-fixtures.md:21`
- `HANDOFFS.md:70` ("built incrementally since 12")
- `12_i2c-register-transport.md:23`
- `21_fresh-one-shot.md:33`
- `30_cooperative-measurement.md:35`

**Fix**
- Stage 12 produces an artifact `register-fake` (e.g.
  `test/support/FakeTmp11xDevice.{h,cpp}`) with literal, ledger-derived
  behaviour.
- Stages 13–31 consume and extend it.
- Stage 32 consolidates it.
- Mirror the change in SEQUENCE, the prompts and HANDOFFS.

### CON-6 · Medium · The stage 5 validator's regression tests have no runner

**Where**
- `5_register-ledger.md:24` ("register its regression tests")
- `10_package-foundation.md:20`
- `COMMON.md:85-86`
- `VERIFICATION.md:52`

**Fix**
- 5:24 → "Add `tools/test_check_register_ledger.py`, run as
  `python tools/test_check_register_ledger.py`."
- 10:20 → "Register the stage 5 validator and its tests in the host
  entrypoint."
- Add the command to VERIFICATION and to COMMON:85.

### CON-7 · Medium · Five different vocabularies for the evidence-level columns

| Location | Columns |
|---|---|
| `COVERAGE.md:4-5` | documented, implemented, native tested, firmware built, CI tested, hardware qualified |
| `FULL_COVERAGE.md:99-100` | documented, implemented, host-tested, MCU-linked, remote-CI, physically qualified |
| `EVIDENCE.md:26-27` | documented / implemented / host-tested / firmware-built / physically-qualified (no CI column) |
| `42_package-docs.md:22` | documented/implemented/native-tested/MCU-built/physical-verified |
| `44_final-independent-audit.md:19` | documented/implemented/native/firmware/CI/hardware |

**Fix**
- Freeze one set in FULL_COVERAGE:
  `documented | implemented | host-tested | MCU-linked | remote-CI | physically-qualified`.
- Replace the other four lists with "the FULL_COVERAGE evidence columns".

### CON-8 · Medium · FULL_COVERAGE and COVERAGE owners do not match the prompts that do the work

**Where**
- `FULL_COVERAGE.md:30`, `:42`, `:62`, `:68`, `:85-87`, `:92`
- `COVERAGE.md:17`, `:22`, `:31`, `:38`

**Wrong**
- **Listed owners whose prompt never addresses the row:**
  - R04 ← 19
  - F07 ← 17, 18, 19, 22, 31
  - F13 ← 4, 6
  - X03 ← 32
  - X08 ← 5, 6, 33
- **Prompts that do the work but are not listed:**
  - X03 ← 33 (33:26), 38 (38:29)
  - X01, X02 ← 38 (38:29)
  - X02 ← 18 (18:38)
  - R16 ← 25, 26, 38, 41
- **COVERAGE.md** diverges further and nothing links to it. Only the checker's
  `REQUIRED_SHARED` list keeps it alive.
- **COMMON.md:11-15** (the entry reading list) omits COVERAGE and
  CLI_CONTRACT.

**Fix**
- Apply the owner changes above.
- For COVERAGE.md, either:
  - mark it "non-normative summary; FULL_COVERAGE owners govern", link it from
    README and regenerate its owner lists; or
  - delete it and remove it from `REQUIRED_SHARED`.
- Add CLI_CONTRACT to COMMON:11-15.

### CON-9 · Medium · The checker has blind spots, and HANDOFFS overstates what it verifies

**Where**
- `scripts/check_promptset.py:99-117`, `:225-233`
- `HANDOFFS.md:9-11`

**Wrong** — the checker:
- compares only the HANDOFFS "Produced IDs" column, not paths or consumer
  prose, which have already drifted (CON-12);
- does not validate R/F/X IDs, their counts or owner ranges;
- will not check that `docs/feature-coverage.md` preserves every ID;
- strips `#page=N` anchors;
- cannot detect glued text (CON-11).

**Fix**
- HANDOFFS:9 → "agreement of this table's Produced IDs column".
- Add to `validate()`:
  - R01–R18, F01–F20 and X01–X08 are unique, and owners are within 1..44;
  - once `docs/feature-coverage.md` exists, every ID is present in it;
  - backticked HANDOFFS paths appear in the producing prompt;
  - `#page=N` is within the page count recorded in the manifest;
  - the pattern `[a-z;][0-9]{1,2}([/–][0-9]{1,2})*\b` is flagged in HANDOFFS and
    FULL_COVERAGE.
- Add matching negative tests to `scripts/test_check_promptset.py`.

### CON-10 · Medium · Stages 38 and 42 are too large for one work unit

**Where**
- `38_cli-device-operations.md:18-29`: 10 obligations, including two firmware
  backends and the handoff document.
- `42_package-docs.md:18-27`: 9 obligations — install/export, IDF and Arduino
  metadata, two tools, three consumer types, a licence audit, full Doxygen, ten
  documentation areas and a coexistence sample.
- `33_native-suite.md`: its title promises packaging gates that it defers to
  stage 42.

**Fix**
- Move backend integration out of stage 38 (see CON-3).
- Give stage 42 two mandatory sub-checkpoints, each verified, committed and
  reported separately:
  - package and consumer qualification;
  - documentation and Doxygen.

  This avoids renumbering.
- Retitle stage 33 "native correctness and source-consumer gates".

### CON-11 · Low · Stage numbers glued to words, and other compressed tokens

`HANDOFFS.md:51-82` contains 33 tokens like these. The same problem appears in:
- `FULL_COVERAGE.md:31`, `:33`, `:37`, `:43`, `:58`, `:60`, `:87`;
- `5_register-ledger.md:27`, `35_idf-examples.md:25`, `36_p4-consumer.md:24`,
  `43_ci-checkpoint.md:14` and `44_final-independent-audit.md:14`.

| Current | Replace with |
|---|---|
| "integration belongs14", "lands18", "until23" | "belongs to stage 14", "lands in stage 18", "until stage 23" |
| "Supplies23/27/30", "hand off to28–30" | "supplies stages 23/27/30", "hands off to stages 28–30" |
| "from 34 OR35 after38", "requires40's" | "from stage 34 or 35 after stage 38", "requires stage 40's" |
| "10 aliases CC and reads00" | "MOD=10 is a continuous alias that reads back 00" |
| "bit1 soft-reset always reads0" | "bit 1 soft reset always reads 0" |
| "full 0x1116 vs117-family low12 DID/revision" | "full 16-bit 0x1116 versus the 117-family low-12-bit DID plus revision nibble" |
| "Upper nibble0", "approximate1.5 ms", "Counts1/8/32/64" | "upper pointer nibble 0", "approximate 1.5 ms", "counts 1/8/32/64" |
| "no ARA handle in35" | "stage 35 creates no ARA (0x0C) device handle" |
| "Audit CI43 and physical40/41" | "Audit the stage 43 CI record and the stage 40/41 physical records" |
| "DP8382x stage71 CI contract" | "DP8382x `docs/prompts/71-ci-checkpoint.md` at the pinned SHA" |

### CON-12 · Low · Consumes lists are incomplete, and the HANDOFFS consumer prose has drifted

- **Missing Consumes.**
  - Stage 7 runs the validator but does not consume `register-validator`.
  - Stage 18 needs `latest-read-engine` and `behavior-contract` (stage 6
    readiness) but does not declare them.
  - Stages 22 and 23 extend stage 18's engine without consuming
    `safe-apply-engine`.
  - Stages 38, 42 and 43 use the host entrypoint without consuming
    `host-test-entrypoint`.
  - Stage 43 builds the Arduino target and refreshes the stage 38 handoff, but
    consumes neither `arduino-adapter-source` nor `firmware-cli-handoff`.
- **Drifted consumer prose.**
  - 6:28 says stages 16–23 and 32 consume the vectors; SEQUENCE lists only 16,
    20, 22 and 32.
  - HANDOFFS:56 and :59 consumer lists differ from SEQUENCE.
- **Fix.**
  - Add the IDs; dependency reachability already permits them.
  - Replace the HANDOFFS prose consumer lists with "consumers: see SEQUENCE".

### CON-13 · Low · The entry prompts' authorization scope is ambiguous

**Wrong**
- The resume prompt (`START_HERE.md:85-90`) does not say whether it covers one
  stage or all remaining stages.
- The preparation prompt says "Stop before driver implementation or API freeze"
  (`:17-18`). That would allow stages 4–7, while `COMMON.md:5` limits
  preparation to stages 1–3.

**Fix**
- Resume prompt: "Resume the earliest usable incomplete stage; continue past it
  only if this message is the whole-set instruction."
- Preparation prompt: "Stop after prompts 1–3."

### CON-14 · Low · Some names and paths have no defining stage

| Name or path | Problem | Fix |
|---|---|---|
| `-DTMP11X_BUILD_TESTS=ON` (`VERIFICATION.md:53`) | No stage defines the option. | Stage 10 names it, together with the single host test command, in `docs/testing.md`. |
| Host test entrypoint | Never named. | Stage 10 names it, in the same `docs/testing.md` entry. |
| HIL runner file (stage 39) | Unnamed. | Stage 39 names `tools/hil_tmp11x_runner.py`. |
| `EVIDENCE.md:21` "or an equally narrow ledger" | Conflicts with the fixed ledger path. | Delete the phrase. |
| `COMMON.md:63-64` CHANGELOG | Required from stage 1, although CHANGELOG exists only from stage 10. | Add "once created". |
| Doxyfile | Has no producer. | See TOOL-9. |

### CON-15 · Low · Report locations, status vocabulary and branch wording drift

| Item | Fix |
|---|---|
| Stage 44 writes two reports: `docs/reports/final-audit.md` (44:23) and `docs/reports/prompt_44.md` (44:47). | Keep a single report, `prompt_44.md`. |
| No stage maintains `docs/reports/index.md`. | Add an index update to the COMMON checkpoint. |
| `NOT_STARTED` (COMMON:17) vs "NOT STARTED" (PROGRESS). | Use one spelling. |
| 39:21 adds "SKIP" to the result vocabulary. | Map SKIP to NOT RUN, or define it in COMMON. |
| The boilerplate says "on `main`"; AGENTS:32 says "existing branch". | "On the existing checked-out branch". |
| AGENTS:55-57 says HANDOFFS assigns producers; HANDOFFS:4-6 says SEQUENCE declares them. | "SEQUENCE owns ordering, dependencies and producer/consumer IDs; HANDOFFS explains boundaries and integration owners." |
| ENGINEERING:114 says ordinary writes canonicalize MOD=10; 17:20 says encoders "may canonicalize". | Use "must emit 00 for continuous" in both places. |

### CON-16 · Low · Stale records

- **Sibling SHAs disagree across three documents.**
  - The documents: `SOURCE_MAP.md:31-32`, `docs/toolchain-and-package-plan.md:19-20`
    and `docs/reports/promptset-audit.md:105-107`.
  - DP8382x: d58a7da, 87aafc0 and ced8b1c respectively.
  - MotorControl-CAN: 1b1d352 in SOURCE_MAP; 60bcefe in the other two.
  - Fix: keep one revision table, in the toolchain plan, and link to it from
    SOURCE_MAP.
- **A wrong FieldCore-node claim.** `docs/reports/promptset-audit.md:109-111`
  says the local checkout "differs from remote". In fact the local checkout is
  on a different branch, which matches its own upstream, and local `main`
  matches `origin/main`. Fix: record the branch.
- **preparation.md is stale.** `docs/reports/preparation.md` still says stage 3
  is PARTIAL, contrary to `docs/reports/prompt_3.md` and PROGRESS. Fix: mark
  those paragraphs superseded.
- **PROGRESS and the stage 1–3 reports are behind the current rules.**
  - PROGRESS's last update predates `73b55d2` (the AGENTS CI-policy rewrite)
    and `19facbe` (the PDF renames).
  - The stage 1–3 reports lack the artifact IDs that COMMON:95-97 requires.
  - `docs/reports/index.md` says "23 regressions"; the current count is 27 + 9.
  - Fix: update all three.
- **A wrong VERIFICATION claim.** `VERIFICATION.md:62` says
  "adapted `scripts/pio.cmd`", but `docs/source-adoption.md` says "copied
  unchanged". See also TOOL-1.

---

## 6 · Tooling, CLI, HIL and CI

### TOOL-1 · High · `scripts/pio.cmd` launches a stale PlatformIO Core

**Where**
- `scripts/pio.cmd:4`
- `AGENTS.md:108`
- `docs/toolchain-and-package-plan.md:69`, `:91-94`
- `docs/reports/prompt_2.md:14`
- `docs/reports/preparation.md:63`
- `VERIFICATION.md:61-62`
- Prompts 10, 34 and 43.

**Wrong**

The wrapper hard-codes `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`. On
the development host, the IDE-managed installation is the one selected by the
user-level `PLATFORMIO_CORE_DIR`, and its penv is Core 6.2.0. The wrapper
instead runs a leftover Core 6.1.19 against that same core directory.

- The lead verified both versions:
  - `scripts\pio.cmd --version` → 6.1.19;
  - `%PLATFORMIO_CORE_DIR%\penv\Scripts\pio.exe --version` → 6.2.0.
- The toolchain plan's package and compiler rows were read from the inactive
  `~/.platformio/packages`.

**Impact**
- Every PlatformIO build in stages 34, 42 and 43 runs a different Core than the
  IDE. That is exactly the "unrelated executable" AGENTS forbids.
- The evidence records the wrong tool and package versions.

**Fix**
- `pio.cmd`:
  - if `PLATFORMIO_CORE_DIR` is set, use
    `%PLATFORMIO_CORE_DIR%\penv\Scripts\pio.exe`;
  - otherwise use `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`;
  - fail if the chosen executable is missing.
- Record the local modification in `docs/source-adoption.md`.
- Regenerate toolchain-plan rows 91-94 from `pio system info` using the
  corrected wrapper, and correct the prompt_2 and preparation claims.

### TOOL-2 · High · Fixture, serial-port and firmware-upload authorization is undefined

**Where**
- `START_HERE.md:31` ("Do not ask for routine approval")
- `39_python-hil-runner.md:18`, `:20`
- `40_hil-readonly.md:12`, `:16`, `:25`
- `41_hil-volatile-soak.md:18`, `:20`, `:24`
- `CLI_CONTRACT.md:45-46`
- `VERIFICATION.md:39-42`

**Wrong**
- The physical stages require a "user-authorized" fixture or "explicit fixture
  authorization", but never define what that is.
- They also never say which serial port may be opened, or when firmware may be
  uploaded.
- Other projects' HIL benches are attached to the same development host.

**Impact**

In an unattended whole-set run, an agent may treat any enumerated ESP32 as the
fixture. It could then flash TMP11x firmware onto another project's bench, or
run campaigns on an unrelated bus. Alternatively, it can never resolve
"authorized" at all.

**Fix** — add to COMMON, VERIFICATION and stage 39:

> Physical work requires a user-written fixture record, e.g. an untracked
> `hil/fixture.local.json`. It names the board, serial port, exact part and
> marking, ADD0 address, the other devices on the bus, upload permission, and
> the permitted campaign classes (observe / volatile / power-cycle).
>
> Without that record:
> - do not select a port by enumeration;
> - do not upload firmware;
> - do not open any port;
> - mark the stage BLOCKED.
>
> The runner refuses physical mode unless `--port`, `--model` and `--address`
> match the record.

### TOOL-3 · Medium · The canonical CLI table lacks lifecycle, configuration-job, job-pacing, stop and framing commands, and `shutdown` duplicates `mode shutdown`

**Where**
- `CLI_CONTRACT.md:9-26`, `:29-33`, `:37-39`
- `37_cli-baseline.md:19`, `:21`
- `38_cli-device-operations.md:18`, `:23-24`
- `41_hil-volatile-soak.md:19`
- `FULL_COVERAGE.md:69` (F14)
- `14_binding-lifecycle.md:21` ("offer explicit shutdown later")

**Wrong**
- **Missing commands.** The table forbids competing spellings, but it omits:
  - bind/unbind/begin/end, so stage 41 has no contracted way to initialize;
  - the configuration job kinds from stage 29;
  - a job-pacing policy. Only a manual `job step` exists, so serial latency
    becomes part of operation deadlines;
  - a way to stop `watch`;
  - request-ID framing.
- **`shutdown` vs `mode shutdown`.** `shutdown` is listed as a separate
  "explicit device mutation", while `oneshot` requires *desired* shutdown.
  - If `shutdown` does not change the desired mode, then `shutdown` followed by
    `oneshot` is rejected.
  - If it does change the desired mode, it is an alias with no alias contract.
- **No owner.** Stage 14's "explicit shutdown later" has no owner.

**Why**

The TMP1x2 CLI has `bind`/`unbind`, `stop`, and `job auto [0|1]` (default 1).
It also notes that manual jobs need suitable deadlines
(`examples/common/Tmp1x2CliOutput.cpp:62`, `:78`, `:84`, `:131`).

**Fix**
- **Add rows to the canonical table:**
  - bus-silent `bind <model> <addr>`, `unbind` and `end`;
  - mutating `begin`;
  - `job start init|configure|recover|read|tryfresh|oneshot [timeout_ms]`;
  - `job auto [0|1]`, default 1. Manual `job step` requires an explicit
    deadline;
  - a bus-silent `stop` for watch and campaigns;
  - a request-ID prefix or `mark <token>`.
- **Settle `shutdown`.** Either define it as an exact alias of
  `mode shutdown`, owned by stage 20, or remove it.

### TOOL-4 · Medium · There is no hardware evidence that observation sessions write no register data

**Where**
- `39_python-hil-runner.md:20`
- `40_hil-readonly.md:25`, `:28`, `:30`, `:36`
- `CLI_CONTRACT.md:16`, `:29-33`

**Wrong**
- A logic analyzer is optional (40:30).
- The CLI has no transfer-counter command: `health` counts only tracked
  transfers.

**Why**

TMP1x2 provides `xfer_stats`/`counters` and `xfer_assert` for exactly this
(`Tmp1x2CliOutput.cpp:124-125`).

**Fix**
- Add a bus-silent `xfer` command. It reports adapter counts since boot, tagged
  with the session ID, split into:
  - pointer-only writes (1 byte);
  - data writes (more than 1 byte);
  - reads.
- Stages 39/40 assert a data-write count of 0 from boot to campaign end.

### TOOL-5 · Medium · SDK millisecond-to-tick truncation can time out small budgets after the transfer has started

**Where**
- `35_idf-examples.md:19` (says only "document SDK tick quantization")
- `34_arduino-examples.md:18`
- `36_p4-consumer.md` (reuses the stage 35 adapter)

**Why**
- **Conversion to ticks.** ESP-IDF `i2c_master` converts the timeout with
  `pdMS_TO_TICKS`.
- **Default tick rate.** IDF's default `CONFIG_FREERTOS_HZ` is 100, so budgets
  of 1–9 ms become 0 ticks. The wait then times out while the hardware transfer
  may still complete.
- **SDK bus recovery.** After a timeout, the driver resets its state machine and
  clocks SCL on the next transaction. This is SDK-owned bus recovery.
- **Arduino.** Arduino-ESP32 3.x Wire sits on the same driver.
- **Precedent.** TMP1x2's IDF example sets `CONFIG_FREERTOS_HZ=1000`
  (`examples/esp_idf/basic/sdkconfig.defaults`). The TMP11x prompts don't
  require it.
- **Line numbers.** The reviewer cited lines 588-612 and 683-688 of IDF v5.5.5
  `i2c_master.c`. The lead did not re-verify those line numbers.

**Fix** (stages 34 and 35):
- Require `CONFIG_FREERTOS_HZ=1000` in the IDF `sdkconfig.defaults`.
- Reject locally — zero I2C, typed budget/timeout error — any remaining budget
  below 2 ticks. Never round up past the caller's deadline.
- Document the SDK's automatic bus clear after a timeout in the board guide, as
  SDK behaviour, not as a driver retry.

### TOOL-6 · Medium · The tooling and integration stages are over-scoped for a small sensor library

**Where**
- `36_p4-consumer.md:18-20`
- `34_arduino-examples.md:21`
- `42_package-docs.md:14`, `:18-20`, `:23`
- `VERIFICATION.md:28-29`
- `41_hil-volatile-soak.md:24-25`
- `CLI_CONTRACT.md:70-74`

**Wrong**
1. **P4 example.** It brings in optional TMP1x2 and TCA9548A and interleaved
   mux verification, although no P4 consumer or board exists.
2. **Coexistence.** It is demonstrated three times (stages 34, 36 and 42), plus
   VERIFICATION's consumer layer.
3. **Package-consumer gates.** There are five of them:
   - A relocated `find_package` consumer, with no sibling precedent. 42:14
     cites "DP8382x relocated installation/consumer checks", but DP8382x's
     CMakeLists has no install/export; only a plan prompt exists.
   - A GitHub `lib_deps` consumer, which needs network access and a pushed SHA.
4. **Version checkers.** There are two, although TMP1x2's
   `scripts/generate_version.py check` already performs the drift check.
5. **Physical "research".** It covers behaviour that 41:25 itself admits cannot
   be proven.
6. **Repeated JSONL fields.** Every JSONL record repeats the firmware version,
   model and ID.

**Fix**
1. **P4.** Make it a link-only build of the stage 35 example, with no TMP1x2 and
   no mux.
2. **Coexistence.** Keep one sample, in stage 42.
3. **Consumers.**
   - Make the GitHub `lib_deps` consumer optional or CI-only.
   - Cite DP8382x prompt 70 as a plan only.
4. **Versions.**
   - Make `tools/check_version_sync.py` a thin wrapper over
     `generate_version.py check`, or drop it.
   - List the derived files: Version.h, `idf_component.yml`, the CMake project
     and package-config VERSION, the Doxyfile, and the IDF example
     PROJECT_VER.
5. **Stage 41 research.** "Observe and record if the fixture permits", with no
   exit criterion.
6. **JSONL.** One session header record, plus a session ID on every record.

### TOOL-7 · Low · Stage 36's premises are wrong or vague

**Wrong**
- FieldCore-node is an ESP32-S3 Arduino application. No local sibling has a P4
  or native-IDF consumer.
- "P4 master support" is ambiguous.
- Sibling acquisition is unspecified, which risks `EXTRA_COMPONENT_DIRS`
  pointing at a sibling checkout, contrary to `10_package-foundation.md:10`.
- The P4 silicon revision is not recorded.

**Fix**
- "No P4 consumer exists; FieldCore is an I2C-owner pattern only."
- Replace "P4 master support" with "I2C master driver on ESP32-P4".
- Pin any needed siblings through IDF component-manager git dependencies.
- Record the minimum P4 silicon revision in the build record.

### TOOL-8 · Low · `discover` and mux routing contradict "HIL must not mutate other devices"

**Where**
- `VERIFICATION.md:42`
- `CLI_CONTRACT.md:21`
- `34_arduino-examples.md:21`
- `36_p4-consumer.md:18`

**Why**
- A pointer write of 0x0F to an unknown device at 0x48–0x4B is a data byte for
  that device. Command-byte parts, such as some ADCs and expanders, can
  reconfigure on it.
- Mux route writes also mutate a device.

**Fix**
- VERIFICATION:42 → "must not mutate EEPROM or any device not declared in the
  fixture record; declared mux devices may be written only for routing."
- Exclude `discover` from default campaigns, and probe only addresses listed in
  the fixture record.

### TOOL-9 · Low · No stage produces the Doxyfile, and Doxygen versions are not recorded

**Where**
- `42_package-docs.md:19-22`
- `VERIFICATION.md:58`
- `docs/toolchain-and-package-plan.md:86-96`

**Why**
- TMP1x2's `check_doxygen.py` relies on a Doxyfile whose INPUT includes `docs`,
  with `WARN_AS_ERROR = YES`.
- Copied as-is, it would pull `docs/prompts`, `docs/reports` and
  `docs/reference` into the TMP11x documentation build.
- The local and CI Doxygen versions differ and are not recorded.

**Fix**
- Stage 42 creates a Doxyfile with INPUT = `include/TMP11x` plus the user
  documentation only.
- Record the local and CI Doxygen versions.
- The version generator syncs PROJECT_NUMBER.

### TOOL-10 · Low · The HIL evidence location and the runner's dependency are unspecified

**Wrong**
- `.gitignore:15` ignores `hil_logs/`, and no committed evidence location is
  named.
- Stage 39 does not mark pyserial as optional.

**Fix**
- Name `docs/hil/<date>-<fixture>/` as the evidence location, holding the
  transcript, JSON and SHA-256. Alternatively, keep raw logs ignored and commit
  their hashes and summaries.
- pyserial is required only for physical mode. Dry-run, parsing and unit tests
  use the standard library, as in the TMP1x2 runner.

### TOOL-11 · Low · Miscellaneous

| Location | Problem | Fix |
|---|---|---|
| `35_idf-examples.md:22` | "Component-only builds": an IDF component cannot build without a project. | "A minimal app linking only the component." |
| `42_package-docs.md:19` | The "packaging entrypoint" is undefined. | Name it. TMP1x2 runs `check_package` after `pio pkg pack` in CI. |
| `43_ci-checkpoint.md:14` | Audits `scripts/pio.cmd` for CI, but Linux runners use a pip-installed PlatformIO. | Pin the CI Core version and record its difference from the local Core. |
| `40_hil-readonly.md:24` | Requires the "lot", which may be unreadable on the marking. | Allow "unknown". |
| `docs/reference/README.md:21`, `40_hil-readonly.md:20-24`, `41_hil-volatile-soak.md:18-20` | The TMP117EVM, if used as the fixture: its onboard MSP430 is a second I2C master on the sensor bus, and its GUI can program EEPROM1–3 and lock the offset (SNOU161 pp.3, 22). | Isolate the sensor from the MSP430 (snap-off) or hold the MSP430 in reset. Record possible prior GUI EEPROM programming as non-factory provenance. |

---

## Verified correct (no change needed)

**Structure**
- SEQUENCE ids 1–44 are contiguous.
- Files, titles, phases, types, prerequisite prose and Produces/Consumes match
  across README, the prompts and the HANDOFFS Produced-IDs column.
- Every Consumes is reachable through the dependency closure. There are no
  forward or duplicate producers.
- The counts match what the promptset claims: 60 artifact IDs, 226 consumes
  links, 46 R/F/X rows (18/20/8), 27 + 9 regression tests, 14 PDFs.
- The boilerplate is identical across all 44 prompts.
- Stage 3 COMPLETE is supported by existing artifacts.
- The stage 40 OR gate is worded consistently everywhere.

**TMP117 (SNOSD82D)**
- Registers:
  - register map, resets and access types (p.25);
  - CONFIG layout: writable mask 0x0FFC, storable mask 0x07FC; MOD=10 reads
    back 00; MOD1 is not storable; Soft_Reset lasts 2 ms and reads 0; bit 0 is
    unused (pp.27-28);
  - EUN at bit 15, with busy at bit 14 mirroring CONFIG bit 12 (p.30). EUN=1
    writes program EEPROM;
  - EEPROM programming is 7 ms TYP, with 1,000 writes minimum endurance (p.7);
  - offset format and saturation (p.31);
  - ID: Rev[15:12] and DID 0x117 (p.32).
- Startup:
  - power-on time ≈1.5 ms; tRESET is TYP only, with no MAX (p.7);
  - CONFIG and limit writes are ignored during the load;
  - TEMP reads 0x8000 until the first averaged conversion.
- I2C:
  - 1–400 kHz, 20–40 ms timeout;
  - pointer retention, MSB first, ADD0 address map;
  - the device never drives SCL; no hot-connect;
  - general call 0x00 followed by 0x06; ARA semantics.
- Electrical:
  - supply 1.7 V up to 70 °C, 1.8 V over the full range;
  - the accuracy conditions;
  - one-shot conversion 13/15.5/17.5 ms.

**TMP116 (SBOS740A)**
- 0x07 is EEPROM3; 0x05–0x08 are EEPROM1–4.
- ID 0x1116, with no revision field. CONFIG bits 1:0 are unused.
- The busy conflict (p.17 vs pp.26/29) is real.
- The temperature-range conflict (p.4 vs pp.5/10/43) is real.
- Single conversion 13.5/15.5/17 ms. Supply 1.9–5.5 V.
- 250 nA shutdown current is consistent; TMP116 has no 150 nA conflict.

**TMP119 (SNIS236)**
- ID: 0x0117 in the map vs 0x2117 in the register section.
- The EEPROM2 heading conflict and the four-word prose are both real.
- The p.4 vs §6.5 accuracy disagreement is real.
- TMP119M appears only in the p.7 accuracy rows and is absent from the
  orderables (p.45).
- YBG package only.
- The 1-MHz characterization curve vs the 400-kHz protocol limit is confirmed.

**Behaviour**
- Conversion timing:
  - The CONV×AVG tables are identical across the three parts, all 32 cells.
  - One-shot duration depends on AVG only.
  - OS returns to SD, and cannot be the start-up mode.
  - SD aborts a conversion instantly.
  - Averaging is accumulated, not running.
- Flags:
  - DATA_READY is cleared by CONFIG and TEMP reads.
  - Alert mode compares strictly (> / <) and latches until a CONFIG read.
  - Therm mode: HIGH sets above THIGH and clears below TLOW; LOW is always 0;
    neither a CONFIG read nor ARA clears it.
  - POL and DR/Alert routing; ALERT is open-drain.
- Codec and offset:
  - Codec range −256..+255.9921875, and the literal vectors.
  - The offset sum saturates to 0x7FFF/0x8000, so 0x8000 is valid data after a
    confirmed conversion.
- Genuinely undocumented, as the prompts say:
  - offset/comparator ordering;
  - same-value restart;
  - mid-block AVG/CONV changes;
  - TEMP byte latching;
  - the read-clear edge.

**Excluded families**
- TMP114: ID 0x1114 at 0x0B, CRC, control-byte auto-increment.
- TMP118: ID 0x1180 at 0x0B, reversed 0x02/0x03 limits, a mutating UID
  procedure.
- Both return 0x0000 at pointer 0x0F, and bind/begin traffic cannot mutate
  either.

**Precedent and toolchain**
- These match TMP1x2 `afd5088`:
  - the Status/Err shape;
  - bus-silent bind/unbind/end;
  - the poll budget semantics;
  - exactly-once `takeResult`;
  - late-callback `OPERATION_TIMEOUT`;
  - dirty cleared only by a complete reapply.
- Stage 14's "rejected rebind preserves prior state" improves on TMP1x2.
- All cited TMP1x2, OPT4001, ADS1115, TCA9548A and DP8382x paths exist.
- ESP-IDF 5.4.4–6.1 are installed with `esp32p4` and `i2c_master`.
- The Arduino-ESP32 3.3.x Wire semantics in stage 34 are correct.

## Open evidence (cannot be settled from the pinned documents)

| Question | What settles it | Owner stages |
|---|---|---|
| MOD readback during and after a one-shot; a MOD=11 write during a one-shot | HIL per exact part (BEH-1) | 6 (UNKNOWN entry), 40/41 |
| Soft_Reset EEPROM reload, effect on EUN, effect of the non-reset bits, bus availability during reset | HIL with non-factory volatile values, or a TI query (SAF-5) | 7, 25, 41 |
| An offset write during the power-on load window | HIL (SIL-3) | 6, 41 |
| Therm mode with TLOW > THIGH | HIL (BEH-5) | 6, 41 |
| A coincident DATA_READY set and clear-on-read (the E2E lead returned HTTP 403) | TI E2E access, or a HIL missed-ready rate (BEH-7) | 6, 41 |
| TMP116 comparison after the full average | HIL with AVG=64 (BEH-10) | 6, 41 |
| Whether entering SD clears DATA_READY or the alert-mode flags | HIL (BEH-6) | 6, 41 |
