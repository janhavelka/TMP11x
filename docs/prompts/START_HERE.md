# Start here — VS Code / Codex / Claude coding agent

This repository contains a promptset and preparation evidence; production code
starts at stage 10. Check [PROGRESS.md](PROGRESS.md) and actual artifacts before
resuming. The intended PCB uses TMP117. The planned `TMP11x` driver also covers
TMP116/TMP116N, TMP117M/TMP117N and TMP119; TMP114/TMP118 remain excluded.

## Copy/paste: prepare or refresh this repository

```text
Read AGENTS.md, docs/prompts/COMMON.md, PROGRESS.md and prompts 1–3.
Inspect the current repository and read-only sibling implementations. Acquire
or verify official TI references and their provenance. Improve the entry
instructions, promptset, source/toolchain audits and supporting scripts using
verified sibling precedents. Reuse pinned documents and clean local sibling
checkouts; do not download or copy unrelated archives. Run the preparation
checks and record remaining evidence gates. Stop before driver implementation
or API freeze. Preparation is not permission to execute the full sequence.
```

## Copy/paste: execute the complete promptset

```text
You are the lead coding agent in my new TMP11x repository.
Read root AGENTS.md, README.md, docs/prompts/START_HERE.md,
docs/prompts/README.md, docs/prompts/COMMON.md, SEQUENCE.json, and relevant
shared contracts before changing implementation. Verify existing preparation
and implementation artifacts. Follow prompts 1–44 in ascending numeric order,
resuming from PROGRESS.md if prior work exists. Do not run later prompts if
specific required inputs are unavailable; preserve exact blocked obligations
and proceed with independent eligible work. Do not ask for routine approval.

First inspect my GitHub repositories (especially janhavelka/TMP1x2,
OPT4001, ADS1115, 24Cxx, esp32-platformio-library-template, and selected
FieldCore-node consumer code). You have permission to READ these repositories.
Reuse clean local checkouts read-only, or clone into temporary external
locations. Do not fetch into or change sibling working copies. Audit real
headers, implementations, callers, tests, build scripts,
CI and licenses. Bring only genuinely applicable, correctly attributed MIT
code and infrastructure into this new repo; update namespace, API, tests,
errors, docs and all assumptions for TMP116/TMP117/TMP119 silicon.

Verify the pinned TI reference manifest; acquire missing official documents
and record revision changes explicitly. Preserve originals/provenance,
and build an independently checked register/field/side-effect ledger before
writing a driver. Use TMP117 as the first implemented and physically intended
model. Extend the same driver with explicit TMP116 and TMP119 capability
profiles; do not create fake device identities or reuse TMP1x2 register maps.

Keep portable C++17 core with injected application-owned I2C transport, one
owner, typed Status, passive health, verified configuration, bounded
cooperative operations, explicit recovery, and reliable measurement freshness.
Protect EEPROM and factory traceability data: first-release interface has
read-only EEPROM diagnostics; no nonvolatile writes/general-call reset.

Implement each prompt completely with focused tests, related Doxygen/docs,
a report under docs/reports/prompt_N.md, and a PROGRESS update. Commit and
push verified meaningful work on the existing main branch if origin/access
permit; never modify sibling repositories or make per-prompt branches. Use
subagents for bounded parallel reviews and tests when useful; the lead owns
integration and Git. The first comprehensive remote CI checkpoint is prompt 43.
No available physical TMP11x fixture means hardware stages 40–41 remain
BLOCKED/NOT RUN, not falsely PASS; continue independent software work.

Finish by reporting actual implementation coverage by exact model, native
and MCU build results, package/CI results, physical evidence, current commit,
upstream synchronization and unresolved limitations. Never invent a pass.
```

## Copy/paste: execute one stage

```text
Execute prompt N from docs/prompts/README.md, following root AGENTS.md,
COMMON.md, and all named shared contracts. Inspect its source revisions,
prerequisites and existing progress. Deliver the complete scoped changes,
focused tests, Doxygen/docs, evidence and docs/reports/prompt_N.md; update
PROGRESS.md. Stage only your own work, commit/push when relevant checks pass,
and verify the actual upstream state. Stop after this prompt's handoff. If
blocked, record exact outstanding requirements without fake stubs.
```

## Copy/paste: resume

```text
Read docs/prompts/PROGRESS.md and SEQUENCE.json. Resume the earliest usable
incomplete stage in ascending numeric order. Verify actual prerequisite
artifacts, unchanged source references and local Git state before continuing.
Complete the requested sequence without rerunning already verified unaffected
work. Record partial/blocked hardware separately from software completion.
```

**Execution:** File names use `1_...md`, `2_...md`, ... `44_...md`, not
lexicographic order. Only a deliberate whole-set instruction authorizes
continuing through all stages in one agent task. A single-stage instruction
ends after one stage.
