#!/usr/bin/env python3
"""Negative tests proving the promptset integrity checker rejects corruption."""
from __future__ import annotations

import json
import re
import shutil
import tempfile
import unittest
from pathlib import Path

from check_promptset import LOCAL_LINK, validate

ROOT = Path(__file__).resolve().parent.parent


def preserve_link_targets(source, destination):
    """Keep valid non-document links without copying code, builds or archives."""
    for path in list(source.glob("*.md")) + list((source / "docs").rglob("*.md")):
        text = re.sub(r"```.*?```", "", path.read_text(encoding="utf-8"), flags=re.S)
        for href in LOCAL_LINK.findall(text):
            href = href.strip().split("#", 1)[0]
            if not href or href.startswith(("http://", "https://", "mailto:", "skills://")):
                continue
            original = (path.parent / href).resolve()
            if not original.exists() or not original.is_relative_to(source.resolve()):
                continue
            target = destination / original.relative_to(source.resolve())
            if target.exists():
                continue
            if original.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.touch()


class PromptsetCheckerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "source"
        self.root.mkdir()
        # Keep navigation targets, not .git, build outputs or vendor PDF bytes.
        for path in list(ROOT.glob("*.md")) + [ROOT / "LICENSE"] + list((ROOT / "docs").rglob("*")) + list((ROOT / "scripts").glob("*.py")):
            if not path.is_file() or "__pycache__" in path.parts or ".review" in path.parts:
                continue
            target = self.root / path.relative_to(ROOT)
            target.parent.mkdir(parents=True, exist_ok=True)
            if path.suffix in {".md", ".json", ".py"}:
                shutil.copyfile(path, target)
            else:
                target.touch()
        for name in ("scripts/pio.cmd", ".gitignore", ".gitattributes"):
            if (ROOT / name).is_file():
                (self.root / name).touch()
        preserve_link_targets(ROOT, self.root)

    def test_actual_promptset(self):
        self.assertEqual(validate(self.root), [])

    def test_missing_stage(self):
        (self.root / "docs/prompts/12_i2c-register-transport.md").unlink()
        self.assertTrue(any("missing stage file" in s for s in validate(self.root)))

    def test_forward_dependency(self):
        path = self.root / "docs/prompts/SEQUENCE.json"
        info = json.loads(path.read_text(encoding="utf-8"))
        info["prompts"][0]["depends_on"] = [2]
        path.write_text(json.dumps(info), encoding="utf-8")
        self.assertTrue(any("bad dependencies" in s for s in validate(self.root)))

    def test_missing_required_section(self):
        path = self.root / "docs/prompts/1_ecosystem-core-audit.md"
        path.write_text(path.read_text(encoding="utf-8").replace("## Source audit", "## Sources"), encoding="utf-8")
        self.assertTrue(any("missing heading Source audit" in s for s in validate(self.root)))

    def test_broken_local_link(self):
        path = self.root / "docs/prompts/README.md"
        with path.open("a", encoding="utf-8") as f:
            f.write("\n[Broken](missing_file.md)\n")
        self.assertTrue(any("broken link" in s for s in validate(self.root)))

    def test_prose_dependency_mismatch(self):
        path = self.root / "docs/prompts/2_tooling-peer-audit.md"
        path.write_text(path.read_text(encoding="utf-8").replace(
            "Prerequisites: [1](1_ecosystem-core-audit.md).", "Prerequisites: None."), encoding="utf-8")
        self.assertTrue(any("prose prerequisites differ" in s for s in validate(self.root)))

    def test_index_mismatch(self):
        path = self.root / "docs/prompts/README.md"
        path.write_text(path.read_text(encoding="utf-8").replace(
            "| 1 | software |", "| 3 | software |", 1), encoding="utf-8")
        self.assertTrue(any("README index differs" in s for s in validate(self.root)))

    def test_duplicate_index_row(self):
        path = self.root / "docs/prompts/README.md"
        text = path.read_text(encoding="utf-8")
        row = next(line for line in text.splitlines() if line.startswith("| 1 |"))
        path.write_text(text + "\n" + row + "\n", encoding="utf-8")
        self.assertTrue(any("duplicate stage 1" in s for s in validate(self.root)))

    def test_wrong_manifest_shape(self):
        path = self.root / "docs/prompts/SEQUENCE.json"
        info = json.loads(path.read_text(encoding="utf-8"))
        path.write_text("[]", encoding="utf-8")
        self.assertIn("SEQUENCE.json must be an object", validate(self.root))
        for kind in ([], {}, None):
            with self.subTest(kind=kind):
                info["prompts"][0]["kind"] = kind
                path.write_text(json.dumps(info), encoding="utf-8")
                self.assertTrue(any("unrecognized kind" in s for s in validate(self.root)))

    def test_boolean_dependency(self):
        path = self.root / "docs/prompts/SEQUENCE.json"
        info = json.loads(path.read_text(encoding="utf-8"))
        info["prompts"][1]["depends_on"] = [True]
        path.write_text(json.dumps(info), encoding="utf-8")
        self.assertTrue(any("bad dependencies" in s for s in validate(self.root)))

    def test_title_mismatch(self):
        path = self.root / "docs/prompts/1_ecosystem-core-audit.md"
        path.write_text(path.read_text(encoding="utf-8").replace(
            "# 1 — Audit current sensor-driver ownership and APIs", "# 1 — Wrong task"), encoding="utf-8")
        self.assertTrue(any("title differs" in s for s in validate(self.root)))

    def test_empty_stage(self):
        (self.root / "docs/prompts/1_ecosystem-core-audit.md").write_text("", encoding="utf-8")
        self.assertTrue(any("title differs" in s for s in validate(self.root)))

    def edit_handoff(self, stage, field, value, sync_prose=True):
        path = self.root / "docs/prompts/SEQUENCE.json"
        info = json.loads(path.read_text(encoding="utf-8"))
        item = info["prompts"][stage - 1]
        item[field] = value
        path.write_text(json.dumps(info), encoding="utf-8")
        if sync_prose:
            prompt = self.root / "docs/prompts" / item["file"]
            label = field.capitalize() + ":"
            text = label + " " + (", ".join(f"`{v}`" for v in value) or "None") + "."
            prompt.write_text(re.sub(rf"^{label}.*$", text,
                                    prompt.read_text(encoding="utf-8"), flags=re.M), encoding="utf-8")

    def test_handoff_unknown_producer(self):
        self.edit_handoff(2, "consumes", ["missing-artifact"])
        self.assertTrue(any("has no producer" in s for s in validate(self.root)))

    def test_handoff_future_or_self_producer(self):
        for name in ("ti-archive", "tooling-plan"):
            with self.subTest(artifact=name):
                self.edit_handoff(2, "consumes", [name])
                self.assertTrue(any("not an earlier stage" in s for s in validate(self.root)))

    def test_handoff_requires_dependency_path(self):
        # Stage 34 precedes host CLI37 numerically but is deliberately not a
        # mandatory host prerequisite. Simply naming it cannot add that edge.
        self.edit_handoff(37, "consumes", ["arduino-adapter-source"])
        self.assertTrue(any("outside dependency closure" in s for s in validate(self.root)))

    def test_handoff_transitive_input_is_allowed(self):
        # Stage4 -> stage3 -> stage1 provides a valid indirect producer.
        self.edit_handoff(4, "consumes", ["ti-archive", "ecosystem-audit"])
        self.assertEqual(validate(self.root), [])

    def test_handoff_duplicate_producer(self):
        self.edit_handoff(2, "produces", ["ecosystem-audit"])
        self.assertTrue(any("duplicate producer" in s for s in validate(self.root)))

    def test_handoff_invalid_lists(self):
        for value in (None, {}, "ecosystem-audit", [True], [{}], ["Bad_ID"]):
            with self.subTest(value=value):
                self.edit_handoff(2, "consumes", value, sync_prose=False)
                self.assertTrue(any("invalid consumes artifact list" in s for s in validate(self.root)))
        self.edit_handoff(2, "produces", [], sync_prose=False)
        self.assertTrue(any("invalid produces artifact list" in s for s in validate(self.root)))

    def test_handoff_duplicate_input(self):
        self.edit_handoff(2, "consumes", ["ecosystem-audit", "ecosystem-audit"])
        self.assertTrue(any("duplicate consumes artifact ID" in s for s in validate(self.root)))

    def test_handoff_prose_mismatch(self):
        self.edit_handoff(2, "consumes", [], sync_prose=False)
        self.assertTrue(any("prose consumes differs" in s for s in validate(self.root)))

    def test_handoff_inventory_mismatch(self):
        path = self.root / "docs/prompts/HANDOFFS.md"
        path.write_text(path.read_text(encoding="utf-8").replace(
            "| 1 | `ecosystem-audit` |", "| 1 | `wrong-artifact` |"), encoding="utf-8")
        self.assertTrue(any("HANDOFFS outputs differ" in s for s in validate(self.root)))

    def test_handoff_inventory_missing_or_duplicate_stage(self):
        path = self.root / "docs/prompts/HANDOFFS.md"
        original = path.read_text(encoding="utf-8")
        row = next(line for line in original.splitlines() if line.startswith("| 1 |"))
        path.write_text(original.replace(row + "\n", ""), encoding="utf-8")
        self.assertTrue(any("HANDOFFS inventory must contain" in s for s in validate(self.root)))
        path.write_text(original + "\n" + row + "\n", encoding="utf-8")
        self.assertTrue(any("duplicate stage 1 in HANDOFFS" in s for s in validate(self.root)))

    def test_missing_handoff_contract(self):
        (self.root / "docs/prompts/HANDOFFS.md").unlink()
        self.assertTrue(any("missing shared contract: HANDOFFS.md" in s for s in validate(self.root)))

    def test_non_ascii_or_excessive_table_stage_number(self):
        for filename in ("HANDOFFS.md", "README.md"):
            path = self.root / "docs/prompts" / filename
            original = path.read_text(encoding="utf-8")
            for numeral in ("²", "9" * 5000):
                with self.subTest(file=filename, numeral=numeral[:10]):
                    path.write_text(original.replace("| 1 |", f"| {numeral} |", 1), encoding="utf-8")
                    self.assertTrue(any("invalid stage number" in s for s in validate(self.root)))
            path.write_text(original, encoding="utf-8")

    def test_duplicate_manifest_key(self):
        path = self.root / "docs/prompts/SEQUENCE.json"
        original = path.read_text(encoding="utf-8")
        for field in ("consumes", "produces", "depends_on"):
            with self.subTest(field=field):
                duplicate = original.replace(f'"{field}":', f'"{field}": [], "{field}":', 1)
                path.write_text(duplicate, encoding="utf-8")
                self.assertTrue(any("duplicate JSON key" in s for s in validate(self.root)))

    def test_fixture_excludes_repository_and_pdf_bytes(self):
        self.assertFalse((self.root / ".git").exists())
        self.assertTrue(all(path.stat().st_size == 0 for path in self.root.rglob("*.pdf")))

    def test_fixture_preserves_future_source_link(self):
        header = self.root / "include/TMP11x/TMP11x.h"
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text("// future public header\n", encoding="utf-8")
        with (self.root / "README.md").open("a", encoding="utf-8") as output:
            output.write("\n[Public header](include/TMP11x/TMP11x.h)\n")
        destination = Path(self.tmp.name) / "linked-targets"
        preserve_link_targets(self.root, destination)
        copied = destination / "include/TMP11x/TMP11x.h"
        self.assertTrue(copied.is_file())
        self.assertEqual(copied.stat().st_size, 0)

if __name__ == "__main__":
    unittest.main(verbosity=2)
