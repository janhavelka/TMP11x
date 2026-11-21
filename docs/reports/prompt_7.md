# Prompt 7 — EEPROM, identity and reset safety

**Phase a (2026-11-21).** The research brief is ready. The clean entry
revision was `e1a7a2b6e54d04515f11e199b4261fe6e58659f2` on `main`, equal
to the local `origin/main`. Prompt 6's acceptance request matches the commit
message and three paths in Git history. The wrapper recorded upstream
verification for that SHA at 17:00:56 Prague time. The manifest's next entry
after completed stage 6 is stage 7. The policy document was absent at entry.
[Prompt 7](../prompts/7_eeprom-reset-policy.md), [sequence](../prompts/SEQUENCE.json).

## Verified inputs

| Producer | Artifact inspected at the entry revision |
| --- | --- |
| 3 | [TI archive manifest](../reference/manifest.json) and the exact PDFs below. All pinned bytes verified. |
| 4 | [Variant contract](../variants.md#identity-decision). Six BOM models, model-selected comparison and shadow index selection. |
| 5 | [Register ledger](../registers/tmp11x.json), [feature coverage](../feature-coverage.md), [validator](../../tools/check_register_ledger.py) and [regressions](../../tools/test_check_register_ledger.py). The ledger already distinguishes hardware R/W access from release write policy. The checker enforces model maps, guard field layout, CONFIG reset-command role, NVM field masks and protected shadow policy. |
| 6 | [Timing and read effects](../device-behavior.md#startup-and-reset-evidence) and [literally specified behavior vectors](../registers/behavior-vectors.json) at `d7fe1874d98151836080f3929bec8310f460b96d`. Carry forward the clock rounding, FIRST_ACK, guard lifetime and destructive-read handling. |

## Pinned source evidence

| Source | One-based PDF pages and decisions |
| --- | --- |
| [TMP116/N, SBOS740A](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17) | p.6 EEPROM characteristics. pp.17–18 EEPROM load, factory data and general-call reload/relock. p.17 permits reads and says all register writes are ignored during the approximately 1.5 ms load. Its busy-clear statement conflicts with CONFIG bit 12 (p.26) and the `0x04` bit-14 mirror (p.29). CONFIG bits 1:0 are reserved (pp.26–27). Map p.24 lists zero defaults for all four shadow words. Detail pp.29–31 leaves words 1, 2 and 4 unknown and lists zero for word 3. p.31 documents full-word DID `0x1116`. pp.21, 23 cover ARA and general-call reset. p.35 gives the broadcast synchronization example. |
| [TMP117/M/N, SNOSD82D](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18) | p.7 gives typical 1.5 ms reset and EEPROM characteristics. pp.18–19 describe load, unique data and general-call reload/relock. Busy is set during POR, reads are possible and the ignored-write list names CONFIG and limits. Map p.25 and detail pp.30–32 expose three shadow words and OFFSET at `0x07`, despite four-word prose and the p.20 pointer diagram. EEPROM2's zero heading conflicts with its field table and map. CONFIG pp.27–28 marks Soft_Reset R-0 in the diagram and R/W in the detailed field table. The table specifies write-one, 2 ms and read-zero. pp.21–24 cover ARA and general-call reset. p.34 gives the broadcast synchronization example. |
| [TMP119, SNIS236](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20) | p.8 gives typical 1.5 ms reset and EEPROM characteristics. pp.20–21 describe load, unique data and general-call reload/relock. POR busy and ignored-write wording follow TMP117. The map and details (pp.32, 37–39) give three shadows and OFFSET at `0x07`, despite four-word prose and the p.22 pointer diagram. EEPROM2 has the same heading/field conflict. CONFIG pp.34–35 show the same addressed-reset access discrepancy and 2 ms/read-zero command. DID is `0x117` with a separate revision nibble despite reset-word disagreement between pp.32 and 39. pp.23–26 cover ARA and general call. p.29 gives the broadcast synchronization example. |
| [TI traceability overview, SBAT024](../reference/source/NIST-Traceability-for-Temperature-and-Humidity-Sensors-Overview-SBAT024.pdf#page=2) | pp.2–4 explain traceability of a measurement result through documented calibrations and classify the six models as tier 1B. Preserve factory data and original shipment identification. Current register values alone cannot authenticate a factory ID or establish calibration of an installed system. |

In each electrical table the programming time is 7 ms typical with no
specified maximum, write endurance is 1,000 minimum and 50,000 typical,
and retention is 10 years minimum and 100 typical. These are silicon
characteristics. Keep programmed startup values distinct from factory
defaults, including the programmable CONFIG subset, thresholds and
applicable offset. TMP117/TMP119 startup prose omits DR/Alert and OFFSET
from its stored-field list. Their detailed maps and field tables mark
both as EEPROM-backed. The effect of an OFFSET write during the load
remains unverified.

## Sibling precedent

- `janhavelka/24Cxx` at `0bb27f46beb4fbe907beb11d9134de4e1797347d`.
  `docs/integration.md` and `include/EEPROM24Cxx/EEPROM24Cxx.h` support
  application ownership of the device and bus, terminal callbacks, and
  explicit recovery after indeterminate writes. Memory programming,
  write-protect pin handling and address-only ACK polling are outside
  TMP11x's release scope.
- `janhavelka/TMP1x2` at `c91bac63313ea725ce174ab98abc50705c365639`.
  `src/BusOperations.cpp`, its header and `docs/bus-operations.md` show the
  bus-wide reach of general-call reset and ARA and the need for explicit
  invalidation after external effects. Neither helper nor its TMP102/TMP112
  ARA interpretation transfers to TMP11x.

Both read-only checkouts were clean and matched their local upstream refs.
The reviewed paths match their prior source-adoption pins. No sibling
suites ran and no live upstream check was made in this wrapper-owned run.

## Phase b brief and acceptance cases

Produce `docs/eeprom-and-reset-policy.md` and refine the existing ledger's
policy, access and reset records. Reconcile the related feature coverage
and validator assertions where affected. Keep these as research artifacts.
Stage 45 reviews the preparation range. Stages 18–25, 26, 29–33 and 38 own
future integration and executable callback evidence.
[Handoffs](../prompts/HANDOFFS.md), [engineering](../prompts/ENGINEERING.md).

- **Model and intent.** Exercise all six model selections. Guard identity at
  the bound seven-bit address and retain the compatible raw DID/revision.
  Reject incompatible identity before follow-on access. TMP116/N shadow
  indices map to `0x05/06/07/08`. The other models use `0x05/06/08`;
  allow typed, opt-in OFFSET changes only there. Include arbitrary shadow
  words, zero and externally changed contents. Every shadow write and
  every data write to EEPROM_UL is forbidden, including attempted relock.
- **Guard and ownership.** At entry to every mutating or freshness-producing
  operation, require valid owner/route and startup readiness plus one complete
  `0x04` word with EUN and busy clear (masks `0x8000`, `0x4000`). When
  startup proof is absent, anchor FIRST_ACK to a compatible read's completion,
  apply stage 6's rounded 3 ms policy wait with stable power, then obtain the
  guard. Cover locked/busy transitions, unknown lock, expired budget, failed
  reads and untimed try-fresh. Timed locked/busy paths can repeat only paced,
  bounded `0x04` reads. A failed transfer terminates the attempt. CONFIG
  cannot supply the guard. EUN=1 refuses even shutdown or reset and stays
  fail-closed until an application power cycle or authorized external relock,
  followed by invalidation and renewed startup/ownership evidence. A prior
  operation's guard cannot authorize a new one. Invalidate on reset,
  interference, ambiguous transfer or subsequent busy. Diagnostics may
  observe with unknown startup/configuration and report no fresh acquisition.
- **Failure and recovery.** A data write with ambiguous effect leaves affected
  state untrusted. Explicit recovery checks identity/startup/guard and verifies
  replay of only the owned volatile profile. Cover nonfactory thresholds,
  unmanaged offset preservation, and destructive CONFIG/TEMP diagnostics.
  One application authority must exclude other access to the sensor during
  the operation. A per-transfer bus lock cannot exclude a second master
  between the EUN check and the write.
- **Reset and interference.** TMP116/N addressed reset returns unsupported
  before I²C. On supported parts use one explicit addressed CONFIG command
  with last verified stable settings, canonical continuous or shutdown MOD,
  status/reserved bits zero and bit 1 set. Treat companion-bit application,
  reload, EUN and ACK availability as unknown. Invalidate before the attempt
  including on ambiguous completion. Require the separate rounded 2 ms wait
  and postwait observations, a new FIRST_ACK and guard for later writes.
  The always-zero command read is insufficient. Cover changed and unchanged
  settings and EUN/busy outcomes in later fakes. Record known external power
  loss, reset, ARA and writes as interference with the affected evidence.
- **Forbidden transcripts and dispositions.** Review all planned synchronous,
  diagnostic, recovery, job and CLI routes. Enumerate X01–X08 from
  [FULL_COVERAGE](../prompts/FULL_COVERAGE.md#excluded-or-application-owned-hardware-functions).
  Include zero traffic to general-call address `0x00` with `0x06`, which
  is distinct from the valid TEMP pointer `0x00) at the selected sensor
  address. Include no ARA at `0x0C`, acknowledging the lowest-address winner's
  documented threshold ALERT effect, and no broadcast synchronized-conversion
  procedure. Cover forbidden raw writes and EEPROM programming on any fixture.
  Reserve production traffic assertions for their implementation stages.

## Phase a verification

Python 3.12.10 on Windows.

| Command | Result |
| --- | --- |
| `python -B scripts/check_references.py` | PASS, 14 pinned PDFs with matching sizes and hashes. |
| `python -B scripts/check_promptset.py` | PASS, 55 stages including navigation and PDF page links. |
| `python -B tools/check_register_ledger.py` | PASS, six BOM models with correct field layouts, release gates and conflict citations. |
| `python -B tools/test_check_register_ledger.py` | PASS, 22 existing ledger regressions. |
| `git diff --check`, `git diff --no-index --check -- NUL docs/reports/prompt_7.md` | No whitespace diagnostics. The no-index comparison reports the new file differs from the empty side. |

No owned software prerequisite is blocked. The POR and addressed-reset
evidence limits remain open. Native driver tests, MCU links, remote CI
and physical qualification were NOT RUN.
