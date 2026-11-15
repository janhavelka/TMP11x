#!/usr/bin/env python3
"""Negative tests proving the promptset integrity checker rejects corruption."""
from __future__ import annotations

import json
import shutil
import tempfile
import unittest
from pathlib import Path

from check_promptset import validate

ROOT = Path(__file__).resolve().parent.parent

class PromptsetCheckerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "source"
        shutil.copytree(ROOT, self.root)

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

if __name__ == "__main__":
    unittest.main(verbosity=2)
