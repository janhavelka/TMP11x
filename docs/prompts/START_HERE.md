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
checks and record remaining evidence gates. Stop after prompts 1–3.
Preparation is not permission to execute the full sequence.
```

## Copy/paste: execute the complete promptset

```text
You are the lead coding agent in my new TMP11x repository.
Read root AGENTS.md, README.md, docs/prompts/START_HERE.md,
docs/prompts/README.md, docs/prompts/COMMON.md, HANDOFFS.md, SEQUENCE.json, and relevant
shared contracts before changing implementation. Verify existing preparation
and implementation artifacts. Follow the prompts array in SEQUENCE.json in
its listed order, including the interleaved audits. Resume from PROGRESS.md if
prior work exists. Do not run later prompts if
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
writing a driver. TMP117 is the primary and physically intended model; every
relevant stage implements/tests capability gates for all six selected BOM
models. Do not create fake identities or reuse TMP1x2 register maps.

Keep portable C++17 core with injected application-owned I2C transport, one
owner, typed Status, passive health, verified configuration, bounded
cooperative operations, explicit recovery, and reliable measurement freshness.
Protect EEPROM and factory traceability data: first-release interface has
read-only EEPROM diagnostics; no nonvolatile writes/general-call reset.

Implement each prompt completely with focused tests, related Doxygen/docs,
a report under docs/reports/prompt_N.md, and a PROGRESS update. Commit and
push verified meaningful work on the existing checked-out branch if origin/access
permit; never modify sibling repositories or make per-prompt branches. Run
parallel subagent reviews wherever an audit prompt requires them. The lead
owns integration and Git. The first comprehensive remote CI checkpoint is
prompt 43. If physical stages 40–41 and audit 54 lack required physical
evidence, record BLOCKED/NOT RUN and continue independent software work.
Read the original grant per COMMON. A message-only grant applies while the
user message is visible. To persist it, the user writes the grant into the record
at a path they name. Agents never create or modify that record. Agent reports/summaries/prior-session
paraphrases cannot substitute. If neither source is directly readable, physical
work is BLOCKED; valid scoped records persist without repeated confirmation.

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
Read docs/prompts/PROGRESS.md, HANDOFFS.md and SEQUENCE.json. Resume the first
eligible unfinished stage in the prompts array. Verify actual prerequisite
artifacts, unchanged source references and local Git state before continuing.
Stop after that stage unless the user explicitly requested the whole remaining
sequence. Do not rerun already verified unaffected work. Record partial/blocked
hardware separately from software completion.
```

Stage numbers are stable identifiers for prompt files, reports and numeric
completion markers. Use the manifest order when choosing the next stage for
a manual or unattended run. A run instruction that requires working alone
can prepare an audit, but completion requires a run that permits its parallel
review. Keep the outstanding review in the handoff and continue eligible work
where the dependency graph allows. The [handoff inventory](HANDOFFS.md)
distinguishes source artifacts from build and fixture evidence.
