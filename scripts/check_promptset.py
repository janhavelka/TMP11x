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
ORIGINAL_STAGE_COUNT = 44
STAGE_IDS = set(range(1, 56))
COVERAGE_IDS = {f"{prefix}{number:02d}" for prefix, count in (("R", 18), ("F", 20), ("X", 8))
                for number in range(1, count + 1)}


def coverage_rows(path: Path):
    """Yield requirement rows; Markdown references elsewhere are not evidence rows."""
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if cells and re.match(r"^[RFX][0-9]", cells[0]):
            yield cells


def validate_coverage(root: Path) -> list[str]:
    errors: list[str] = []
    inventory = root / "docs/prompts/FULL_COVERAGE.md"
    if not inventory.is_file():
        return errors
    seen: set[str] = set()
    for cells in coverage_rows(inventory):
        row_id = cells[0]
        if row_id not in COVERAGE_IDS:
            errors.append(f"FULL_COVERAGE: invalid requirement ID {row_id}")
        if row_id in seen:
            errors.append(f"FULL_COVERAGE: duplicate requirement ID {row_id}")
        seen.add(row_id)
        owners = cells[2] if len(cells) == 4 else ""
        # Explicit owner column, not a search for numbers in descriptive prose.
        terms = re.split(r"\s*[,/]\s*", owners)
        parsed: list[int] = []
        valid = bool(owners)
        for term in terms:
            match = re.fullmatch(r"([0-9]{1,2})(?:[–-]([0-9]{1,2}))?", term)
            if match is None:
                valid = False
                break
            first, last = int(match[1]), int(match[2] or match[1])
            if not 1 <= first <= last <= ORIGINAL_STAGE_COUNT:
                valid = False
                break
            parsed.extend(range(first, last + 1))
        if not valid or len(parsed) != len(set(parsed)):
            errors.append(f"FULL_COVERAGE {row_id}: invalid or duplicate owner stages {owners!r}")
    if seen != COVERAGE_IDS:
        errors.append(f"FULL_COVERAGE requirement inventory differs: missing={sorted(COVERAGE_IDS - seen)} extra={sorted(seen - COVERAGE_IDS)}")
    feature = root / "docs/feature-coverage.md"
    if feature.is_file():
        feature_ids: set[str] = set()
        for cells in coverage_rows(feature):
            row_id = cells[0]
            match = re.fullmatch(r"([RFX][0-9]{2})(?:[.-][A-Za-z0-9][A-Za-z0-9_.-]*)?", row_id)
            if match is None or match[1] not in COVERAGE_IDS:
                errors.append(f"feature-coverage: invalid requirement row {row_id}")
            else:
                feature_ids.add(match[1])
        missing = COVERAGE_IDS - feature_ids
        if missing:
            errors.append(f"feature-coverage: missing requirement rows {sorted(missing)}")
    return errors


def pdf_page_counts(root: Path) -> tuple[dict[Path, int], list[str]]:
    """Read pinned page metadata; original byte verification is a separate check."""
    manifest = root / "docs/reference/manifest.json"
    try:
        data = json.loads(manifest.read_text(encoding="utf-8"), object_pairs_hook=unique_json_object)
        documents = data["documents"]
        if not isinstance(documents, list):
            raise ValueError("documents must be an array")
        counts: dict[Path, int] = {}
        for document in documents:
            if not isinstance(document, dict):
                raise ValueError("document must be an object")
            path, count = document.get("path"), document.get("page_count")
            if not isinstance(path, str) or type(count) is not int or count < 1:
                raise ValueError("document needs path and positive integer page_count")
            resolved = (root / path).resolve()
            if not resolved.is_relative_to(root.resolve()) or resolved in counts:
                raise ValueError("invalid or duplicate document path")
            counts[resolved] = count
        return counts, []
    except (OSError, ValueError, KeyError, TypeError) as exc:
        return {}, [f"invalid reference page metadata: {exc}"]


def unique_json_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def validate_handoffs(docs: Path, stages: dict[int, dict], positions: dict[int, int]) -> list[str]:
    """Validate planned ownership/reachability without requiring future files."""
    errors: list[str] = []
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

    for n in stages:
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
                if (type(dep) is int and dep in positions
                        and positions[dep] < positions[current] and dep not in ancestors):
                    ancestors.add(dep)
                    pending.append(dep)
        for artifact in declared.get((n, "consumes"), []):
            owner = producers.get(artifact)
            if owner is None:
                errors.append(f"prompt {n}: consumed artifact {artifact} has no producer")
            elif positions[owner] >= positions[n]:
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
        if set(rows) != STAGE_IDS:
            errors.append("HANDOFFS inventory must contain every stage exactly once")
        elif list(rows) != list(stages):
            errors.append("HANDOFFS inventory order differs from SEQUENCE.json")
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
    if type(seq.get("version")) is not int or seq["version"] != 2:
        errors.append("SEQUENCE.json version must be 2")
    ps = seq.get("prompts")
    if not isinstance(ps, list):
        return errors + ["SEQUENCE.json prompts must be an array"]
    stages = {item["id"]: item for item in ps if isinstance(item, dict)
              and type(item.get("id")) is int and item["id"] in STAGE_IDS}
    if set(stages) != STAGE_IDS or len(ps) != len(STAGE_IDS):
        errors.append("manifest must contain every stage ID exactly once")
    positions = {n: position for position, n in enumerate(stages)}
    errors.extend(validate_handoffs(docs, stages, positions))
    errors.extend(validate_coverage(root))
    page_counts, page_errors = pdf_page_counts(root)
    errors.extend(page_errors)
    allnames: set[str] = set()
    filenames = {n: item.get("file") for n, item in stages.items()}
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
    if set(index_rows) != STAGE_IDS:
        errors.append("README index must contain every stage exactly once")
    elif list(index_rows) != list(stages):
        errors.append("README index order differs from SEQUENCE.json")
    for item in ps:
        if not isinstance(item, dict):
            errors.append("non-object manifest item")
            continue
        n = item.get("id")
        name = item.get("file")
        deps = item.get("depends_on")
        kind = item.get("kind")
        if (type(n) is not int or n not in positions or not isinstance(name, str)
                or not re.fullmatch(rf"{n}_[a-z0-9-]+\.md", name)):
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
        if not isinstance(kind, str) or kind not in {"software", "physical", "ci", "audit"}:
            errors.append(f"prompt {n}: unrecognized kind: {kind!r}")
        if not isinstance(deps, list) or any(
                type(d) is not int or d not in positions or positions[d] >= positions[n] for d in deps):
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
            href, _, fragment = href.strip().partition("#")
            if not href or href.startswith(("http://", "https://", "mailto:", "skills://")):
                continue
            if not (p.parent / href).exists():
                errors.append(f"broken link: {p.relative_to(root)} -> {href}")
            target = (p.parent / href).resolve()
            page_values = [part.partition("=")[2] for part in fragment.split("&")
                           if part.partition("=")[0] == "page"]
            if target.suffix.lower() == ".pdf" and page_values:
                page = page_values[0]
                count = page_counts.get(target)
                if (len(page_values) != 1 or not re.fullmatch(r"[0-9]{1,6}", page) or count is None
                        or not 1 <= int(page) <= count):
                    errors.append(f"invalid PDF page anchor: {p.relative_to(root)} -> {href}#{fragment}")
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
    print(f"PASS: {len(STAGE_IDS)} stages, execution order, dependency DAG, handoffs, "
          "coverage IDs/owners, prose/index parity and links/PDF pages")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
