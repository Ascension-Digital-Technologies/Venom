from pathlib import Path
import sys

root = Path(sys.argv[1])
support = (root / 'src/compiler/pipeline/build_support.cpp').read_text()
header = (root / 'src/compiler/pipeline/build_support.hpp').read_text()
build = (root / 'src/compiler/pipeline/build.cpp').read_text()
js = (root / 'src/compiler/pipeline/js.cpp').read_text()
hardener = (root / 'src/compiler/pipeline/native_js_hardener.cpp').read_text()

assert 'const std::string& build_salt' in header
assert 'hardener-v2|' in support
assert 'venom-hardener-cache-v2|' in support
assert 'poly.seed_commitment' in build
assert "seed.toString(36)" in hardener
assert 'opaque_bytecode_label' in js
assert 'protected-registry-chunk' in js
assert 'ast_harden_release_js' in js
print('build-bound hardener source checks passed')
