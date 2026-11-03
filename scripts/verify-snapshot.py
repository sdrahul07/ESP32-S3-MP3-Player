"""Verify archived file bytes; no dependency installs, builds or uploads."""
from pathlib import Path
import hashlib
import json
import sys

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "snapshots/SHA256.json").read_text(encoding="utf-8"))
failures = []
for relative, entry in manifest.items():
    path = root / relative
    if not path.is_file():
        failures.append(relative + ": missing")
    elif hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
        failures.append(relative + ": modified")
if failures:
    print("\n".join(failures))
    sys.exit(1)
print(f"Verified {len(manifest)} preserved files (SHA-256).")
