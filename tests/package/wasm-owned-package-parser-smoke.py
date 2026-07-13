from pathlib import Path
root=Path(__file__).resolve().parents[2]
c=(root/'src/runtime/wasm_runtime.c').read_text()
j=(root/'src/compiler/wasm_runtime_js.cpp').read_text()
r=(root/'src/compiler/runtime_js.cpp').read_text()
for token in ['venom_wasm_parse_package','venom_wasm_package_meta_ptr','validate_all_sections','serialize_package_metadata']:
    assert token in c, token
for token in ['installVenomWasmPackageParser','VPKG0002',"parserOwner: 'wasm'"]:
    assert token in j, token
assert 'if (venomPackageParser) return venomPackageParser(bytes, expectedPackageHash);' in r
print('wasm-owned package parser smoke: ok')
