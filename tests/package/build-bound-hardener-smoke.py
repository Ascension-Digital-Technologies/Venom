from pathlib import Path
import sys

root = Path(sys.argv[1])
support = (root / 'src/pipeline/build_support.cpp').read_text()
header = (root / 'include/venom/internal/pipeline/build_support.hpp').read_text()
build = (root / 'src/pipeline/build.cpp').read_text()
js = (root / 'src/pipeline/js.cpp').read_text()
encoding = (root / 'src/pipeline/js_bundle_encoding.cpp').read_text()
hardener = (root / 'src/pipeline/native_js_hardener.cpp').read_text()

assert 'const std::string& build_salt' in header
assert 'hardener-v5|' in support
assert 'venom-hardener-cache-v5|' in support
assert 'poly.seed_commitment' in build
assert "seed.toString(36)" in hardener
assert 'opaque_bytecode_label' in encoding
assert 'protected-registry-chunk' in js
assert 'ast_harden_release_js' in js
print('build-bound hardener source checks passed')
