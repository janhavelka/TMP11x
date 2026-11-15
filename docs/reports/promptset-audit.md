# Full promptset audit - 2026-11-15

**Audit status: COMPLETE.** Reviewed every numbered prompt 1-44 and the shared
contracts. Prompts 1-2 needed no correction; prompts 3-44 were corrected. This
is a review of implementation instructions, not execution of those stages or
qualification of a sensor driver.

Input: clean `main` at `312a42e5b15fc91f76efd69441307c1187ae6de2`, synchronized
with `origin/main` at `https://github.com/janhavelka/TMP11x.git`. Three reviewers
owned disjoint prompt ranges (1-17, 18-33, 34-44). The lead integrated shared
contracts, dependency metadata and reports. Cross-reviews reconciled the
command/settings distinction and bound-only diagnostic route. No sibling was
modified, built or subjected to CI.

## Main corrections and evidence

| Finding | Correction | Primary evidence / reference design |
|---|---|---|
| TMP116 busy-clear incorrectly usable as startup proof | Preserve the contradictory text; require an explicit startup timing policy, owner assumptions and guarded readback. Approximate POR duration is not a guaranteed maximum. | [TMP116](../reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17) SBOS740A p.17 section 7.5.1.1 versus p.26 Table 6 / p.29 Table 10; lead and reviewers visually checked originals. |
| Legal CONFIG mode treated as reserved; command bits could be preserved/replayed | Decode MOD=10 as continuous with readback 00. Stable desired mode is CC/SD. One-shot and reset use dedicated operations; ordinary writes never assert reset. | TMP116 p.26; [TMP117](../reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27) SNOSD82D pp.27-28 Table 7-6; [TMP119](../reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34) SNIS236 pp.34-35 Table 8-5. |
| Nominal cadence conflated with conversion duration/deadline | Separate minimum, typical and maximum conditions; AVG affects active time and cycle, while CONV does not set one-shot duration. Tie freshness to owned hardware evidence. | TMP116 p.5 and pp.12-14; TMP117 p.6 and pp.13-15; TMP119 p.7 and pp.14-17. |
| TEMP, CONFIG and therm flag effects conflated | CONFIG acknowledges alert-mode HIGH/LOW and DATA_READY; TEMP clears DATA_READY. Therm HIGH is threshold/hysteresis controlled and LOW is zero. | TMP116 pp.15-16/26; TMP117 pp.16-17/27; TMP119 pp.18-19/34. |
| EEPROM reads could imply immutable factory/NVM data | Treat them as volatile-shadow observations; deny writes even when EUN=0. TMP117/119 use 0x05,0x06,0x08, with offset at 0x07. | TMP116 pp.29-31; TMP117 pp.25/30-32; TMP119 pp.32/37-39. Four-word prose in TMP117 p.18/TMP119 p.20 remains a source conflict. |
| Reset and offset edge cases underspecified | Guard reset before its write, invalidate on ambiguous attempt, honor the documented 2 ms reset interval, reject TMP116 reset without I2C. Distinguish offset-input bounds from result saturation. | TMP117 pp.28/31 and TMP119 pp.35/38; TMP116 p.26 reserved bits. |
| Entry-only poll timestamp claimed to detect late transfers | Require an authoritative clock sampled after callbacks for deadline-bearing operations; separate physical success from operation timeout. | TMP1x2 operation implementation/test precedent, with the stronger TMP11x deadline contract made explicit. |
| Core asked to detect short transfers through Status-only callbacks | Adapter success must mean complete transfer; adapter validates counts and maps short transfers to terminal error. Null opaque context can be valid. | TMP1x2 Config/transport implementations and ADS1115 adapter patterns. |
| Read-only HIL could initialize/write at boot | Freeze bound-only diagnostics and a freshly bound zero-data-write transcript. Observation firmware never silently calls begin/recover/reset/one-shot, including after serial-triggered reboot. | Bus-silent TMP1x2 lifecycle precedent; TMP11x register read effects above. |
| CLI grammar and restoration rules conflicted | One canonical grammar; persistent setters stay applied. Temporary campaigns restore stable writable fields and canonical modes, not flags, commands or conversion history. | [CLI contract](../prompts/CLI_CONTRACT.md); CONFIG tables above; sibling bounded CLI design. |
| Adapter, P4 and soak evidence overstated | Require synchronous IDF use and finite timeout conversion; distinguish board absence from link capability. No active-bus reconnect or invented physical-conversion loss counters. | TMP117 p.20; [official IDF I2C documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html); local IDF v6.0.1 header contract. |
| Stage order and optional sources blocked valid work | Defer verified initialization to 18; add codec/read dependencies; make host CLI independent of SDKs; allow one matching firmware for HIL. Optional TI code access no longer gates required datasheet intake. | Updated SEQUENCE, numbered prerequisite text and index; actual source paths/tooling audits. |

Policy choices are labelled as policy. An unsupported raw-write path, timing
margin or threshold-ordering restriction is not presented as a silicon limit.
The scalar codec range is -256 through +255.9921875 degrees C, distinct from
rated measurement/accuracy limits. `0x8000` is not globally invalid after a
confirmed conversion (including a saturated result with offset).

## Individual prompt disposition

All rows refer to the current corrected prompt. Common boilerplate and links
were checked along with each prompt's objective, sources, work and exit gates.

| # | Verdict | Reviewed or corrected scope |
|---|---|---|
| 1 | No change | Source/API/test mapping, licenses, actual remote and read-only sibling boundary. |
| 2 | No change | Selective tooling reuse, installed-tool discovery and no premature firmware claims. |
| 3 | Fixed | Required original PDFs versus optional secondary code; precise archive closure. |
| 4 | Fixed | TMP116 full 16-bit ID versus 117-family revision nibble; no grade/part proof. |
| 5 | Fixed | Per-model 0x07 map, legal mode alias, JSON duplicate detection and source conflicts. |
| 6 | Fixed | Model-specific timing, AVG/CONV distinction, startup assumptions and one-shot baseline. |
| 7 | Fixed | Shadow-register limits, exclusive sensor authority and no destructive maintenance escape. |
| 8 | Fixed | Stable desired modes, distinct acquisitions, bound-only diagnostics and callback contract. |
| 9 | Fixed | Post-callback clock, portable wrap math, token retention and mux budget distinction. |
| 10 | Fixed | Real version/type consumer; no sensor placeholder; 0.1.0 is not a prerelease suffix. |
| 11 | Fixed | Separate sample and identity types; conversion-cycle semantics. |
| 12 | Fixed | Pointer/word format, correct test precedent, null context and short-transfer responsibility. |
| 13 | Fixed | No invented TMP116 revision nibble or unique-part guarantee from ID. |
| 14 | Fixed | Bus-silent lifecycle only; verified begin waits for stage 18 prerequisites. |
| 15 | Fixed | Passive OFFLINE allows normal authorized I/O; diagnostics stay health-neutral. |
| 16 | Fixed | Portable signed decode, exact arithmetic endpoints and binding/health prerequisites. |
| 17 | Fixed | Writable masks, reset/one-shot command separation and MOD normalization. |
| 18 | Fixed | Local validation, startup/EUN/busy proof, ignored POR writes and limit-codec dependency. |
| 19 | Fixed | External-writer limits, destructive-read uncertainty and stable-field comparison. |
| 20 | Fixed | Nominal timing versus measured rate, coupled averaging and profile invalidation. |
| 21 | Fixed | Stale-ready baseline, evidence consumption, bounded clock/work and no retrigger. |
| 22 | Fixed | Mode-specific alert effects, equality, polarity and one-shot ALERT prose ambiguity. |
| 23 | Fixed | Offset input bounds versus sensor saturation; no stale sample under new offset. |
| 24 | Fixed | Shadow observations, noncontiguous word pointers and no immutable serial claim. |
| 25 | Fixed | Pre-reset guards, ambiguous reset effect, 2 ms wait and no bit-clear completion proof. |
| 26 | Fixed | Bound-only observation transcript, raw command escape prevention and typed-only offset. |
| 27 | Fixed | Observation timestamps, cache provenance and explicit calibration limitations. |
| 28 | Fixed | No accepted placeholder jobs; bounded private engine and exactly-once results. |
| 29 | Fixed | Return pending during waits; physical result versus late operation outcome. |
| 30 | Fixed | Latest/try-fresh/owned-one-shot parity and separately budgeted readiness/safety steps. |
| 31 | Fixed | Observable fault injection; short transfers reported by adapter, read uncertainty distinct. |
| 32 | Fixed | Incremental model fixtures, both POR-conflict interpretations and compiled driver scope. |
| 33 | Fixed | Actual tests-OFF source consumer; installed exports are a later independent gate. |
| 34 | Fixed | Real sibling transport paths, finite timeout conversion, partial writes and observation boot. |
| 35 | Fixed | Synchronous IDF transfers, finite signed timeouts and no bus-wide reset/ARA handles. |
| 36 | Fixed | Discovered consumer paths, no assumed PCB MCU, board versus SDK evidence. |
| 37 | Fixed | Canonical CLI, observation semantics, truncation rejection and SDK-independent host tests. |
| 38 | Fixed | One grammar, persistent setters versus temporary trials, honest watch/job effects. |
| 39 | Fixed | Read-only guarantee includes firmware/serial reboot; durable preimage and session-loss evidence. |
| 40 | Fixed | Register-write-free observation, destructive-read disclosure and one matching firmware gate. |
| 41 | Fixed | Masked restoration, unsafe-cleanup refusal, quiesced fixture handling and observable counters. |
| 42 | Fixed | Isolated consumers and pinned optional coexistence dependency without sibling fallback. |
| 43 | Fixed | Preserve workflow triggers, board-independent links and no default physical-port access. |
| 44 | Fixed | Observation startup/restoration review, P4 disposition and exact evidence limits. |

Dependency/title changes are mirrored in [SEQUENCE](../prompts/SEQUENCE.json)
and the [index](../prompts/README.md): stage 14 is bus-silent lifecycle; 16 adds
14/15; 18 adds 16; 37 requires 27/28/30/33; 40 requires 39 plus an explicit
runtime gate of one matching linked firmware from 34 **or** 35 and a fixture.

## Reference designs and revision limits

Reviewed actual source/test contracts, not only README claims. TMP1x2
`afd50889312f6e218a6a34eca687e3e8547d8b40`, OPT4001
`441ef579d7fa3f7bca7665f52632fdaddd306300`, ADS1115
`e07622a99fe9f13c5d6a5a2497a8112a14185b9d`, and 24Cxx
`1bd9f969d1df55ca5819b0d290ff331869e7d5bc` matched their observed remote HEADs.
TMP1x2 transport callbacks are in the Arduino/IDF example `main.cpp` files;
there is no assumed `examples/common/I2cTransport.h` at that revision.

DP8382x was observed at `ced8b1ce5418aec044eb156b507ea78573302576` with unrelated
dirty implementation work; MotorControl-CAN at
`60bcefe5f2bd29429b8019361374e1776a2b5972` with unrelated dirty bench work.
Their source/progress patterns do not grant TMP11x hardware authorization.
FieldCore-node local `c40ea8341edf8d665460b902cafefbe857a7f73f` differs from
remote `eccdbdcc0e9ce22beee4f54494c6c179a320d3aa`; local consumer evidence is
labelled as such. None is a required runtime dependency of TMP11x.

## Validation and remaining limits

Python 3.12.10 on the reviewed working tree:

- `python scripts/check_promptset.py`: PASS, all 44 stages, dependency order,
  prose/index parity and explicit local links.
- `python scripts/test_check_promptset.py`: PASS, 14 regression cases.
- `python scripts/check_references.py`: PASS, all 14 original PDFs unchanged.
- `python scripts/test_check_references.py`: PASS, nine integrity/restore cases.
- `git diff --check` and staged whitespace check: PASS.
- C++ implementation tests, MCU links, remote CI and physical HIL: NOT RUN;
  this task audits instructions and introduces no sensor implementation.

TMP116 startup-busy wording, TMP119 ID defaults, TMP117/TMP119 EEPROM-count
prose and one-shot ALERT wording remain documented vendor inconsistencies.
The TI E2E DATA_READY race thread was retried and still returned HTTP 403;
its title does not establish a verified defect or workaround. Stage 6 owns
further applicable evidence. TMP119M scope remains a stage 4 orderable-grade
question. These are bounded source obligations, not fabricated resolutions.

Stage 3 is now COMPLETE under its corrected intake scope: required PDFs,
hashes, visual notes and honest source catalog already exist. The earlier
PARTIAL verdict incorrectly treated optional TI code access as mandatory.
Stages 4-44 remain NOT STARTED as implementation stages. The final handoff
records the resulting commit and upstream synchronization; the input SHA at
the top is not claimed as the commit containing these changes.
