#!/usr/bin/env python3
"""Check structural integrity of the TMP11x staged implementation promptset.

This checks navigation, task ordering and declared producer/consumer handoffs,
not register correctness or completed implementation evidence.
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
    "PROGRESS.md", "SEQUENCE.json", "HANDOFFS.md",
)
LOCAL_LINK = re.compile(r"(?<!!)\[[^\]]*\]\(([^)]+)\)")
PREREQUISITE_LINK = re.compile(r"\[([0-9]+)\]\(([^)]+)\)")
ARTIFACT_ID = re.compile(r"[a-z][a-z0-9-]*")


def unique_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def validate_handoffs(docs: Path, prompts: list) -> list[str]:
    """Validate planned ownership/reachability without requiring future files."""
    errors: list[str] = []
    stages = {p["id"]: p for p in prompts if isinstance(p, dict)
              and type(p.get("id")) is int and 1 <= p["id"] <= 44}
    declared: dict[tuple[int, str], list[str]] = {}
    producers: dict[str, int] = {}
    for n, item in stages.items():
        for field in ("produces", "consumes"):
            values = item.get(field)
            if (not isinstance(values, list)
                    or any(not isinstance(v, str) or not ARTIFACT_ID.fullmatch(v)
                           for v in values)
                    or (field == "produces" and not values)):
                errors.append(f"prompt {n}: invalid {field} artifact list")
                continue
            if len(values) != len(set(values)):
                errors.append(f"prompt {n}: duplicate {field} artifact ID")
            declared[n, field] = values
            if field == "produces":
                for value in values:
                    if value in producers:
                        errors.append(f"artifact {value}: duplicate producer {producers[value]} and {n}")
                    else:
                        producers[value] = n
        filename = item.get("file")
        if not isinstance(filename, str) or not re.fullmatch(rf"{n}_[a-z0-9-]+\.md", filename):
            continue
        path = docs / filename
        if not path.is_file():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        for field in ("produces", "consumes"):
            if (n, field) not in declared:
                continue
            values = declared[n, field]
            label = field.capitalize() + ":"
            expected = label + " " + (", ".join(f"`{v}`" for v in values) or "None") + "."
            actual = [line for line in lines if line.startswith(label)]
            if actual != [expected]:
                errors.append(f"prompt {n}: prose {field} differs from SEQUENCE.json")

    for n, item in stages.items():
        ancestors: set[int] = set()
        pending = [n]
        while pending:
            current = pending.pop()
            deps = stages[current].get("depends_on")
            if not isinstance(deps, list):
                continue
            for dep in deps:
                # Ignore malformed/forward edges here; the main validator reports
                # them. Never recurse through invalid graphs or boolean IDs.
                if type(dep) is int and dep in stages and dep < current and dep not in ancestors:
                    ancestors.add(dep)
                    pending.append(dep)
        for artifact in declared.get((n, "consumes"), []):
            owner = producers.get(artifact)
            if owner is None:
                errors.append(f"prompt {n}: consumed artifact {artifact} has no producer")
            elif owner >= n:
                errors.append(f"prompt {n}: artifact {artifact} producer {owner} is not an earlier stage")
            elif owner not in ancestors:
                errors.append(f"prompt {n}: artifact {artifact} producer {owner} is outside dependency closure")

    inventory = docs / "HANDOFFS.md"
    if inventory.is_file():
        rows: dict[int, str] = {}
        for line in inventory.read_text(encoding="utf-8").splitlines():
            cells = [s.strip() for s in line.strip().strip("|").split("|")]
            if line.startswith("|") and len(cells) == 3 and cells[0].isdigit():
                if not re.fullmatch(r"[0-9]{1,2}", cells[0]):
                    errors.append("invalid stage number in HANDOFFS inventory")
                    continue
                n = int(cells[0])
                if n in rows:
                    errors.append(f"duplicate stage {n} in HANDOFFS inventory")
                rows[n] = cells[1]
        if set(rows) != set(range(1, 45)):
            errors.append("HANDOFFS inventory must contain stages 1 through 44 exactly once")
        for (n, field), values in declared.items():
            if field == "produces" and rows.get(n) != ", ".join(f"`{v}`" for v in values):
                errors.append(f"prompt {n}: HANDOFFS outputs differ from SEQUENCE.json")
    return errors


def validate(root: Path) -> list[str]:
    errors: list[str] = []
    docs = root / "docs" / "prompts"
    for name in ["AGENTS.md", "CLAUDE.md", "README.md", "START_HERE.md", "docs/README.md",
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
        seq = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_json_object)
    except (OSError, ValueError) as exc:
        return errors + [f"invalid SEQUENCE.json: {exc}"]
    if not isinstance(seq, dict):
        return errors + ["SEQUENCE.json must be an object"]
    if type(seq.get("version")) is not int or seq["version"] != 1:
        errors.append("SEQUENCE.json version must be 1")
    ps = seq.get("prompts")
    if not isinstance(ps, list):
        return errors + ["SEQUENCE.json prompts must be an array"]
    if len(ps) != 44:
        errors.append(f"expected 44 prompts, found {len(ps)}")
    errors.extend(validate_handoffs(docs, ps))
    ids = [x.get("id") for x in ps if isinstance(x, dict)]
    if ids != list(range(1, len(ps) + 1)):
        errors.append("IDs must be contiguous ascending positive integers")
    allnames: set[str] = set()
    filenames = {x.get("id"): x.get("file") for x in ps
                 if isinstance(x, dict) and type(x.get("id")) is int}
    index_path = docs / "README.md"
    index_rows = {}
    if index_path.is_file():
        for line in index_path.read_text(encoding="utf-8").splitlines():
            cells = [s.strip() for s in line.strip().strip("|").split("|")]
            if line.startswith("|") and len(cells) == 4 and cells[0].isdigit():
                if not re.fullmatch(r"[0-9]{1,2}", cells[0]):
                    errors.append("invalid stage number in README index")
                    continue
                row_id = int(cells[0])
                if row_id in index_rows:
                    errors.append(f"duplicate stage {row_id} in README index")
                index_rows[row_id] = cells[1:]
    if set(index_rows) != set(range(1, 45)):
        errors.append("README index must contain stages 1 through 44 exactly once")
    for item in ps:
        if not isinstance(item, dict):
            errors.append("non-object manifest item")
            continue
        n = item.get("id")
        name = item.get("file")
        deps = item.get("depends_on")
        kind = item.get("kind")
        if type(n) is not int or not isinstance(name, str) or not re.fullmatch(rf"{n}_[a-z0-9-]+\.md", name):
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
        title = item.get("title")
        phase = item.get("phase")
        if not isinstance(title, str) or not title.strip():
            errors.append(f"prompt {n}: missing title")
        elif t.split("\n", 1)[0] != f"# {n} — {title}":
            errors.append(f"prompt {n}: title differs from SEQUENCE.json")
        if not isinstance(phase, str) or not phase.strip():
            errors.append(f"prompt {n}: missing phase")
        elif f"Phase: **{phase}**. Type: **{kind}**." not in t:
            errors.append(f"prompt {n}: phase/type differs from SEQUENCE.json")
        for heading in ("Objective and boundaries", "Source audit", "Complete implementation work",
                        "Verification and exit criteria", "Documentation and delegation", "Checkpoint and report"):
            if f"## {heading}" not in t:
                errors.append(f"prompt {n}: missing heading {heading}")
        if f"docs/reports/prompt_{n}.md" not in t:
            errors.append(f"prompt {n}: missing stage report path")
        if not isinstance(kind, str) or kind not in {"software", "physical", "ci"}:
            errors.append(f"prompt {n}: unrecognized kind: {kind!r}")
        if not isinstance(deps, list) or any(type(d) is not int or d < 1 or d >= n for d in deps):
            errors.append(f"prompt {n}: bad dependencies: {deps!r}")
        if isinstance(deps, list) and len(deps) != len(set(map(str,deps))):
            errors.append(f"prompt {n}: duplicate dependencies")
        if isinstance(deps, list) and all(type(d) is int for d in deps):
            prereq = next((line for line in t.splitlines()
                           if line.startswith("Prerequisites:")), "")
            links = [(int(label), target) for label, target
                     in PREREQUISITE_LINK.findall(prereq)]
            expected = [(d, filenames.get(d)) for d in deps]
            if not prereq or links != expected or (not deps and "None" not in prereq):
                errors.append(f"prompt {n}: prose prerequisites differ from SEQUENCE.json")
            row = [f"[{title}]({name})", ", ".join(map(str, deps)) or "—", kind]
            if index_rows.get(n) != row:
                errors.append(f"prompt {n}: README index differs from SEQUENCE.json")
    disk_stages = {p.name for p in docs.glob("*.md") if re.match(r"^[0-9]+_.*\.md$", p.name)}
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
    print("PASS: 44 stages, dependency DAG, producer/consumer handoffs, prose/index parity and links")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
