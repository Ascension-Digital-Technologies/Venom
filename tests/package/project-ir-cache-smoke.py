#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
checks = {
    root / 'src/compiler/core/project_ir.hpp': ['protected_modules', 'module_edges', 'protected_contracts_json', 'plan_fingerprint'],
    root / 'src/compiler/core/project_ir.cpp': ['enrich_project_ir', 'project_ir_summary', 'venom-project-plan:'],
    root / 'src/compiler/pipeline/build_support.cpp': ['venom-hardener-cache-v1', 'hardener hit:', 'hardener miss:'],
    root / 'src/quickjs/bytecode.cpp': ['venom-qjs-bytecode-cache-v1', 'QuickJS bytecode hit:', 'QuickJS bytecode miss:'],
    root / 'src/compiler/pipeline/build.cpp': ['compiler-v9', 'configure_bytecode_cache'],
    root / 'src/compiler/commands/cli.cpp': ['--no-cache', '--cache-dir'],
}
for path, markers in checks.items():
    text = path.read_text(encoding='utf-8')
    missing = [marker for marker in markers if marker not in text]
    if missing:
        raise SystemExit(f'{path}: missing {missing}')
print('project IR and compiler cache smoke passed')
