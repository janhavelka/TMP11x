# Prompt 2 — tooling and promptset audit

**Status: COMPLETE.** Prerequisite: [prompt 1](prompt_1.md).
Input: `6c5e4496d5c896af689228d4bee4382a53c20626`; reviewed 2026-11-15.

[Toolchain/package plan](../toolchain-and-package-plan.md) pins TMP1x2,
OPT4001, 24Cxx, MotorControl-CAN, DP8382x and the template. It records clean
versus concurrent dirty sibling trees, source licenses, selected build rules,
CLI/HIL/package/version decisions and exact installed tool/SDK observations.
The template has no root CMake; TMP1x2 lacks the required installed CMake export;
its version generator has a device-specific regex. These are explicit stage 10
adaptations, not copied assumptions.

The managed PlatformIO wrapper was adopted unchanged and verified at 6.1.19;
no tool installation occurred. Python 3.12.10, CMake 4.0.1 and GCC 15.1.0 are
available. Native IDF SDKs exist but their environment was not active. No
firmware build was attempted without TMP11x sources.

All 44 stage prerequisites and index rows agreed before changes. Preparation
added parity/schema checks, lightweight fixtures, a single agent-instruction
source and canonical execution guide. Integrated structural checker and 14
regression cases: PASS; diff checks: PASS. See [preparation report](preparation.md)
for full commands and current evidence limits. Final commit/sync identity is
reported in the task handoff.

## Revalidation response — 2026-11-17

Input: `171dbf58251e8606d3b643aa1ceddcc7e921c906`. TOOL-1 is accepted: the
unchanged wrapper did print 6.1.19, but it selected the inactive default penv.
The process/user `PLATFORMIO_CORE_DIR=C:\pio` selects Core 6.2.0. The corrected
wrapper honors that directory, falls back to the default only when unset, and
fails rather than selecting another executable when the chosen penv is missing.
The [toolchain plan](../toolchain-and-package-plan.md) corrects active Arduino/
compiler metadata and documents the distinction. No installation/build occurred.

The seven tooling/prompt-workflow recommendations in that plan were individually
rechecked and retained: single instruction source, bounded execution scope, safe
Git provenance, structural parity/schema validation, narrow fixtures, separated
evidence layers and targeted source refresh. CON-9 adds coverage-ID/owner and PDF
page-anchor checks; they still cannot prove semantic correctness or implementation.

Consumes `ecosystem-audit`; produces `tooling-plan` at
`docs/toolchain-and-package-plan.md`. Stage 10 owns real build/version/test
entrypoints and stages 34–43 own actual adapter/consumer/CI evidence. Historical
test counts and SDK observations above are not claims about current passing builds.
