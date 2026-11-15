#!/usr/bin/env python3
"""Check structural integrity of the TMP11x staged implementation promptset.

This checks documentation navigation and task ordering, not the actual device
register correctness or completed implementation evidence.
"""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

REQUIRED_SHARED = (
    "START_HERE.md", "README.md", "COMMON.md", "SOURCE_MAP.md", "SILICON.md",
    "EVIDENCE.md", "CONVENTIONS.md", "ENGINEERING.md", "FULL_COVERAGE.md",
    "COVERAGE.md", "VERIFICATION.md", "CLI_CONTRACT.md", "DELEGATION.md",
    "PROGRESS.md", "SEQUENCE.json",
)
LOCAL_LINK = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")


def validate(root: Path) -> list[str]:
    errors: list[str] = []
    docs = root / "docs" / "prompts"
    for name in ["AGENTS.md", "CLAUDE.md", "README.md", "docs/README.md",
                 "docs/reference/README.md", "scripts/check_promptset.py"]:
        if not (root / name).is_file():
            errors.append(f"missing required root file: {name}")
    for name in REQUIRED_SHARED:
        if not (docs / name).is_file():
            errors.append(f"missing shared contract: {name}")
    path = docs / "SEQUENCE.json"
    if not path.is_file():
        return errors
    try:
        seq = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return errors + [f"invalid SEQUENCE.json: {exc}"]
    ps = seq.get("prompts")
    if not isinstance(ps, list):
        return errors + ["SEQUENCE.json prompts must be an array"]
    if len(ps) != 44:
        errors.append(f"expected 44 prompts, found {len(ps)}")
    ids = [x.get("id") for x in ps if isinstance(x, dict)]
    if ids != list(range(1, len(ps) + 1)):
        errors.append("IDs must be contiguous ascending positive integers")
    allnames: set[str] = set()
    for item in ps:
        if not isinstance(item, dict):
            errors.append("non-object manifest item")
            continue
        n = item.get("id")
        name = item.get("file")
        deps = item.get("depends_on")
        kind = item.get("kind")
        if not isinstance(n, int) or not isinstance(name, str) or not name.startswith(f"{n}_") or not name.endswith(".md"):
            errors.append(f"invalid prompt id or filename: {n!r} {name!r}")
            continue
        if name in allnames:
            errors.append(f"duplicate filename: {name}")
        allnames.add(name)
        file = docs / name
        if not file.is_file():
            errors.append(f"missing stage file: {name}")
            continue
        t = file.read_text(encoding="utf-8")
        for heading in ("Objective and boundaries", "Source audit", "Complete implementation work",
                        "Verification and exit criteria", "Documentation and delegation", "Checkpoint and report"):
            if f"## {heading}" not in t:
                errors.append(f"prompt {n}: missing heading {heading}")
        if f"docs/reports/prompt_{n}.md" not in t:
            errors.append(f"prompt {n}: missing stage report path")
        if kind not in {"software", "physical", "ci"}:
            errors.append(f"prompt {n}: unrecognized kind: {kind!r}")
        if not isinstance(deps, list) or any(not isinstance(d, int) or d < 1 or d >= n for d in deps):
            errors.append(f"prompt {n}: bad dependencies: {deps!r}")
        if isinstance(deps, list) and len(deps) != len(set(map(str,deps))):
            errors.append(f"prompt {n}: duplicate dependencies")
    disk_stages = {p.name for p in docs.iterdir() if re.match(r"^[0-9]+_.*\.md$", p.name)}
    if disk_stages != allnames:
        errors.append(f"stage file inventory differs: extra={sorted(disk_stages-allnames)} missing={sorted(allnames-disk_stages)}")
    # Only validate explicit Markdown link targets. Inline-code paths describe planned
    # source files and are not expected to exist before the corresponding stages.
    for p in list(root.glob("*.md")) + list((root / "docs").rglob("*.md")):
        raw = p.read_text(encoding="utf-8")
        no_fences = re.sub(r"```.*?```", "", raw, flags=re.S)
        for href in LOCAL_LINK.findall(no_fences):
            href = href.strip().split("#", 1)[0]
            if not href or href.startswith(("http://", "https://", "mailto:", "skills://")):
                continue
            if not (p.parent / href).exists():
                errors.append(f"broken link: {p.relative_to(root)} -> {href}")
    return errors


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    args = ap.parse_args()
    err = validate(args.root.resolve())
    for e in err:
        print("ERROR:", e)
    if err:
        print(f"FAIL: {len(err)} promptset issue(s)")
        return 1
    print("PASS: 44 stages, dependency DAG, shared contracts, stage sections and links")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
