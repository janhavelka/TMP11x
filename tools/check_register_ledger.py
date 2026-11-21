#!/usr/bin/env python3
"""Validate the TMP11x register ledger against its pinned sources and release gates."""

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs/registers/tmp11x.json"
PROFILES = {"TMP116", "TMP117", "TMP119"}


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
    elif name == "EEPROM_UL":
        expected = {"EUN": (0x8000, "R/W", 0), "BUSY": (0x4000, "R", 0),
                    "RESERVED": (0x3FFF, "R", 0)}
    elif name == "DEVICE_ID":
        if profile == "TMP116":
            expected = {"DID": (0xFFFF, "R", 0)}
        else:
            expected = {"REV": (0xF000, "R", 0), "DID": (0x0FFF, "R", 0)}
    else:
        if len(fields) != 1:
            raise ValueError(f"{where}: expected one full-word field")
        expected = {next(iter(fields)): (0xFFFF, access, 0 if readonly else 0xFFFF)}

    if fields.keys() != expected.keys():
        raise ValueError(f"{where}: expected fields {', '.join(expected)}")
    for field_name, layout in expected.items():
        field = fields[field_name]
        if (field["mask"], field["access"], field["nvm_mask"]) != layout:
            raise ValueError(f"{where}.{field_name}: incorrect mask, access or NVM domain")
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
    for topic in ("protocol", "startup"):
        section = profile.get(topic)
        if not isinstance(section, dict):
            raise ValueError(f"{name}: missing {topic} source record")
        pages(section, document, f"{name}.{topic}")
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
        for owner in items(reg, "owners", where):
            if type(owner) is not int or owner not in stages:
                raise ValueError(f"{where}: unknown owner stage {owner!r}")
        check_semantics(name, reg, check_fields(reg, where), where)
    if seen != expected.keys():
        missing = ", ".join(expected[p] for p in sorted(expected.keys() - seen))
        raise ValueError(f"{name}: missing registers {missing}")


def check_citation(citation, documents, where):
    text_fields(citation, ("profile", "claim"), where)
    if citation["profile"] not in documents:
        raise ValueError(f"{where}: unknown source profile {citation['profile']!r}")
    pages(citation, documents[citation["profile"]], where)


def check_file(path):
    ledger = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_keys)
    if (not isinstance(ledger, dict) or type(ledger.get("schema_version")) is not int
            or ledger["schema_version"] != 1):
        raise ValueError("expected schema_version 1")
    profiles = ledger.get("profiles")
    if not isinstance(profiles, dict) or profiles.keys() != PROFILES:
        raise ValueError("expected TMP116, TMP117 and TMP119 profiles")
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
        for source in sources:
            check_citation(source, documents, where)
        if len(sources) < 2 or all(s == sources[0] for s in sources):
            raise ValueError(f"{where}: expected paired source claims")
        if conflict["status"] == "resolved":
            check_citation(conflict.get("resolution_evidence"), documents, f"{where} resolution_evidence")
        elif conflict["status"] != "open":
            raise ValueError(f"{where}: expected open or resolved status")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ledger", nargs="?", type=Path, default=LEDGER)
    args = parser.parse_args(argv)
    try:
        check_file(args.ledger)
    except (ValueError, OSError, KeyError) as exc:
        print(f"FAIL: {args.ledger}: {exc}", file=sys.stderr)
        return 1
    print("PASS: six BOM models, register maps, field layouts, release gates and conflict citations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
