#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
js = (root / "src/pipeline/js.cpp").read_text(encoding="utf-8")
js += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
worker = (root / "src/generated/runtime/worker_runtime_js.cpp").read_text(encoding="utf-8")
bootstrap = js + "\n" + worker
runtime = (root / "src/generated/runtime/runtime_js.cpp").read_text(encoding="utf-8")
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
engine = (root / "src/generated/runtime/turbojs_engine_module.cpp").read_text(encoding="utf-8")
assert "expectedPackageSha256" in js
assert "package SHA-256 attestation mismatch" in js
assert "const [packageBytes, wasmBytes] = await Promise.all" in bootstrap
assert "const [actualPackageSha256, actualTurboJsWasmSha256] = await Promise.all" in bootstrap
assert "cache: 'force-cache'" in bootstrap
assert "cache: 'force-cache'" in runtime
assert "cache: 'force-cache'" in engine
cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
m = re.search(r"VERSION\s+(\d+)\.(\d+)\.(\d+)", cmake)
assert m and tuple(map(int, m.groups())) >= (1, 0, 31)
print("phase13 release bootstrap optimization smoke passed")
