from pathlib import Path
root = Path(__file__).resolve().parents[2]
js = (root / 'src/compiler/pipeline/js.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/js_planning.cpp').read_text(encoding='utf-8')
js += (root / 'src/compiler/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
build = (root / 'src/compiler/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
for marker in [
    'VENOM_FUNCTION_PLAN_V1',
    'explicit function-level browser directive',
    'explicit function-level protected directive',
    'whole-file promotion preserves shared lexical state',
    'conservative-whole-file-promotion',
    'function-level browser directive conflicts with file-scope protected directive',
]:
    if marker not in js:
        raise SystemExit(f'missing function realm planner marker: {marker}')
for marker in ['function-plan.txt', 'function-plan.json']:
    if marker not in build:
        raise SystemExit(f'missing function realm report emission: {marker}')
print('function realm plan smoke passed')
