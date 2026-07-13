from pathlib import Path
root=Path(__file__).resolve().parents[2]
c=(root/'src/runtime/wasm_runtime.c').read_text()
js=(root/'src/compiler/wasm_runtime_js.cpp').read_text()
runtime=(root/'src/compiler/runtime_js.cpp').read_text()
for needle in ['venom_wasm_resolve_route','venom_wasm_route_record_ptr','VROUTE01','normalize_route_bytes','route_entry_full']:
    assert needle in c, needle
for needle in ['installVenomWasmRouteResolver','resolverOwner: \'wasm\'','venom_wasm_resolve_route','venom_wasm_route_record_size']:
    assert needle in js, needle
assert 'if (venomRouteResolver) return venomRouteResolver(pathname);' in runtime
assert 'v0.88.0' in (root/'docs/wasm-owned-route-module-resolution.md').read_text()
print('wasm-owned-route-resolution-smoke: ok')
