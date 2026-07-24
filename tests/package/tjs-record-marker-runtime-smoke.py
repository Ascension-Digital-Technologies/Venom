from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
scaffold=(root/'src/runtime/turbojs_runtime_scaffold.c').read_text()
worker=(root/'src/generated/runtime/worker_runtime_template.hpp').read_text()
engine=(root/'src/templates/turbojs-engine/20-context-execution.js').read_text()
checks={
 'bytecode-validator-uses-tjs': "record[1] == 'T'" in scaffold,
 'module-validator-uses-tjs': "g_source[1] == 'T'" in scaffold,
 'worker-validates-before-execute': 'prepareTurboJsRecordForExecution' in worker and 'protected registry chunk bytecode validation failed' in worker,
 'worker-legacy-runtime-retry': 'view[1] = 0x51' in worker,
 'engine-legacy-runtime-retry': 'runtimeBytes[1] = 0x51' in engine,
}
failed=[name for name,ok in checks.items() if not ok]
if failed: raise SystemExit('TJS record marker runtime smoke failed: '+', '.join(failed))
print('TJS record marker runtime smoke: PASS')
