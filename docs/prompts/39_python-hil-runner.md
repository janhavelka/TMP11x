# 39 — Implement correlated Python HIL runner and finite campaigns

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Firmware, CLI and runner integrations**. Type: **software**.
Prerequisites: [37](37_cli-baseline.md), [38](38_cli-device-operations.md).
Produces: `hil-runner`.
Consumes: `cli-complete`, `cli-protocol`, `firmware-cli-handoff`, `host-test-entrypoint`.

## Objective and boundaries

Provide unattended but safe diagnostic verification tooling that records reproducible real-serial evidence, never invents fixture results and never performs EEPROM programming.

## Source audit

Study TMP1x2 `tools/hil_tmp1x2_runner.py` and parser/unit tests, OPT4001 and LDC1614 runners, DP8382x runner/event contracts and this promptset `CLI_CONTRACT` and `VERIFICATION`. Reuse parsing/record ideas selectively, not board pin assumptions.

## Complete implementation work

- Build `tools/hil_tmp11x_runner.py` with explicit serial port, baud, expected firmware version/identity/model/address, fixture description for physical claims, command timeout and output directory. Physical mode requires directly reading the COMMON-authorized source: a user instruction visible in current context or a user-named-path record written by the user or approved in a visible user message. Verify board/port/part/address, bus peers and campaign classes from the source itself; an agent report/summary/prior-session paraphrase is never a grant. If neither source is directly readable, physical work is BLOCKED. Valid records persist across sessions within scope without repeated confirmation; upload needs its own scope. Never infer the target from port enumeration or another project's record. Report serial reset/control-line behavior; port opening or MCU reboot must not implicitly initialize/configure the sensor in an observation campaign.
- Correlate requests and completions by nonce/sequence; detect split/late/duplicated/missing records, reboot/line noise, incomplete transactions and failure while waiting. Keep finite buffer/record/time bounds and return meaningful nonzero exits.
- Implement observation-only identity, configuration, temperature, device-capability, flags and EEPROM campaigns by default. Permit pointer selection but no register-data writes; identify CONFIG/TEMP flag consumption explicitly. Prove the same policy across firmware startup, CLI setup and runner commands. More intrusive volatile config/restoration trials must be opt-in and require explicit fixture authorization.
- Record exact raw transcript, structured JSON summary, input environment/firmware git identity, document revision, suite counts, PASS/FAIL/NOT RUN/BLOCKED and restoration outcome; no synthetic serial printout may be marked physical PASS. Persist the baseline before the first trial write, distinguish primary and restoration outcomes, and report a reboot or transport loss as loss of session continuity. Never blindly replay a saved baseline to an unconfirmed replacement device.
- Provide a dry-run plan that opens no port, unit tests for runner parser/record integrity, and a compiled fake CLI integration harness for full protocol exercise.
- Import optional `pyserial` only in physical mode. Dry-run, transcript parsing and unit/compiled-fixture tests use the standard library and require no serial hardware. Missing pyserial produces a clear physical-mode dependency error, not an import failure for host tests.
- Store raw logs outside release artifacts (the ignored `hil_logs/` is suitable); commit per-run metadata, transcript/summary SHA-256 and a durable raw-artifact location under `docs/hil/<date>-<fixture>/`. Never claim evidence from a hash whose source cannot be retrieved by a reviewer.
- Validate the selected model/address/port against the original directly read grant before opening a port, not a report's paraphrase; firmware upload needs its separate scope. Before a mutating campaign, establish matching firmware/boot/route/address and compatible identity under the BOM selection; repeat after reset, reconnect, rebind or loss of continuity. Check that target/route continuity remains valid before each write; a bare ID match never proves exact grade or authorizes another fixture. Default observation campaigns omit `discover` and allow only declared addresses. Compare passive `xfer` counters from boot to campaign end and require zero sensor data-write attempts, stable session and no counter loss for a software-instrumented no-write claim; external bus capture is a separate stronger layer.
- Consume `docs/cli-protocol.md`, golden transcripts and the actual compiled stage 37/38 fixture. Verify protocol compatibility and boot/session changes independently of library version; reject unknown/mismatched firmware identity for a physical qualification claim. Add runner unit and subprocess CLI integration tests to the standard host entrypoint and `docs/testing.md`; stage 33's earlier PASS does not cover these new suites.
- The runner may be fully host-tested without an MCU SDK. Physical execution separately selects a current image from `docs/firmware-cli-handoff.md` and the fixture gate; do not require both firmware backends or use an older example image simply because it once linked.

- Make campaigns identify the exact model/package/firmware and applicable R/F evidence rows. Record bus speed, supply, ADD0 and ALERT wiring, actual AVG/cycle/offset/limits and restoration scope. Recipes for new latch/re-arm, averaging and saturation/coherence questions must separate observed results from unresolved silicon claims and never silently enable excluded operations.

## Verification and exit criteria

Test interrupted sessions, nonce mismatch, malformed values, missing verification, restore failure, false-positive PASS string, bounded soak termination and no fixture behavior. Mark actual hardware NOT RUN when absent.

## Documentation and delegation

Follow [COMMON.md](COMMON.md), [CONVENTIONS.md](CONVENTIONS.md),
[ENGINEERING.md](ENGINEERING.md), [EVIDENCE.md](EVIDENCE.md),
[VERIFICATION.md](VERIFICATION.md), [DELEGATION.md](DELEGATION.md) and
[SOURCE_MAP.md](SOURCE_MAP.md) for the relevant code and sources. The lead owns
shared API/architecture, integration, progress and Git; delegate only disjoint
read-only audits, feature tests or implementation under frozen interfaces.
Update public Doxygen and affected guides in the same stage as source changes.
Do not run sibling suites. Follow COMMON for remote-CI dispatch/poll/wait policy; preserve automatic triggers.

## Checkpoint and report

For each closed software unit, run appropriate focused local checks; inspect
both working and staged diffs; record actual commands, versions, commit and
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_39.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
