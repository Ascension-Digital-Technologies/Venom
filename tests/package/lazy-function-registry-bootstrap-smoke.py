from pathlib import Path
import sys
root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
src = (root / 'src/compiler/pipeline/js_rewriting.cpp').read_text(encoding='utf-8')
assert 'std::string protected_registry_bootstrap_js(bool include_modules)' in src
assert 'registry << protected_registry_bootstrap_js(false);' in src
assert 'registry << protected_registry_bootstrap_js(true);' in src
assert "globalThis.__venomBridgeRevive=function" in src
assert "globalThis.__venomBridgeEncode=function" in src
print('lazy function registry bootstrap smoke: PASS')
