#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tools"))
from venom_tools.examples import load_example_registry

cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
match = re.search(r"project\(venom\s+VERSION\s+(\d+\.\d+\.\d+)", cmake, re.S)
if not match:
    raise SystemExit("project version missing")
version = match.group(1)
registry = load_example_registry(root)
semantic_examples = {f"build-and-launch-{spec.launcher}" for spec in registry.examples}
windows = {path.stem for path in (root / "scripts/windows").glob("*.bat")}
linux = {path.stem for path in (root / "scripts/linux").glob("*.sh")}
example_dirs = {path.name for path in (root / "examples").iterdir() if path.is_dir()}

required = {
    "generated version template": 'VENOM_VERSION_STRING "@PROJECT_VERSION@"' in (root / "cmake/templates/version.hpp.in").read_text(encoding="utf-8"),
    "CLI uses generated version": "VENOM_VERSION_STRING" in (root / "src/cli/cli.cpp").read_text(encoding="utf-8"),
    "README current": (root / "README.md").read_text(encoding="utf-8").startswith("# Venom Secure Web Runtime"),
    "semantic Windows launchers": semantic_examples <= windows,
    "semantic Linux launchers": semantic_examples <= linux,
    "launcher platform parity": semantic_examples <= (windows & linux),
    "no numbered Windows launchers": not any(name.startswith("build-and-launch-example") for name in windows),
    "no numbered Linux launchers": not any(name.startswith("build-and-launch-example") for name in linux),
    "canonical Emscripten controller": (root / "tools/build_emscripten.py").is_file(),
    "production leak gate present": (root / "tools/check_production_leaks.py").is_file(),
    "semantic example launcher helper": (root / "tools/launch_example.py").is_file(),
    "canonical example registry": (root / "contracts/examples.json").is_file(),
    "launcher generator": (root / "tools/generate_example_launchers.py").is_file(),
    "shared Python tooling package": (root / "tools/venom_tools/examples.py").is_file(),
    "root integrations folder removed": not (root / "integrations").exists(),
    "root fuzz folder removed": not (root / "fuzz").exists(),
    "Vite package consolidated": (root / "packages/vite/package.json").is_file(),
    "fuzz targets colocated with tests": (root / "tests/fuzz/targets/package_parser_fuzz.cpp").is_file(),
    "registry covers examples": {spec.directory for spec in registry.examples} == example_dirs,
    "typed core error API": (root / "src/base/include/venom/base/error.hpp").is_file(),
}
failed = [name for name, ok in required.items() if not ok]
if failed:
    raise SystemExit("repository consistency failed: " + ", ".join(failed))
print(f"repository consistency smoke passed: {version}; {len(registry.examples)} examples")
