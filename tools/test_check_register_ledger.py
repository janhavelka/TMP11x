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

    def test_citations_alone_cannot_replace_protocol_or_startup(self):
        for topic, missing in (("protocol", "pointer"), ("startup", "writes")):
            with self.subTest(topic=topic):
                profile = self.profile()
                original = profile[topic]
                profile[topic] = {"pages": original["pages"]}
                self.rejected(f"TMP117.{topic}: missing or empty {missing}")
                profile[topic] = original

    def test_bus_addresses_timing_and_ownership(self):
        protocol = self.profile()["protocol"]
        for key, replacement in (
            ("add0_7bit", {"GND": 0x48, "V+": 0x49, "SDA": 0x4B, "SCL": 0x4A}),
            ("scl_khz", {"min": 1, "max": 1000}),
            ("scl_khz", {"min": True, "max": 400}),
            ("interface_timeout_ms", {"min": 20, "max": 40}),
        ):
            with self.subTest(key=key, replacement=replacement):
                original = protocol[key]
                protocol[key] = replacement
                self.rejected(f"incorrect {key}")
                protocol[key] = original
        protocol["owners"] = [0]
        self.rejected("unknown owner stage")

    def test_nominal_cycle_table_requires_every_conv_avg_pair(self):
        field = self.field("CONFIG", "CONV", "TMP119")
        cycles = field.pop("cycle_ms_by_conv_avg")
        self.rejected("CONV x AVG cycle table")
        field["cycle_ms_by_conv_avg"] = cycles
        cycles[2][3] = 250  # CONV=010, AVG=11 occupies a nominal 1000 ms in Table 8-6.
        self.rejected("CONV x AVG cycle table")

    def test_single_conversion_limits_and_source_by_profile(self):
        for profile, wrong_max in (("TMP116", 17.5), ("TMP117", 17), ("TMP119", 17)):
            avg = self.field("CONFIG", "AVG", profile)
            with self.subTest(profile=profile):
                original = avg["single_conversion_ms"]["max"]
                avg["single_conversion_ms"]["max"] = wrong_max
                self.rejected(f"{profile}.CONFIG.AVG: incorrect single-conversion limits")
                avg["single_conversion_ms"]["max"] = original
                page = avg["single_conversion_page"]
                avg["single_conversion_page"] = page + 1
                self.rejected(f"{profile}.CONFIG.AVG: expected electrical row on page {page}")
                avg["single_conversion_page"] = page
                condition = avg.pop("timing_condition")
                self.rejected(f"{profile}.CONFIG.AVG: missing or empty timing_condition")
                avg["timing_condition"] = condition

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

    def test_pointer_07_rejects_fields_from_the_other_family(self):
        eeprom = self.register("EEPROM3", "TMP116")
        offset = self.register("OFFSET", "TMP117")
        eeprom_fields = eeprom["fields"]
        eeprom["fields"] = offset["fields"]
        self.rejected("TMP116.EEPROM3: expected D field with role eeprom-data")
        eeprom["fields"] = eeprom_fields
        offset["fields"] = eeprom_fields
        self.rejected("TMP117.OFFSET: expected D field with role calibration")

    def test_command_and_nvm_domains(self):
        self.field("CONFIG", "SOFT_RESET", "TMP119")["role"] = "setting"
        self.rejected("explicit command")
        self.field("CONFIG", "SOFT_RESET", "TMP119")["role"] = "command"
        self.field("CONFIG", "MOD")["nvm_mask"] = 0x0C00
        self.rejected("incorrect mask, access or NVM domain")

    def test_operation_guard_requires_lock_busy_and_independent_startup(self):
        guard = self.ledger["operation_guard"]
        for key, wrong in (
            ("register", 0x01), ("unlock_mask", 0), ("busy_mask", 0x1000),
            ("expected_masked_value", 0x8000), ("startup_wait_ms", 0),
        ):
            with self.subTest(key=key):
                original = guard[key]
                guard[key] = wrong
                self.rejected(f"operation_guard: incorrect {key}")
                guard[key] = original

    def test_typical_por_interval_does_not_supply_a_maximum(self):
        self.profile("TMP116")["startup"]["reset_max_ms"] = 1.5
        self.rejected("TMP116.startup: expected typical 1.5 ms with no specified maximum")

    def test_unlock_field_powers_up_locked_and_encodes_both_states(self):
        for profile in ("TMP116", "TMP117", "TMP119"):
            with self.subTest(profile=profile):
                unlock = self.field("EEPROM_UL", "EUN", profile)
                unlock["factory"] = "1"
                self.rejected(f"{profile}.EEPROM_UL.EUN: must power up locked (0)")
                unlock["factory"] = "0"
                encodings = unlock["encodings"]
                encodings["0"], encodings["1"] = encodings["1"], encodings["0"]
                self.rejected(f"{profile}.EEPROM_UL.EUN: expected locked=0 and unlocked=1")
                encodings["0"], encodings["1"] = encodings["1"], encodings["0"]

    def test_addressed_reset_is_model_gated(self):
        reset = self.profile()["addressed_reset"]
        self.profile("TMP116")["addressed_reset"] = reset
        self.rejected("TMP116.addressed_reset: unsupported on TMP116/N")
        self.profile("TMP116")["addressed_reset"] = None
        self.profile("TMP119")["addressed_reset"] = None
        self.rejected("TMP119.addressed_reset: missing addressed reset command record")

    def test_reset_preserves_only_stable_fields_and_requires_postwrite_wait(self):
        reset = self.profile()["addressed_reset"]
        for key, wrong in (
            ("register", 0x04), ("command_mask", 0x0001), ("stable_mask", 0xFFFC),
            ("readback", 2), ("postwrite_wait_ms", 0),
        ):
            with self.subTest(key=key):
                original = reset[key]
                reset[key] = wrong
                self.rejected(f"TMP117.addressed_reset: incorrect {key}")
                reset[key] = original
        reset["allowed_mode_codes"].append(3)
        self.rejected("expected canonical continuous and shutdown modes")

    def test_reset_records_keep_relock_and_reload_outcomes_unknown(self):
        self.profile("TMP119")["addressed_reset"]["unknown_effects"].remove("eun")
        self.rejected("TMP119.addressed_reset: incomplete or incorrect unknown effects")

    def test_reset_command_field_reads_zero_and_cites_both_source_claims(self):
        for profile in ("TMP117", "TMP119"):
            with self.subTest(profile=profile):
                field = self.field("CONFIG", "SOFT_RESET", profile)
                field["factory"] = "1"
                self.rejected(f"{profile}.CONFIG.SOFT_RESET: expected zero readback")
                field["factory"] = "0"
                reset = self.profile(profile)["addressed_reset"]
                original = reset["pages"]
                reset["pages"] = [original[0]]
                self.rejected(f"{profile}.addressed_reset: cite both the CONFIG diagram and field table")
                reset["pages"] = original

    def test_addressed_reset_requires_write_one_encoding(self):
        for profile in ("TMP117", "TMP119"):
            with self.subTest(profile=profile):
                field = self.field("CONFIG", "SOFT_RESET", profile)
                codes = field.pop("encodings")
                self.rejected(f"{profile}.CONFIG.SOFT_RESET: expected write-one reset command")
                field["encodings"] = {"0": codes["1"], "1": codes["0"]}
                self.rejected(f"{profile}.CONFIG.SOFT_RESET: expected write-one reset command")
                field["encodings"] = codes

    def test_identity_uses_the_correct_family_literal(self):
        self.field("DEVICE_ID", "DID", "TMP116")["factory"] = "0x117"
        self.rejected("expected compatibility ID 0x1116")

    def test_release_denies_writable_shadows_and_unlock(self):
        for profile in ("TMP116", "TMP117", "TMP119"):
            for register in self.profile(profile)["registers"]:
                if register["name"].startswith("EEPROM"):
                    with self.subTest(profile=profile, register=register["name"]):
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
        source = conflict["sources"][1]
        conflict["resolution_evidence"] = dict(source, pages=list(reversed(source["pages"])))
        self.rejected("repeats a disputed source claim")
        # A distinct explanation on an already cited page can qualify for manual review.
        conflict["resolution_evidence"]["claim"] = "Synthetic reconciliation for the schema test."
        self.path.write_text(json.dumps(self.ledger), encoding="utf-8")
        checker.check_file(self.path)

    def test_conflict_must_preserve_both_claims(self):
        conflict = self.ledger["conflicts"][0]
        conflict["sources"] = conflict["sources"][:1]
        self.rejected("paired source claims")

    def test_required_conflicts_survive_ledger_edits(self):
        conflicts = self.ledger["conflicts"]
        conflicts.remove(next(item for item in conflicts if item["id"] == "tmp119-id-default"))
        self.rejected("missing required conflicts: tmp119-id-default")


if __name__ == "__main__":
    unittest.main(verbosity=2)
