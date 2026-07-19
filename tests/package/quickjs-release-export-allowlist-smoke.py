#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
authored = (root / "src/templates/quickjs-engine/00-module-contract.js").read_text(encoding="utf-8")
worker = (root / "src/generated/include/venom/generated/runtime/worker_runtime_template.hpp").read_text(encoding="utf-8")
generated = (root / "src/generated/runtime/quickjs_engine_module.cpp").read_text(encoding="utf-8")
for name in ("__cxa_increment_exception_refcount", "_initialize", "__indirect_function_table", "_emscripten_stack_restore", "emscripten_stack_get_current"):
    for label, source in (("authored engine", authored), ("worker", worker), ("generated engine", generated)):
        assert name in source, f"{name} missing from {label} release export allowlist"
assert "entry.kind" in authored and "kindMismatches" in authored
assert "entry.kind" in worker and "kindMismatches" in worker
print("quickjs release export allowlist smoke: PASS")
