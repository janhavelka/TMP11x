#!/usr/bin/env python3
"""Run offline rejection tests on isolated copies of the checked-in ledger."""

from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import check_register_ledger as checker


class RegisterLedgerTests(unittest.TestCase):
    def setUp(self):
        self.ledger = json.loads(checker.LEDGER.read_text(encoding="utf-8"))
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.path = Path(temp.name) / "fixture.json"

    def profile(self, name="TMP117"):
        return self.ledger["profiles"][name]

    def register(self, name, profile="TMP117"):
        return next(r for r in self.profile(profile)["registers"] if r["name"] == name)

    def field(self, register, name, profile="TMP117"):
        return next(f for f in self.register(register, profile)["fields"] if f["name"] == name)

    def rejected(self, message, raw=None):
        self.path.write_text(json.dumps(self.ledger) if raw is None else raw, encoding="utf-8")
        error = io.StringIO()
        with redirect_stderr(error), redirect_stdout(io.StringIO()):
            status = checker.main([str(self.path)])
        self.assertNotEqual(status, 0)
        self.assertIn(message, error.getvalue())

    def test_default_command_from_another_directory(self):
        proc = subprocess.run(
            [sys.executable, "-B", checker.__file__], cwd=self.path.parent,
            capture_output=True, text=True, check=False,
        )
        self.assertEqual(proc.returncode, 0, proc.stderr)

    def test_variant_pointer_and_id_splits(self):
        # TMP116 Tables 3/13/15, TMP117 Tables 7-3/7-13/7-15 and TMP119 Tables 8-2/8-12/8-14.
        self.assertEqual(self.register("EEPROM3", "TMP116")["address"], 0x07)
        self.assertEqual(self.field("DEVICE_ID", "DID", "TMP116")["mask"], 0xFFFF)
        for profile in ("TMP117", "TMP119"):
            self.assertEqual(self.register("OFFSET", profile)["address"], 0x07)
            self.assertEqual(self.register("EEPROM3", profile)["address"], 0x08)
            self.assertEqual(self.field("DEVICE_ID", "DID", profile)["mask"], 0x0FFF)

    def test_duplicate_json_key_is_rejected_before_collapse(self):
        raw = json.dumps(self.ledger)
        # A last-value-wins parser would restore the original valid mask.
        token = '"nvm_mask":'
        self.assertIn(token, raw)
        self.rejected("duplicate JSON key 'nvm_mask'", raw.replace(token, '"nvm_mask": -1, ' + token, 1))

    def test_duplicate_register_in_one_profile(self):
        self.profile()["registers"].append(self.register("OFFSET"))
        self.rejected("duplicate register address 0x07")

    def test_overlapping_fields_in_one_word(self):
        self.field("CONFIG", "POL")["mask"] |= 0x0010
        self.rejected("overlapping field mask")

    def test_out_of_width_and_empty_masks(self):
        for mask in (0x10000, 0, -1, True):
            with self.subTest(mask=mask):
                self.field("CONFIG", "POL")["mask"] = mask
                self.rejected("outside register width")

    def test_missing_and_out_of_range_register_provenance(self):
        register = self.register("TEMP")
        del register["pages"]
        self.rejected("nonempty pages")
        register["pages"] = [999]
        self.rejected("invalid tmp117 source page")

    def test_wrong_family_source_and_grade(self):
        profile = self.profile()
        source = profile["source"]
        profile["source"] = self.profile("TMP116")["source"]
        self.rejected("source must be the pinned")
        profile["source"] = source
        profile["models"].remove("TMP117M")
        self.rejected("selected BOM grades")

    def test_non_register_provenance(self):
        for topic in ("protocol", "startup"):
            with self.subTest(topic=topic):
                record = self.profile()[topic]
                original = record.pop("pages")
                self.rejected(f"TMP117.{topic}: expected nonempty pages")
                record["pages"] = [0]
                self.rejected("invalid tmp117 source page")
                record["pages"] = original

    def test_reserved_and_invalid_pointers_do_not_alias(self):
        register = self.register("OFFSET")
        for address in (*range(0x09, 0x0F), 0x17, 0xF7, -1, 0x107, True):
            with self.subTest(address=address):
                register["address"] = address
                self.rejected("invalid or reserved pointer")

    def test_tmp116_has_no_offset_and_tmp119_has_no_fourth_shadow(self):
        register = self.register("EEPROM3", "TMP116")
        register["name"] = "OFFSET"
        self.rejected("0x07 must be EEPROM3")
        register["name"] = "EEPROM3"
        self.register("EEPROM3", "TMP119")["name"] = "EEPROM4"
        self.rejected("0x08 must be EEPROM3")

    def test_command_and_nvm_domains(self):
        self.field("CONFIG", "SOFT_RESET", "TMP119")["role"] = "setting"
        self.rejected("explicit command")
        self.field("CONFIG", "SOFT_RESET", "TMP119")["role"] = "command"
        self.field("CONFIG", "MOD")["nvm_mask"] = 0x0C00
        self.rejected("incorrect mask, access or NVM domain")

    def test_identity_uses_the_correct_family_literal(self):
        self.field("DEVICE_ID", "DID", "TMP116")["factory"] = "0x117"
        self.rejected("expected compatibility ID 0x1116")

    def test_release_denies_writable_shadows_and_unlock(self):
        for name in ("EEPROM_UL", "EEPROM1", "EEPROM2", "EEPROM3", "EEPROM4"):
            with self.subTest(name=name):
                register = self.register(name, "TMP116")
                register["policy"] = "guarded"
                self.rejected("release policy deny-write")
                register["policy"] = "deny-write"

    def test_status_access_and_mode_encodings(self):
        ready = self.field("CONFIG", "DATA_READY")
        ready["access"] = "R/C"
        self.rejected("incorrect mask, access or NVM domain")
        ready["access"] = "R"
        del self.field("CONFIG", "MOD")["encodings"]["2"]
        self.rejected("expected encodings 0 through 3")

    def test_resolution_requires_source_evidence(self):
        conflict = self.ledger["conflicts"][0]
        conflict["status"] = "resolved"
        conflict.pop("resolution_evidence", None)
        self.rejected("resolution_evidence")
        conflict["resolution_evidence"] = {"profile": "TMP116", "claim": "fixture", "pages": [0]}
        self.rejected("invalid tmp116 source page")
        # Exercise the resolution schema without changing the real conflict's disposition.
        conflict["resolution_evidence"]["pages"] = [17]
        self.path.write_text(json.dumps(self.ledger), encoding="utf-8")
        checker.check_file(self.path)

    def test_conflict_must_preserve_both_claims(self):
        conflict = self.ledger["conflicts"][0]
        conflict["sources"] = conflict["sources"][:1]
        self.rejected("paired source claims")


if __name__ == "__main__":
    unittest.main(verbosity=2)
