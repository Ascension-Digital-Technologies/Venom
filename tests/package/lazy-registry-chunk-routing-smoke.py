from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
js=(root/'src/compiler/pipeline/js.cpp').read_text()
build=(root/'src/compiler/pipeline/build.cpp').read_text()
worker=(root/'src/generated/runtime/worker_runtime_js.cpp').read_text()
checks={
 'ir-v12':'compiler-v12' in build,
 'manifest-v2':'candidate-chunk-activation' in js and '\\"chunk\\"' in js,
 'grouped-emission':'registry-" + chunk.id' in build,
 'candidate-routing':'lazyRegistryByCandidate' in worker,
 'per-chunk-state':'lazyRegistryReady = new Set()' in worker and 'lazyRegistryPromises = new Map()' in worker,
 'selective-preload':'filter((chunk) => chunk.preload)' in worker,
}
failed=[k for k,v in checks.items() if not v]
if failed: raise SystemExit('lazy registry chunk routing smoke failed: '+', '.join(failed))
print('lazy registry chunk routing smoke: PASS')
