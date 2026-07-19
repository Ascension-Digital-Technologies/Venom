from pathlib import Path
root=Path(__file__).resolve().parents[2]
js=(root/'src/pipeline/js.cpp').read_text(encoding='utf-8') + (root/'src/pipeline/js_planning.cpp').read_text(encoding='utf-8')
js += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
build=(root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
for marker in ['VENOM_FUNCTION_EXTRACTION_PLAN_V1','bridge-candidate','async-json-v1','synchronous cross-runtime result bridge unavailable']:
    assert marker in js, marker
for marker in ['function-extraction-plan.txt','function-extraction-plan.json','runtime-bridge-contract.json']:
    assert marker in build, marker
print('function extraction plan smoke: PASS')
