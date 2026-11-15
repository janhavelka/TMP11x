#!/usr/bin/env python3
"""Check the pinned vendor reference archive offline (Python standard library)."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/reference/manifest.json"


def load_documents(manifest=MANIFEST, root=ROOT):
    """Validate metadata and resolve paths strictly inside the source archive."""
    data = json.loads(manifest.read_text(encoding="utf-8"))
    if (not isinstance(data, dict) or type(data.get("schema_version")) is not int
            or data["schema_version"] != 1
            or not isinstance(data.get("documents"), list)
            or not data["documents"]):
        raise ValueError("expected schema_version 1 and nonempty documents")
    source = (root / "docs/reference/source").resolve()
    seen_ids, seen_paths = set(), set()
    documents = []
    for item in data["documents"]:
        if not isinstance(item, dict):
            raise ValueError("each document must be an object")
        for field in ("id", "title", "document_id", "revision", "source_url",
                      "resolved_url", "retrieved_date", "path", "sha256",
                      "size_bytes", "page_count", "notice", "role"):
            if field not in item or item[field] in (None, ""):
                raise ValueError(f"missing {field}: {item.get('id', '?')}")
        for field in ("id", "title", "document_id", "revision", "source_url",
                      "resolved_url", "retrieved_date", "path", "sha256",
                      "notice", "role"):
            if not isinstance(item[field], str):
                raise ValueError(f"{field} must be a string")
        for field in ("size_bytes", "page_count"):
            if type(item[field]) is not int or item[field] <= 0:
                raise ValueError(f"{field} must be a positive integer: {item['id']}")
        if not item["path"].startswith("docs/reference/source/") or "\\" in item["path"]:
            raise ValueError(f"expected repository-relative archive path: {item['path']}")
        path = (root / item["path"]).resolve()
        if not path.is_relative_to(source) or path.suffix.lower() != ".pdf":
            raise ValueError(f"path outside PDF archive: {item['path']}")
        if item["id"] in seen_ids or path in seen_paths:
            raise ValueError(f"duplicate reference: {item['id']}")
        if not re.fullmatch(r"[a-f0-9]{64}", item["sha256"]):
            raise ValueError(f"invalid SHA-256: {item['id']}")
        if not item["source_url"].startswith("https://www.ti.com/"):
            raise ValueError(f"unexpected source host: {item['id']}")
        reviewed = item.get("visually_reviewed_pdf_pages", [])
        if not isinstance(reviewed, list):
            raise ValueError(f"reviewed pages must be a list: {item['id']}")
        for page in reviewed:
            if type(page) is not int or not 1 <= page <= item["page_count"]:
                raise ValueError(f"invalid reviewed PDF page: {item['id']}")
        seen_ids.add(item["id"])
        seen_paths.add(path)
        documents.append((item, path))
    return documents


def verify_bytes(item, data):
    """Reject HTML error pages, incomplete transfers and changed vendor bytes."""
    if not data.startswith(b"%PDF-"):
        raise ValueError(f"{item['id']}: missing PDF signature")
    if len(data) != item["size_bytes"]:
        raise ValueError(f"{item['id']}: size mismatch")
    if hashlib.sha256(data).hexdigest() != item["sha256"]:
        raise ValueError(f"{item['id']}: SHA-256 mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    try:
        documents = load_documents()
        for item, path in documents:
            verify_bytes(item, path.read_bytes())
        print(f"PASS: {len(documents)} pinned PDFs; paths, sizes and SHA-256 verified")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
