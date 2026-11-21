#!/usr/bin/env python3
"""Regression tests for checking and synchronizing the version mirrors."""

import io
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import patch

import generate_version


class VersionMirrorTests(unittest.TestCase):
    def setUp(self) -> None:
        scratch = tempfile.TemporaryDirectory()
        self.addCleanup(scratch.cleanup)
        root = Path(scratch.name)
        root_override = patch.object(generate_version, "ROOT", root)
        root_override.start()
        self.addCleanup(root_override.stop)
        self.manifest = root / "library.json"
        self.manifest.write_bytes(b'{"version": "0.1.0"}\n')
        self.idf = root / "idf_component.yml"
        self.idf.write_bytes(b'version: "0.1.0"\n')
        self.header = root / "include/TMP11x/Version.h"
        with redirect_stdout(io.StringIO()):
            generate_version.sync_outputs(check_only=False)

    def test_rejects_version_inside_description(self) -> None:
        description = b"""description: 'start
version: "0.1.0"
end'
"""
        for content, diagnostic in (
            (description, "first content line"),
            (b'"version": "9.8.7"\n' + description, "exactly one top-level version field"),
        ):
            with self.subTest(diagnostic=diagnostic):
                self.idf.write_bytes(content)
                with self.assertRaisesRegex(ValueError, diagnostic):
                    generate_version.sync_outputs(check_only=True)
                self.assertEqual(self.idf.read_bytes(), content)

    def test_recognizes_yaml_version_keys_and_rejects_duplicates(self) -> None:
        for key in (b"version ", b'"version"', b"'version'"):
            with self.subTest(key=key):
                canonical = b'# Version mirror\r\n\r\n' + key + b': "0.1.0" # comment\r\n'
                self.idf.write_bytes(canonical)
                self.assertTrue(generate_version.sync_outputs(check_only=True))
                self.idf.write_bytes(canonical + b'version: "9.8.7"\r\n')
                with self.assertRaisesRegex(ValueError, "exactly one top-level version field"):
                    generate_version.sync_outputs(check_only=True)

    def test_unwritable_idf_leaves_header_unchanged(self) -> None:
        old_header = self.header.read_bytes()
        old_idf = self.idf.read_bytes()
        self.manifest.write_bytes(b'{"version": "0.2.0"}\n')
        open_path = Path.open

        def deny_idf_write(path, mode="r", *args, **kwargs):
            if path == self.idf and mode in ("r+b", "wb"):
                raise PermissionError("IDF manifest is read-only")
            return open_path(path, mode, *args, **kwargs)

        with patch.object(Path, "open", deny_idf_write):
            with self.assertRaises(PermissionError):
                generate_version.sync_outputs(check_only=False)
        self.assertEqual(self.header.read_bytes(), old_header)
        self.assertEqual(self.idf.read_bytes(), old_idf)
        with redirect_stderr(io.StringIO()):
            self.assertFalse(generate_version.sync_outputs(check_only=True))
        with redirect_stdout(io.StringIO()):
            generate_version.sync_outputs(check_only=False)
        self.assertTrue(generate_version.sync_outputs(check_only=True))


if __name__ == "__main__":
    unittest.main()
