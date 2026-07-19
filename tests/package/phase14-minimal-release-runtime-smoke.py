#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
build = (root / "src/pipeline/build.cpp").read_text(encoding="utf-8") + (root / "src/pipeline/build_package_metadata.cpp").read_text(encoding="utf-8") + (root / "src/pipeline/build_runtime_metadata.cpp").read_text(encoding="utf-8") + (root / "src/pipeline/build_runtime_audit_metadata.cpp").read_text(encoding="utf-8") + (root / "src/pipeline/build_runtime_module_metadata.cpp").read_text(encoding="utf-8")
engine = (root / "src/generated/runtime/quickjs_engine_module.cpp").read_text(encoding="utf-8")
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")

assert "minify_release_js" in build
assert "opaque_names" in build
assert "protected QuickJS module surface marker missing" in engine
assert "minimal_surface" in engine
assert "new Function" in build  # protected validation still rejects it
m = re.search(r"VERSION\s+(\d+)\.(\d+)\.(\d+)", cmake)
assert m and tuple(map(int, m.groups())) >= (1, 0, 32)
print("phase14 minimal release runtime smoke passed")
