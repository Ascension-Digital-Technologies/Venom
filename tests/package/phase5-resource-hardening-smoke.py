#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
scaffold = (root/'src/runtime/turbojs_runtime_scaffold.c').read_text()
engine = (root/'src/generated/runtime/turbojs_engine_module.cpp').read_text()
abi = (root/'contracts/turbojs-wasm-abi.json').read_text()
cmake = (root/'CMakeLists.txt').read_text()
checks = {
  'v1.0.18+': project_version(cmake) >= (1, 0, 18),
  'interrupt handler': 'JS_SetInterruptHandler' in scaffold and 'venom_tjs_interrupt_handler' in scaffold,
  'pending job ceiling': 'pending_job_limit' in scaffold and 'pending job limit exceeded' in scaffold,
  'secure arena wipe': 'venom_tjs_secure_zero(g_source' in scaffold,
  'execution limits export': 'venom_tjs_context_set_execution_limits' in scaffold and 'venom_tjs_context_set_execution_limits' in abi,
  'bounded bridge records': 'boundedPush' in engine and 'maxExecutionRecords' in engine,
}
failed=[name for name, ok in checks.items() if not ok]
if failed:
  raise SystemExit('phase5 hardening smoke failed: '+', '.join(failed))
print('phase5 runtime resource hardening smoke passed')
