#!/usr/bin/env python3
"""Run the version generator's read-only check."""

import subprocess
import sys
from pathlib import Path


if __name__ == "__main__":
    generator = Path(__file__).resolve().parents[1] / "scripts/generate_version.py"
    raise SystemExit(subprocess.run([sys.executable, str(generator), "check"], check=False).returncode)
