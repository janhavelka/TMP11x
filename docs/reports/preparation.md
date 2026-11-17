# Repository preparation — 2026-11-15

Historical preparation checkpoint. The later [full promptset audit](promptset-audit.md)
corrected stage 3's scope/status: optional secondary TI code is not an intake
gate. The original access observations and tests below remain valid.

The requested supporting archive and repository preparation are complete.
Driver implementation has not started. Stages 1 and 2 are complete; stage 3
retains specific unavailable-source evidence. This request did not invoke
stages 4–44, freeze the API, or authorize hardware activity.

## Input and ownership

Input HEAD: `6c5e4496d5c896af689228d4bee4382a53c20626` on `main`, tracking
`origin/main` at `https://github.com/janhavelka/TMP11x.git`. Initial tree was
clean; `git fetch origin` succeeded with no incoming changes. The repository
contained planning Markdown/JSON and two checker scripts, with no C++ sources,
package/build files, workflows or downloaded references.

One lead integrated shared instructions/checkers/reports and owns Git. Three
bounded subagents audited sibling APIs, tooling/prompt workflows and TI sources;
independent follow-up reviews challenged checker fixtures and reference restore
behavior. Siblings were read-only throughout. Their exact revisions and dirty
states are recorded in the audits, including the differing FieldCore local and
remote revisions.

## Delivered work

- [Library comparison](../library-comparison.md) maps declarations to actual
  implementations, registered tests/callers and TMP11x adoption decisions.
  TMP1x2 is the main owner/health precedent; OPT4001's lifecycle differs.
- [Toolchain/package plan](../toolchain-and-package-plan.md) records build,
  packaging, CLI, version, promptset and installed-tool evidence. The available
  SDKs are not claimed as successful TMP11x firmware builds.
- [Reference archive](../reference/README.md): 14 original TI PDFs totaling
  14,498,321 bytes, hashed manifest, 12 visually reviewed datasheet pages,
  concise page-linked notes and a commit/license-pinned public-code catalog.
- [Adoption record](../source-adoption.md): unchanged MIT `scripts/pio.cmd`
  and LICENSE from TMP1x2, adapted `.gitignore`, byte-preserving attributes.
  No sensor implementation or sibling reference folder was copied.
- AGENTS owns project instructions; CLAUDE imports it. Root START_HERE forwards
  to the canonical guide. Preparation, one-stage and whole-set requests are
  distinct, and progress resumes from real artifacts.
- The checker now checks title/phase/type, prerequisite links and README index
  against SEQUENCE. Regression fixtures omit Git and PDF payloads while retaining
  links to future sources. Reference tooling detects corrupt/changed downloads
  and preserves existing or concurrently created files.

## Verification

Commands below were run on the integrated preparation tree with Python 3.12.10.
The final resulting commit and remote synchronization are reported in the task
handoff; this report records its immutable input SHA above.

| Command/evidence | Result |
|---|---|
| `python scripts/check_promptset.py` | PASS: 44-stage structure, dependencies, prose/index parity and local links |
| `python scripts/test_check_promptset.py` | PASS: 14 regression cases |
| `python scripts/check_references.py` | PASS: 14 PDF paths/signatures/sizes/SHA-256 |
| `python scripts/test_check_references.py` | PASS: 9 offline corruption, metadata, path and no-overwrite cases |
| `python scripts/fetch_references.py tmp117` | PASS: verifies existing bytes without download |
| Reference agent live restoration probe | PASS: missing TMP117 restored in a separate temporary probe; repeated call preserved it |
| `scripts\pio.cmd --version` | PASS: existing managed PlatformIO Core 6.1.19 |
| `git diff --check` and staged diff check | PASS |
| Native sensor tests, MCU full links, package consumers | NOT RUN: implementation/build definitions do not exist |
| Remote CI | NOT RUN: no workflow; comprehensive checkpoint belongs to stage 43 |
| Physical HIL | NOT RUN: fixture not established; stages 40–41 not invoked |

## Remaining evidence and next scope

Superseded stage-3 verdict from the original preparation: stage 3 was labelled
PARTIAL because TI generated/firmware source revision and license
could not be inspected: ASC Studio timed out and TMP116 software requires
export approval. These are recorded access limits, not evidence of a software
failure. No installer was run or approval bypassed.

The TI E2E DATA_READY race thread returned HTTP 403. Stage 6 owns resolving
that research lead. Stage 4 must verify TMP119M orderable/grade scope; stage 5
must retain TMP119's conflicting device-ID defaults. No source ambiguity was
resolved by guessing or assigning TMP119 identity from a revision nibble.

On the next explicit implementation request, verify the source manifest and
relevant audit revisions. Revisit optional official-code evidence when
accessible; independent stage 4 work can use the acquired exact-part
PDFs once its concrete source inputs are confirmed. Stages 4–9 still own the
checked ledger, timing/safety and API contracts before stage 10 begins code.

## Revalidation response — 2026-11-17

Input: `171dbf58251e8606d3b643aa1ceddcc7e921c906`. Reviewed each delivered-work
claim and remaining gate above against the repository. The source/adoption maps,
14 pinned originals, shared agent entry, scoped checker fixtures and reference
integrity/no-overwrite protections remain present. Their tests establish only the
reported tooling/evidence layer; there is still no sensor implementation.

Three historical statements require qualification:

1. Stage 3's PARTIAL verdict is superseded by `prompt_3.md`; optional gated TI
   software never prevents required datasheet intake from being COMPLETE.
2. The wrapper's 6.1.19 output was real, but its description as the active managed
   Core was wrong. TOOL-1 selects the configured `C:\pio` penv (6.2.0) now and
   corrects the active package inventory. Initial unchanged adoption is historical;
   current modifications are recorded in `docs/source-adoption.md`.
3. FieldCore's differing hashes are `rodtester` and `main` branch evidence, not a
   demonstrated stale checkout. Neither branch grants TMP11x bench authorization.

The current source-integrity checks and review responses are reported with this
revalidation's handoff. Original suite counts remain dated facts. Implementation,
MCU links, remote CI and physical HIL remain NOT RUN; stage status is unchanged.
