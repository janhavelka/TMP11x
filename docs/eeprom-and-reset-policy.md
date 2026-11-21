# EEPROM, identity and reset safety

The release policy permits addressed observations and a typed volatile profile.
Applications select the exact BOM model, seven-bit address and bus route.
Identity checks read one word at `0x0F` on that target before any dependent
access in initialization, recovery or a multi-register diagnostic. A mismatch
ends the attempt. Preserve the raw word and any revision in the evidence.
[Variant and identity contract](variants.md#identity-decision).

## Model gates

EEPROM data indices are one-based. Each index selects one complete MSB-first
word. Reject an out-of-range index, unsupported offset or unsupported reset
before any I²C. OFFSET management starts disabled and preserves unmanaged
calibration. TMP116/N uses pointer `0x07` for EEPROM3 and permits a checked
shadow read there. An offset request on those models returns `UNSUPPORTED`
or the equivalent typed error.

| BOM model | Compatible identity | EEPROM shadow indices in order | Typed OFFSET | Addressed reset |
| --- | --- | --- | --- | --- |
| TMP116 | Full word `0x1116` | `0x05`, `0x06`, `0x07`, `0x08` | Unsupported | Unsupported |
| TMP116N | Full word `0x1116` | `0x05`, `0x06`, `0x07`, `0x08` | Unsupported | Unsupported |
| TMP117 | `(raw & 0x0FFF) == 0x0117` | `0x05`, `0x06`, `0x08` | Opt-in at `0x07` | CONFIG bit 1 |
| TMP117M | `(raw & 0x0FFF) == 0x0117` | `0x05`, `0x06`, `0x08` | Opt-in at `0x07` | CONFIG bit 1 |
| TMP117N | `(raw & 0x0FFF) == 0x0117` | `0x05`, `0x06`, `0x08` | Opt-in at `0x07` | CONFIG bit 1 |
| TMP119 | `(raw & 0x0FFF) == 0x0117` | `0x05`, `0x06`, `0x08` | Opt-in at `0x07` | CONFIG bit 1 |

On the last four rows, bits 15:12 are revision information. The compatible
DID alone cannot identify the exact part, package or accuracy grade.
[TMP116 map p.24 and DID p.31](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=24),
[TMP117 map p.25 and DID p.32](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25),
[TMP119 map p.32 and DID p.39](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32).

## Protected data and traceability

At power-up the device loads prior EEPROM contents into the register map.
Programmed startup values can differ from TI's factory values. The maps and
field tables identify the EEPROM-backed CONFIG subset (MOD0 and bits 9:2),
limits, supported OFFSET and the mapped EEPROM data words. TMP117/TMP119
startup prose omits DR/Alert and OFFSET. The corresponding map and field
tables mark both EEPROM-backed. MOD1, status, commands and reserved bits are
outside the stored CONFIG subset. The known general-call reload uses the
programmed startup data. Addressed Soft_Reset has separate evidence below.
[TMP116 pp.17,24,26–27](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 pp.18,25,27–28,31](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=18),
[TMP119 pp.20,32,34–35,38](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=20).

TI describes a unique ID programmed into general-purpose EEPROM at production
to support NIST traceability. Its tier 1B evidence requires preserving the
factory-encoded ID and the lot code from the original shipment-box label.
Reads return volatile shadows, which a prior writer could have changed.
An ordered multiword snapshot is non-atomic. Preserve every raw word,
including zero, without inferring ID layout,
authenticity, physical EEPROM contents or calibration of an installed
system. The TMP116 shadow map says zero for all four words, while its detail
tables give unknown for words 1, 2 and 4. TMP117/TMP119 repeat four-word prose
and diagrams, but map three shadows and OFFSET. Their EEPROM2 reset headings
say zero where the field tables and maps say unknown. The EEPROM1/EEPROM3
prose also reuses inconsistent word labels. Protect the entire mapped set.
[TMP116 pp.17,24,29–31](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=17),
[TMP117 pp.18,20,25,30–32](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=25),
[TMP119 pp.20,22,32,37–39](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=32),
[TI traceability tiers and models, SBAT024 pp.2–4](reference/source/NIST-Traceability-for-Temperature-and-Humidity-Sensors-Overview-SBAT024.pdf#page=2).

EUN powers up locked (`0`). With EUN clear, silicon permits volatile changes
to EEPROM-backed registers. EUN set can make a write program EEPROM, including
otherwise ordinary CONFIG and limit writes. The release forbids every data
write to `0x04` (including relock) and every data write to any EEPROM data
shadow, even with EUN clear. Approved typed operations may write only CONFIG,
THIGH, TLOW and, with opt-in on a supported model, OFFSET under the guard
below. Programming takes 7 ms typical with no specified maximum. The ratings
are 1,000 writes minimum (50,000 typical) and 10 years retention minimum
(100 typical), under the stated datasheet conditions.
[TMP116 pp.6,17,29](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=6),
[TMP117 pp.7,18,30](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=7),
[TMP119 pp.8,20,37](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=8).

## Startup and operation guard

One application authority must exclude competing access to the selected
sensor from admission through the last dependent transfer. Control power and
keep the route stable. A lock that serializes individual transfers leaves a
gap where another owner or a second controller can unlock EEPROM between the
guard and the write. Waits release the bus for unrelated devices, subject to
the application's routing and ownership guarantees.

1. Require startup evidence independently of BUSY. With no retained proof,
   initialize or recover under stable power. Timestamp completion of the first
   successful compatible read internally using the owner clock (`FIRST_ACK`).
   A read can succeed during the load. Wait at least 3 ms from that completion,
   rounding conservatively for the clock resolution. Bind time and caller
   timestamps supply no origin. The post-reset path below also uses a new
   origin. Managed fresh acquisition needs existing proof. Reuse proof while
   power and ownership remain valid. Unknown power history requires new proof
   before dependent work. The 3 ms interval is policy. TI lists about 1.5 ms for startup, with
   no maximum. TMP116 p.17 says BUSY is clear during load and all register
   writes are ignored. Its pp.26 and 29 say BUSY is set. TMP117/TMP119 state
   BUSY is set and name ignored CONFIG and limit writes. The OFFSET write
   response during load is unknown.
   [Clock, startup and busy evidence](device-behavior.md#startup-and-reset-evidence).
2. At entry to every mutating or freshness-producing operation, before its
   first dependent write or freshness observation, read one complete `0x04`
   word. Check EUN bit 15 (`0x8000`) and BUSY bit 14 (`0x4000`) together. BUSY
   mirrors CONFIG bit 12. A guard passes only when both bits are clear. CONFIG
   reads consume flags and cannot supply this guard. The guard applies to
   initialize/apply/recover, settings (including shutdown), limits, managed
   offset, one-shot, addressed reset and every managed fresh attempt. Latest
   and diagnostic observations may run with unknown startup/lock/profile
   evidence and make no freshness claim.
3. EUN set refuses immediately, even with BUSY set or when asked to shut down.
   Report `EEPROM_UNSAFE` or the equivalent. Once observed, remain fail-closed
   until an application power cycle or authorized external relock, followed by
   invalidation and renewed startup, ownership and guard evidence. An ordinary
   recovery, a later diagnostic zero word or a library reset cannot clear
   that condition. The library sends no command to stop conversions on an
   unlocked part. If the guard transfer failed, end the attempt without a
   write or fresh result. Retain the transport failure and unknown lock state.
   A separate attempt needs a new complete safe observation.
4. With EUN clear and BUSY set, a timed operation may make only paced `0x04`
   rereads within its deadline and work cap until a complete safe word arrives.
   Each transfer must succeed to continue. No time or budget means no further
   transfer. Untimed `tryReadFresh` returns busy/not-ready after that one read.
   A successful but late callback cannot authorize a later transfer or result.
5. Reuse a passing word only within this uninterrupted operation. Reset,
   rebind, interference, ambiguous transfer and any observed BUSY invalidate
   the guard. A subsequent CONFIG observation of BUSY retains any returned
   flag evidence and requires a fresh safe `0x04` before another dependent
   step. BUSY alone leaves stable profile trust unchanged.
   [Owner and guard rules](prompts/ENGINEERING.md#eeprom-safety-and-reset).

## Addressed reset and recovery

TMP116/N CONFIG bits 1:0 are reserved. TMP117/M/N and TMP119 field tables
describe CONFIG bit 1 `Soft_Reset` as a write-one command with a 2 ms reset
interval and an always-zero readback. The diagrams label the field `R-0`.
Use the detailed command definition and retain the access discrepancy.
EEPROM reload, EUN changes, application of companion bits and I²C ACK
availability during this command are unknown. General-call behavior supplies
no evidence for these effects of addressed reset.
[TMP116 p.26](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=26),
[TMP117 pp.27–28](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=27),
[TMP119 pp.34–35](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=34).

Only an explicit request on a supported model can issue the command. Require
startup, the operation-local guard and last verified stable CONFIG input.
Encode that input with canonical continuous (`MOD=00`) or shutdown (`MOD=01`),
bits 15:12 and bit 0 clear, and bit 1 set. Exclude transient `MOD=11`, raw
status flags and reserved bits. Ordinary CONFIG writes keep bit 1 clear.
Immediately before the single addressed command attempt, invalidate startup,
guard, initialization, profile trust, fresh conversion/owned-trigger and live
alert evidence. Preserve desired settings and historical alerts with their
prior generation and reset/unknown provenance. An ambiguous callback retains
the invalidation and uncertain command effect. It never triggers a resend.

After a successful transfer, honor at least 2 ms from its completion, rounded
for the clock, before post-reset observation. Require compatible identity,
an observed safe `0x04` and full profile readback for successful verification.
Observe outcomes and report uncertainty or mismatch where warranted. Time,
ACK and command-bit zero cannot establish a particular reset image. The first
successful compatible postwait read supplies a new FIRST_ACK. Any later
write requires the separate startup wait and a new locked/idle guard.
Reapply only when the caller explicitly requests recovery with replay. The
reset path never assumes factory settings or a relock.
[Reset implementation and test requirements](prompts/25_per-device-reset.md).

Any ambiguous volatile data write can leave its target changed. Mark the
affected profile dirty/untrusted, record transport and operation evidence,
and stop dependent work. For explicit recovery, check identity, startup and
lock/busy, then replay and verify only the owned volatile profile (stable
CONFIG, both limits and any supported, managed offset). Preserve unmanaged
offset even when its programmed startup value differs from the prior shadow.
Do not hide reset, rollback or retries in recovery. A failed CONFIG/TEMP read
may already have consumed status. Preserve previously retained evidence and
mark the affected ready/alert observation uncertain. Diagnostic reads alone
cannot repair trust or publish freshness.
[Trust and destructive-read rules](device-behavior.md#temp-destructive-reads-and-sample-publication).

## External interference

Known power loss/cycle or external reset invalidates startup proof, guard,
initialized/configuration trust, freshness, the owned trigger and live alert
evidence. Outside writes invalidate the guard and affected profile, conversion
or status evidence. Unknown write scope requires full invalidation. A known
external ARA can acknowledge a threshold alert. Among responding devices the
lowest seven-bit address wins arbitration. TI describes that winner's ALERT
pin becoming inactive and losing responders retaining their asserted pin.
The response's low bit identifies the high or low threshold source. Effects
on DATA_READY routing remain unspecified.
Invalidate affected live alert/acknowledgement evidence and retain historical
events. If the outcome is unknown, mark the affected evidence uncertain.
Outside CONFIG/TEMP reads can consume the corresponding flags as well. The
application must report interference and reestablish ownership. A valid DID
or later matched CONFIG word cannot detect every outside action.
[TMP116 pp.21,23](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=21),
[TMP117 pp.22–23](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=22),
[TMP119 pp.24,26](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=24).

Use the bus-silent invalidation lifecycle where full invalidation is needed.
Stop or finish the active job and consume any result before invalidating,
then recover only after stable power and exclusive access are restored.
Retain desired settings, historical evidence and transport history across
invalidation. Accepted bind/rebind, end/unbind and reset clear prior startup
proof. Treat a known unlocked physical target as unsafe across repeated calls
until the owner has taken the external corrective action above.
[Owner lifecycle](prompts/ENGINEERING.md#eeprom-safety-and-reset).

## Excluded and application-owned functions

| ID | Release disposition |
| --- | --- |
| X01 | EEPROM unlock, relock, programming, data-shadow writes and persistent startup changes are excluded across API, diagnostics, CLI and every fixture, including sacrificial devices. A future maintenance campaign needs its own user request and policy. |
| X02 | Exclude general-call reset at seven-bit `0x00` with command `0x06` and synchronized conversion via programmed startup plus broadcast reset. Silicon reloads programmed power-up values and clears EUN on the documented general call. The shared scope can include other devices. |
| X03 | Exclude SMBus ARA (`0x0C`, read address octet `0x19`), including CLI/adapter routes. ARA has the threshold-alert arbitration and pin effect above. Addressed CONFIG observation in alert mode is itself an acknowledgement with read effects. |
| X04 | Exclude public raw data writes, bare current-pointer reads and multi-register bursts. Each checked read names one mapped pointer and requests exactly two bytes. Only typed guarded operations may write data words. |
| X05 | The application owns bus clock, pulls, pins, mux routing, GPIO interrupts, bus recovery and retries. Account for authorized route changes separately from sensor traffic. The core calls bounded transfers. |
| X06 | Preserve EEPROM data and shipment provenance. A shadow dump alone supplies neither an authenticated factory ID/certificate nor a calibrated system accuracy claim. |
| X07 | Reject out-of-family model selections (TMP114/N, TMP118 variants, TMP102/112/1075/110 and unmapped TMP119M). Use the authorized target and compatible identity rule before dependent I/O. No foreign protocol probes. |
| X08 | Exclude undocumented CRC/PEC, fault queue, extended mode and sample counter. Process DATA_READY as one eligible indication per observation. |

[Coverage inventory](prompts/FULL_COVERAGE.md#excluded-or-application-owned-hardware-functions),
[TMP116 general call and sync example pp.21,35](reference/source/TMP116-Digital-Temperature-Sensor-Datasheet-SBOS740A.pdf#page=35),
[TMP117 pp.22,34](reference/source/TMP117-Digital-Temperature-Sensor-Datasheet-SNOSD82D.pdf#page=34),
[TMP119 pp.24,29](reference/source/TMP119-Digital-Temperature-Sensor-Datasheet-SNIS236.pdf#page=29).

## Transcript vectors for implementation

`R(addr, selector, 2)` means one addressed combined pointer/read transfer.
`W(addr, bytes)` means one addressed write transfer. `addr` is the bound
seven-bit address. The examples use `0x48` on the application-selected route.
Pointer traffic for `R(0x48, 0x00, 2)` selects TEMP and is valid. All vectors
apply to all six models where the capability exists.

| Case | Inputs and allowed transcript | Forbidden transcript or outcome |
| --- | --- | --- |
| Identity and shadow indices | After compatible identity where required, use one explicit `R` for each requested index, with selectors in model order. On a `0x1117` read, accept the last four BOM selections, retain revision 1, and refuse TMP116/N. On `0x1116`, accept TMP116/N only. Exercise zero, nonzero and externally changed shadow contents. | No dependent access after identity mismatch. No callback for invalid index, TMP116/N offset/reset or out-of-family selection. No read of `0x07` in a three-word EEPROM dump, no request for a fourth word, no bare read or burst. |
| Startup | With proof absent and first compatible read completing at 50.9 ms on a 1 ms floor clock, tick 54 is the earliest safe tick for the 3 ms wait. After the wait require `R(0x48, 0x04, 2) => 0x0000` before dependent I/O. | No write or fresh result before the wait and guard, even with an early zero BUSY on TMP116/N. An old bind timestamp or CONFIG BUSY clear cannot authorize work. |
| Busy or guard failure | With valid startup, `0x4000` permits only a paced `R(0x48, 0x04, 2)` on a timed path. After a timely `0x0000`, proceed within the same operation. Untimed fresh stops after `0x4000`. On partial read, timeout, expired budget or late success, stop using the attempt's guard. | No CONFIG condition polling as substitute, retry of a failed transfer, unsafe write or fresh result. If subsequent CONFIG shows BUSY, no dependent step before a new safe word. |
| Unlocked | `R(0x48, 0x04, 2) => 0x8000` or `0xC000` terminates the operation with `EEPROM_UNSAFE`. Also test a later diagnostic zero and an ordinary recover before the owner's correction. | No shutdown, command reset, attempted relock or other data write. No fresh publication and no busy polling after EUN set. |
| Protected memory and raw escape | The checked transport refuses shadow writes. For a known supported OFFSET, permit `W(0x48, [0x07, MSB, LSB])` only for an explicitly managed offset change or owned-profile replay under the guard. | No `W(0x48, [0x04, MSB, LSB])`, including `[0x04, 0x00, 0x00]`. No data write to `0x05`, `0x06`, `0x08` on any model, or `0x07` on TMP116/N. Reject equivalent attempts through diagnostic, job and CLI dispatch. |
| Supported reset | After safe preflight, with last verified stable code `0x0220` (continuous) or `0x0620` (shutdown), one `W(0x48, [0x01, 0x02, 0x22])` or `W(0x48, [0x01, 0x06, 0x22])`, respectively. Invalidate before the attempt. On success at 50.9 ms, a 1 ms floor clock allows postwait observation from tick 53. Vary changed/unchanged profile and EUN/BUSY/ACK outcomes independently. | No command on TMP116/N, without verified stable input or before startup/guard. No replay of flags or transient one-shot MOD, no resend after ambiguous completion, no postwait write on old proof or success inferred from bit 1 reading zero. |
| Recovery | Inject ambiguous CONFIG, second limit or managed offset writes. An explicit recovery establishes evidence again and verifies replay of nonfactory limits such as THIGH `0x1280` (+37 °C) and TLOW `0xFB00` (−10 °C). On supported parts, exercise OFFSET `0x0040` (+0.5 °C) as an owned setting and as an observed unmanaged value. Inject failed CONFIG/TEMP reads and possible flag loss. | No hidden reset, rollback or blind replay after partial failure. No unmanaged calibration write or fresh result from a diagnostic. |
| Interference after the guard | Start with locked/idle evidence, then report an external power cycle, general call or write before the next dependent transfer. Separately report external ARA with the target winning, losing or unknown. | No write or fresh result using the invalidated guard. No use of old profile or conversion proof after reset. ARA cannot supply a fresh sample or erase retained historical alerts. |
| Shared bus exclusions | Across all typed, diagnostic, recovery, job and CLI paths, including bind-only boot, check callback counts and addresses. Each operation that requires a guard must obtain its own passing word, including consecutive operations on one owner. | Zero sensor callbacks at `0x00` (including `W(0x00, [0x06])`), zero ARA callbacks at `0x0C`, zero broadcast sync and no unapproved route traffic. No sensor writes from bind, job admission/cancel/result consumption or teardown. |

These are research inputs. Stages 18–23 own guard, trust and acquisition
traces. Stages 24–26 own EEPROM, reset and diagnostic tests. Stages 29–33 own
cooperative and adversarial traces and stage 38 owns complete CLI dispatch.
Stages 40–41 retain separate fixture evidence. The
[register ledger](registers/tmp11x.json) and [feature coverage](feature-coverage.md)
record current evidence and consumer ownership.
