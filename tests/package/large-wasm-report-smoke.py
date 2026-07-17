from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
source = (root / "src/runtime/wasm_runtime.c").read_text(encoding="utf-8")
assert "#define RESULT_CAP (256u * 1024u)" in source
assert "static int g_result_overflow;" in source
assert "if (g_result_overflow) { result_reset(); return ERR_CAPACITY; }" in source
assert 'result_str(",\\\"domOps\\\":")' not in source
assert "VDOP0020" in source
print("large WASM report boundary: PASS")
