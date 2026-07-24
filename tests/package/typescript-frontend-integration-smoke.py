#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
cpp = (root / 'src/frontends/typescript/frontend.cpp').read_text()
runtime = (root / 'src/frontends/typescript/typescript_runtime.cpp').read_text()
hpp = (root / 'src/frontends/typescript/frontend.hpp').read_text()
assert 'transpile_embedded' in cpp
assert 'EmbeddedFrontendService' in cpp
assert 'run_process' not in cpp
assert 'VENOM_NODE' not in cpp
assert 'typescript_structural_frontend.js' not in cpp
assert '__venomTranspileTypeScript' in runtime
assert 'typescript_compiler_asset.hpp' in runtime
assert 'compiler_source_sha256' in runtime
assert 'typescript_bridge_bytecode.hpp' in runtime
assert 'bridge_bytecode_sha256' in runtime
assert 'VENOM_TYPESCRIPT_BRIDGE' in runtime
assert 'third_party' not in runtime
assert 'VENOM_SOURCE_ROOT' not in runtime
assert 'source_map' in hpp
assert 'frontend_version' in hpp
assert (root / 'third_party/typescript/LICENSE.txt').is_file()
assert (root / 'src/generated/compiler/typescript_compiler_asset.cpp').is_file()
assert (root / 'tools/generators/typescript/embed_typescript_compiler.py').is_file()
assert (root / 'src/generated/compiler/typescript_bridge_bytecode.cpp').is_file()
assert (root / 'src/templates/typescript/bridge.js').is_file()
assert (root / 'tools/generators/typescript/compile_typescript_bridge_bytecode.cpp').is_file()
assert (root / 'tools/generators/typescript/embed_typescript_bridge_bytecode.py').is_file()
assert not (root / 'tools/typescript_structural_frontend.js').exists()
print('Node-free TypeScript frontend integration smoke passed')
