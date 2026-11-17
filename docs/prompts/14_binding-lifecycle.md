# 14 — Implement bus-silent binding and lifecycle state

Apply [COMMON.md](COMMON.md) and root [AGENTS.md](../../AGENTS.md). Phase: **Portable device protocol, identity and trustworthy configuration**. Type: **software**.
Prerequisites: [8](8_api-architecture.md), [9](9_owner-deadline-contract.md), [11](11_status-values.md), [12](12_i2c-register-transport.md), [13](13_identity-selection.md).
Produces: `binding-api`, `public-identity-api`.
Consumes: `api-contract`, `owner-contract`, `value-types`, `register-transport`, `identity-engine`, `register-fake`.

## Objective and boundaries

Deliver real instance-local binding/teardown state without controlling the application's I2C bus. Verified hardware initialization is implemented in prompt 18 after the temperature and CONFIG codecs exist; do not duplicate it here or substitute a successful placeholder.

## Source audit

Trace TMP1x2 `bind`, `unbind`, `begin`, `end` implementation and tests; compare OPT4001 framework-neutral bind/begin. Inspect EEPROM timing evidence established in prompt 7.

## Complete implementation work

- Implement bus-silent `bind(Config)`, `unbind()` and `end()`. Validate binding structure (model/address/callbacks, timeout/clock contract and contexts) here; stage 18 extends bind to validate the complete accepted sensor profile using stages 16/17 codecs before replacing any state. Document active-job/result policy now; stage 28 supplies actual slots and executable guards/tests rather than adding unused job scaffolding here.
- Attach stage 13's real identity engine as public bound `readIdentity`/`probe`, including unbound refusal and bound-but-uninitialized success. These reads do not initialize the managed profile and remain health-neutral; stage 15 asserts that policy.
- Establish the state and guard interfaces required by prompt 18's real `begin(Config)`/`begin()`: bound versus initialized, selected profile, identity observations, startup state and verified configuration. No public initializer reports success before that implementation exists; identity-only probe remains a distinct prompt 13 operation.
- Record initialized state separately from transport health, fresh-conversion evidence and dirty config state. `end` never sends physical shutdown implicitly; offer explicit shutdown later.
- Define rebind semantics for model/address change or externally reset device, including invalidation of available observations and the future operation-token rules. Integrate sample caches at stage 16 and active/retained operation slots at 28; maintain no heap and no task creation.
- Reject an invalid rebind without changing the previous binding or evidence. An accepted rebind replaces the desired target and discards old-target observations/dirty reasons, but starts uninitialized with unknown hardware trust; it does not prove the new hardware clean. First-write target commitment belongs to setters/configure, not accepted bind or begin(Config).

## Verification and exit criteria

Test zero-I2C bind/unbind/end, rejected/failed rebind preserving the prior state under the frozen policy, unbound-operation guards, and independent instances at the same address on distinct mux routes. Prompt 18 owns initialized-state success, verified write order, identity mismatch and startup/EEPROM gating integration tests.

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
PASS/FAIL/NOT RUN/BLOCKED. Write `docs/reports/prompt_14.md` with sources,
owned artifacts, named test cases and open evidence gates. Update
`docs/prompts/PROGRESS.md`, commit and sync verified meaningful work on the existing checked-out branch
when authenticated upstream is configured, and verify synchronization. Keep
missing physical/SDK evidence explicit; never claim tests not run.
