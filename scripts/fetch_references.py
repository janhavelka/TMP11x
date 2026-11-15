#!/usr/bin/env python3
"""Restore missing pinned TI PDFs; never overwrite files or accept changed bytes."""

import argparse
from urllib.request import Request, urlopen
import sys

from check_references import load_documents, verify_bytes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ids", nargs="*", help="manifest document IDs; default: all")
    args = parser.parse_args()
    try:
        documents = load_documents()
        available = {item["id"] for item, _ in documents}
        unknown = set(args.ids) - available
        if unknown:
            raise ValueError(f"unknown document IDs: {', '.join(sorted(unknown))}")
        for item, path in documents:
            if args.ids and item["id"] not in args.ids:
                continue
            if path.exists():
                verify_bytes(item, path.read_bytes())
                print(f"VERIFIED {item['id']} (already present)")
                continue
            request = Request(item["source_url"], headers={
                "User-Agent": "TMP11x-reference-intake/1.0"})
            with urlopen(request, timeout=45) as response:
                # A changed response cannot consume unbounded memory.
                data = response.read(item["size_bytes"] + 1)
            verify_bytes(item, data)
            path.parent.mkdir(parents=True, exist_ok=True)
            # Exclusive creation also protects a concurrent user's download.
            with path.open("xb") as output:
                output.write(data)
            print(f"RESTORED {item['id']}")
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        print("Preserve changed vendor files separately and review provenance; "
              "this command never updates manifest hashes.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
