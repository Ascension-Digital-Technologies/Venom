from pathlib import Path
root=Path(__file__).resolve().parents[2]
engine=(root/'src/generated/runtime/quickjs_engine_module.cpp').read_text()
assert '#include "generated/contracts/quickjs_wasm_abi.hpp"' in engine
assert 'quickjs_wasm_abi::required_exports' in engine
assert 'quickjs_wasm_abi::allowed_toolchain_exports' in engine
assert 'QuickJS ABI export marker missing' in engine
print('QuickJS engine generated ABI binding: PASS')
