# Prompt 2 ? tooling and promptset audit

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
