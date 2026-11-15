#!/usr/bin/env python3
"""Exercise archive corruption and non-overwriting restoration without network."""

from contextlib import redirect_stderr, redirect_stdout
import copy
import hashlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from check_references import load_documents, verify_bytes
import fetch_references


class ReferenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        # Integrity fixture, not evidence of valid sensor documentation.
        self.payload = b"%PDF-1.7\nreference integrity fixture\n"
        self.item = {
            "id": "fixture", "title": "Integrity fixture", "document_id": "TEST",
            "revision": "initial", "source_url": "https://www.ti.com/fixture.pdf",
            "resolved_url": "https://www.ti.com/fixture.pdf",
            "retrieved_date": "2026-11-15", "path": "docs/reference/source/fixture.pdf",
            "sha256": hashlib.sha256(self.payload).hexdigest(),
            "size_bytes": len(self.payload), "page_count": 1,
            "notice": "Synthetic test bytes", "role": "test only",
        }
        self.path = self.root / self.item["path"]
        self.manifest = self.root / "manifest.json"

    def load(self, data=None):
        if data is None:
            data = {"schema_version": 1, "documents": [self.item]}
        self.manifest.write_text(json.dumps(data), encoding="utf-8")
        return load_documents(self.manifest, self.root)

    def restore(self, response=None, side_effect=None):
        with patch.object(fetch_references, "load_documents", return_value=self.load()), \
             patch.object(fetch_references, "urlopen", return_value=io.BytesIO(response or self.payload),
                          side_effect=side_effect) as network, \
             patch("sys.argv", ["fetch_references.py"]), \
             redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            return fetch_references.main(), network.call_count

    def test_same_size_corruption_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
            verify_bytes(self.item, self.payload[:-1] + b"X")

    def test_html_and_truncated_response_are_rejected(self):
        for payload in (b"<html>error</html>", self.payload[:-1]):
            with self.subTest(payload=payload), self.assertRaises(ValueError):
                verify_bytes(self.item, payload)

    def test_malformed_manifest_is_rejected(self):
        for data in ([], {"schema_version": True, "documents": [self.item]},
                     {"schema_version": 1, "documents": [None]}):
            with self.subTest(data=data), self.assertRaises(ValueError):
                self.load(data)
        self.item["size_bytes"] = float(len(self.payload))
        with self.assertRaises(ValueError):
            self.load()

    def test_duplicate_archive_entry_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate reference"):
            self.load({"schema_version": 1, "documents": [self.item, copy.copy(self.item)]})

    def test_escaping_and_absolute_paths_are_rejected(self):
        for path in ("docs/reference/source/../../../escape.pdf", self.path.as_posix()):
            with self.subTest(path=path), self.assertRaises(ValueError):
                self.item["path"] = path
                self.load()

    def test_missing_file_is_restored_then_verified_without_network(self):
        self.assertEqual(self.restore(), (0, 1))
        self.assertEqual(self.path.read_bytes(), self.payload)
        self.assertEqual(self.restore(), (0, 0))

    def test_existing_modified_file_is_preserved_without_network(self):
        self.path.parent.mkdir(parents=True)
        self.path.write_bytes(b"user modification")
        self.assertEqual(self.restore(), (1, 0))
        self.assertEqual(self.path.read_bytes(), b"user modification")

    def test_changed_remote_does_not_create_file(self):
        self.assertEqual(self.restore(self.payload[:-1] + b"X"), (1, 1))
        self.assertFalse(self.path.exists())

    def test_concurrent_file_is_not_overwritten(self):
        def concurrent_download(*args, **kwargs):
            self.path.parent.mkdir(parents=True)
            self.path.write_bytes(b"concurrent user file")
            return io.BytesIO(self.payload)

        self.assertEqual(self.restore(side_effect=concurrent_download), (1, 1))
        self.assertEqual(self.path.read_bytes(), b"concurrent user file")


if __name__ == "__main__":
    unittest.main(verbosity=2)
