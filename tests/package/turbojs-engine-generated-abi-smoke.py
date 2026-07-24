from pathlib import Path
root=Path(__file__).resolve().parents[2]
engine=(root/'src/generated/runtime/turbojs_engine_module.cpp').read_text()
assert '#include "generated/contracts/turbojs_wasm_abi.hpp"' in engine
assert 'turbojs_wasm_abi::required_exports' in engine
assert 'turbojs_wasm_abi::allowed_toolchain_exports' in engine
assert 'TurboJS ABI export marker missing' in engine
print('TurboJS engine generated ABI binding: PASS')
