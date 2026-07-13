#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
runtime = (root / 'src/compiler/runtime_js.cpp').read_text(encoding='utf-8')
wasm = (root / 'src/runtime/wasm_runtime.c').read_text(encoding='utf-8')
checks = {
    'single active route runtime': 'let activeRuntime = null;' in runtime and 'disposeActive();' in runtime,
    'strict lazy route isolation': "protected release requires route-lazy sections" in runtime,
    'owned route section copies': 'cloneSectionForRoute' in runtime and '__venomOwnedData' in runtime,
    'route data zeroization': 'secureWipeView' in runtime and 'wipeRouteRuntime' in runtime,
    'route generation on chunks': 'routeGeneration' in runtime and 'routeGeneration }))' in runtime,
    'navigation teardown': "bridge.teardownRoute('navigation')" in runtime and 'routeRuntimeLoader.dispose();' in runtime,
    'async route reset': 'resetRoute(reason' in runtime and 'clearRouteResources' in runtime,
    'quickjs route context destruction': 'function destroyRoute(route)' in runtime,
    'wasm materialized wipe': 'venom_wasm_secure_zero' in wasm and 'venom_wasm_release_materialized' in wasm,
    'wasm release operation': 'case 5u: venom_wasm_release_materialized(); return ERR_OK;' in wasm,
    'wasm transient wipe': 'venom_wasm_release_transient' in wasm and 'case 6u: venom_wasm_release_transient(); return ERR_OK;' in wasm,
    'decoder finally wipe': 'finally {' in (root / 'src/compiler/wasm_runtime_js.cpp').read_text(encoding='utf-8') and 'e.v12_x(5, 0, 0, 0, 0);' in (root / 'src/compiler/wasm_runtime_js.cpp').read_text(encoding='utf-8'),
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('phase9 route isolation checks failed: ' + ', '.join(failed))
print('phase9 route isolation smoke: ok')
