#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
subprocess.run([sys.executable, root / "tools/check_example_registry.py", root], cwd=root, check=True)
subprocess.run([sys.executable, root / "tools/generate_example_launchers.py", "--repo-root", root, "--check"], cwd=root, check=True)
print("example registry smoke passed")
