#!/usr/bin/env python3
"""Validate the TMP11x register ledger against its pinned sources and release gates."""

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs/registers/tmp11x.json"
PROFILES = {"TMP116", "TMP117", "TMP119"}
REQUIRED_CONFLICTS = {
    "tmp116-startup-busy", "tmp116-shadow-defaults", "tmp116-operating-range",
    "tmp116-alert-connection", "tmp116-program-current", "tmp116-conv-cycle",
    "tmp117-data-word-count", "tmp117-stored-fields", "tmp117-eeprom2-default",
    "tmp117-reset-access", "tmp117-one-shot-alert", "tmp117-shutdown-current",
    "tmp117-program-current", "tmp117-conv-cycle", "tmp117-address-reference",
    "tmp117-supply-summary", "tmp117-revision-default",
    "tmp119-data-word-count", "tmp119-stored-fields", "tmp119-eeprom2-default",
    "tmp119-reset-access", "tmp119-one-shot-alert", "tmp119-shutdown-current",
    "tmp119-program-current", "tmp119-conv-cycle", "tmp119-id-default",
    "tmp119-accuracy-summary", "tmp119-summary-tmp116-supply", "tmp119-characterization-speed",
    "tmp119m-orderable", "tmp119-address-reference",
}


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def text_fields(obj, names, where):
    if not isinstance(obj, dict):
        raise ValueError(f"{where}: expected an object")
    for name in names:
        if not isinstance(obj.get(name), str) or not obj[name].strip():
            raise ValueError(f"{where}: missing or empty {name}")


def items(obj, key, where):
    value = obj.get(key)
    if not isinstance(value, list) or not value:
        raise ValueError(f"{where}: expected nonempty {key} list")
    return value


def pages(obj, document, where):
    for page in items(obj, "pages", where):
        if type(page) is not int or not 1 <= page <= document["page_count"]:
            raise ValueError(f"{where}: invalid {document['id']} source page {page!r}")


def check_owners(obj, stages, where):
    for owner in items(obj, "owners", where):
        if type(owner) is not int or owner not in stages:
            raise ValueError(f"{where}: unknown owner stage {owner!r}")


def check_fields(register, where):
    """Check the partition before interpreting any model-specific field names."""
    fields = {}
    used_bits = 0
    for field in items(register, "fields", where):
        text_fields(field, ("name", "access", "factory", "role", "read", "write"), where)
        name = field["name"]
        location = f"{where}.{name}"
        mask = field.get("mask")
        if type(mask) is not int or not 0 < mask < 1 << register["width"]:
            raise ValueError(f"{location}: mask is zero or outside register width")
        if mask & used_bits:
            raise ValueError(f"{location}: overlapping field mask {mask:#06x}")
        if name in fields:
            raise ValueError(f"{location}: duplicate field name")
        used_bits |= mask
        nvm = field.get("nvm_mask")
        if type(nvm) is not int or nvm < 0 or nvm & ~mask:
            raise ValueError(f"{location}: nvm_mask must be a subset of mask")
        if "encodings" in field:
            codes = field["encodings"]
            if not isinstance(codes, dict):
                raise ValueError(f"{location}: encodings must be an object")
            for code, meaning in codes.items():
                if (not code.isascii() or not code.isdecimal() or str(int(code)) != code
                        or int(code) >= 1 << mask.bit_count()
                        or not isinstance(meaning, str) or not meaning.strip()):
                    raise ValueError(f"{location}: invalid encoding {code!r}")
        fields[name] = field
    if used_bits != (1 << register["width"]) - 1:
        raise ValueError(f"{where}: fields must cover the entire register, including reserved bits")
    return fields


def check_semantics(profile, register, fields, where):
    """Independent invariants from the three TI map and field tables."""
    name = register["name"]
    readonly = name in ("TEMP", "DEVICE_ID")
    access = "R" if readonly else "R/W"
    if name == "CONFIG":
        policy = "mixed"
    elif name.startswith("EEPROM"):
        policy = "deny-write"
    else:
        policy = "observe" if readonly else "guarded"
    if register["access"] != access or register["policy"] != policy:
        raise ValueError(f"{where}: expected hardware access {access} and release policy {policy}")

    # Each tuple is (register-positioned mask, literal hardware access, NVM-eligible bits).
    if name == "CONFIG":
        expected = {
            "HIGH": (0x8000, "R", 0), "LOW": (0x4000, "R", 0),
            "DATA_READY": (0x2000, "R", 0), "EEPROM_BUSY": (0x1000, "R", 0),
            "MOD": (0x0C00, "R/W", 0x0400), "CONV": (0x0380, "R/W", 0x0380),
            "AVG": (0x0060, "R/W", 0x0060), "THERM": (0x0010, "R/W", 0x0010),
            "POL": (0x0008, "R/W", 0x0008), "DR_ALERT": (0x0004, "R/W", 0x0004),
            "RESERVED": (0x0003 if profile == "TMP116" else 0x0001, "R", 0),
        }
        if profile != "TMP116":
            expected["SOFT_RESET"] = (0x0002, "R/W", 0)
            if fields.get("SOFT_RESET", {}).get("role") != "command":
                raise ValueError(f"{where}: SOFT_RESET must be an explicit command")
        for selector in ("MOD", "CONV", "AVG", "THERM", "POL", "DR_ALERT"):
            count = 1 << expected[selector][0].bit_count()
            if set(fields.get(selector, {}).get("encodings", {})) != {str(i) for i in range(count)}:
                raise ValueError(f"{where}.{selector}: expected encodings 0 through {count - 1}")
        # TI Tables 7, 7-7 and 8-6 give these nominal ms for each CONV (row) and AVG (column).
        cycles = fields["CONV"].get("cycle_ms_by_conv_avg")
        periods = (15.5, 125, 250, 500, 1000, 4000, 8000, 16000)
        avg_periods = (15.5, 125, 500, 1000)
        expected_cycles = [[max(period, avg) for avg in avg_periods] for period in periods]
        if cycles != expected_cycles:
            raise ValueError(f"{where}.CONV: incorrect nominal CONV x AVG cycle table")
    elif name == "EEPROM_UL":
        expected = {"EUN": (0x8000, "R/W", 0), "BUSY": (0x4000, "R", 0),
                    "RESERVED": (0x3FFF, "R", 0)}
    elif name == "DEVICE_ID":
        if profile == "TMP116":
            expected = {"DID": (0xFFFF, "R", 0)}
        else:
            expected = {"REV": (0xF000, "R", 0), "DID": (0x0FFF, "R", 0)}
    else:
        full_words = {"TEMP": ("T", "measurement"), "THIGH": ("H", "limit"),
                      "TLOW": ("L", "limit"), "OFFSET": ("D", "calibration")}
        field_name, role = full_words.get(name, ("D", "eeprom-data"))
        if fields.get(field_name, {}).get("role") != role:
            raise ValueError(f"{where}: expected {field_name} field with role {role}")
        expected = {field_name: (0xFFFF, access, 0 if readonly else 0xFFFF)}

    if fields.keys() != expected.keys():
        raise ValueError(f"{where}: expected fields {', '.join(expected)}")
    for field_name, layout in expected.items():
        field = fields[field_name]
        if (field["mask"], field["access"], field["nvm_mask"]) != layout:
            raise ValueError(f"{where}.{field_name}: incorrect mask, access or NVM domain")
    if name == "EEPROM_UL":
        unlock = fields["EUN"]
        if unlock["factory"] != "0":
            raise ValueError(f"{where}.EUN: must power up locked (0)")
        if unlock.get("encodings") != {
            "0": "locked (shadow writes volatile)",
            "1": "unlocked (writes can program EEPROM)",
        }:
            raise ValueError(f"{where}.EUN: expected locked=0 and unlocked=1")
    if "SOFT_RESET" in fields and fields["SOFT_RESET"]["factory"] != "0":
        raise ValueError(f"{where}.SOFT_RESET: expected zero readback")
    if name == "DEVICE_ID":
        did = 0x1116 if profile == "TMP116" else 0x117
        try:
            factory_id = int(fields["DID"]["factory"], 0)
        except ValueError:
            raise ValueError(f"{where}.DID: expected a numeric factory ID") from None
        if factory_id != did:
            raise ValueError(f"{where}.DID: expected compatibility ID {did:#x}")


def check_profile(name, profile, document, stages):
    text_fields(profile, ("source",), name)
    if profile["source"] != document["path"] or not (ROOT / document["path"]).is_file():
        raise ValueError(f"{name}: source must be the pinned {document['path']}")
    for topic, required in (
        ("startup", ("writes", "policy")),
        ("protocol", ("pointer", "read_word", "write_word", "timeout_condition", "stop_behavior", "ownership")),
    ):
        section = profile.get(topic)
        if not isinstance(section, dict):
            raise ValueError(f"{name}: missing {topic} source record")
        pages(section, document, f"{name}.{topic}")
        text_fields(section, required, f"{name}.{topic}")
    startup = profile["startup"]
    if (startup.get("reset_typ_ms") != 1.5 or "reset_max_ms" not in startup
            or startup["reset_max_ms"] is not None):
        raise ValueError(f"{name}.startup: expected typical 1.5 ms with no specified maximum")

    reset = profile.get("addressed_reset")
    where = f"{name}.addressed_reset"
    if name == "TMP116":
        if "addressed_reset" not in profile or reset is not None:
            raise ValueError(f"{where}: unsupported on TMP116/N")
    else:
        if not isinstance(reset, dict):
            raise ValueError(f"{where}: missing addressed reset command record")
        pages(reset, document, where)
        source_pages = {27, 28} if name == "TMP117" else {34, 35}
        if not source_pages.issubset(reset["pages"]):
            raise ValueError(f"{where}: cite both the CONFIG diagram and field table")
        for key, expected in (
            ("register", 0x01), ("command_mask", 0x0002), ("stable_mask", 0x0FFC),
            ("readback", 0), ("postwrite_wait_ms", 2),
        ):
            if type(reset.get(key)) is not int or reset[key] != expected:
                raise ValueError(f"{where}: incorrect {key}")
        modes = reset.get("allowed_mode_codes")
        if modes != [0, 1] or any(type(code) is not int for code in modes):
            raise ValueError(f"{where}: expected canonical continuous and shutdown modes")
        unknowns = items(reset, "unknown_effects", where)
        if not all(isinstance(effect, str) for effect in unknowns) or sorted(unknowns) != [
            "bus-availability", "companion-bits", "eeprom-reload", "eun",
        ]:
            raise ValueError(f"{where}: incomplete or incorrect unknown effects")
    protocol = profile["protocol"]
    check_owners(protocol, stages, f"{name}.protocol")
    for key, expected_values in (
        ("add0_7bit", {"GND": 0x48, "V+": 0x49, "SDA": 0x4A, "SCL": 0x4B}),
        ("scl_khz", {"min": 1, "max": 400}),
        ("interface_timeout_ms", {"min": 20, "typ": 35, "max": 40}),
    ):
        values = protocol.get(key)
        if values != expected_values or any(type(value) is not int for value in values.values()):
            raise ValueError(f"{name}.protocol: incorrect {key}")
    models = items(profile, "models", name)
    if not all(isinstance(m, str) for m in models) or sorted(models) != sorted(document["models"]):
        raise ValueError(f"{name}: models must match the selected BOM grades in the manifest")
    holes = items(profile, "reserved_pointers", name)
    if not all(type(p) is int for p in holes) or sorted(holes) != list(range(0x09, 0x0F)):
        raise ValueError(f"{name}: reserved pointers must enumerate 0x09 through 0x0E")
    upper = profile.get("pointer_upper_mask")
    if type(upper) is not int or upper != 0xF0:
        raise ValueError(f"{name}: pointer_upper_mask must be 0xF0")

    expected = {
        0x00: "TEMP", 0x01: "CONFIG", 0x02: "THIGH", 0x03: "TLOW", 0x04: "EEPROM_UL",
        0x05: "EEPROM1", 0x06: "EEPROM2", 0x07: "EEPROM3" if name == "TMP116" else "OFFSET",
        0x08: "EEPROM4" if name == "TMP116" else "EEPROM3", 0x0F: "DEVICE_ID",
    }
    seen = set()
    for reg in items(profile, "registers", name):
        text_fields(reg, ("name", "table", "factory", "reset", "access", "policy", "read", "write",
                          "prerequisites"), name)
        address = reg.get("address")
        where = f"{name}.{reg['name']}"
        # Test the supplied selector as-is. Truncation would admit e.g. 0x17 as OFFSET.
        if type(address) is not int or address not in expected:
            raise ValueError(f"{where}: invalid or reserved pointer {address!r}")
        if address in seen:
            raise ValueError(f"{where}: duplicate register address {address:#04x}")
        seen.add(address)
        if expected[address] != reg["name"]:
            raise ValueError(f"{where}: {address:#04x} must be {expected[address]}")
        if type(reg.get("width")) is not int or reg["width"] != 16:
            raise ValueError(f"{where}: expected 16-bit register width")
        pages(reg, document, where)
        check_owners(reg, stages, where)
        check_semantics(name, reg, check_fields(reg, where), where)
    if seen != expected.keys():
        missing = ", ".join(expected[p] for p in sorted(expected.keys() - seen))
        raise ValueError(f"{name}: missing registers {missing}")


def check_citation(citation, documents, where):
    text_fields(citation, ("profile", "claim"), where)
    if citation["profile"] not in documents:
        raise ValueError(f"{where}: unknown source profile {citation['profile']!r}")
    pages(citation, documents[citation["profile"]], where)
    return citation["profile"], frozenset(citation["pages"]), citation["claim"].strip()


def check_file(path):
    ledger = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_keys)
    if (not isinstance(ledger, dict) or type(ledger.get("schema_version")) is not int
            or ledger["schema_version"] != 1):
        raise ValueError("expected schema_version 1")
    profiles = ledger.get("profiles")
    if not isinstance(profiles, dict) or profiles.keys() != PROFILES:
        raise ValueError("expected TMP116, TMP117 and TMP119 profiles")
    guard = ledger.get("operation_guard")
    if not isinstance(guard, dict):
        raise ValueError("ledger: missing operation_guard record")
    for key, expected in (
        ("register", 0x04), ("unlock_mask", 0x8000), ("busy_mask", 0x4000),
        ("expected_masked_value", 0), ("startup_wait_ms", 3),
    ):
        if type(guard.get(key)) is not int or guard[key] != expected:
            raise ValueError(f"operation_guard: incorrect {key}")
    manifest = json.loads((ROOT / "docs/reference/manifest.json").read_text(encoding="utf-8"))
    documents = {doc["id"].upper(): doc for doc in manifest["documents"] if doc["id"].upper() in PROFILES}
    sequence = json.loads((ROOT / "docs/prompts/SEQUENCE.json").read_text(encoding="utf-8"))
    stages = {stage["id"] for stage in sequence["prompts"]}
    for name, profile in profiles.items():
        check_profile(name, profile, documents[name], stages)

    seen = set()
    for conflict in items(ledger, "conflicts", "ledger"):
        text_fields(conflict, ("id", "status", "policy"), "conflict")
        where = f"conflict {conflict['id']}"
        if conflict["id"] in seen:
            raise ValueError(f"{where}: duplicate conflict ID")
        seen.add(conflict["id"])
        sources = items(conflict, "sources", where)
        citations = {check_citation(source, documents, where) for source in sources}
        if len(citations) < 2:
            raise ValueError(f"{where}: expected paired source claims")
        if conflict["status"] == "resolved":
            evidence = check_citation(conflict.get("resolution_evidence"), documents, f"{where} resolution_evidence")
            if evidence in citations:
                raise ValueError(f"{where}: resolution_evidence repeats a disputed source claim")
        elif conflict["status"] != "open":
            raise ValueError(f"{where}: expected open or resolved status")
    missing = REQUIRED_CONFLICTS - seen
    if missing:
        raise ValueError(f"missing required conflicts: {', '.join(sorted(missing))}")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ledger", nargs="?", type=Path, default=LEDGER)
    args = parser.parse_args(argv)
    try:
        check_file(args.ledger)
    except (ValueError, OSError, KeyError) as exc:
        print(f"FAIL: {args.ledger}: {exc}", file=sys.stderr)
        return 1
    print("PASS: six BOM models, register maps, field layouts, guard/reset policy and conflict citations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
