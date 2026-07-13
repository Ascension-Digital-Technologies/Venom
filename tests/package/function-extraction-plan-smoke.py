from pathlib import Path
root=Path(__file__).resolve().parents[2]
js=(root/'src/compiler/js.cpp').read_text(encoding='utf-8')
build=(root/'src/compiler/build.cpp').read_text(encoding='utf-8')
for marker in ['VENOM_FUNCTION_EXTRACTION_PLAN_V1','bridge-candidate','async-json-v1','synchronous cross-realm result bridge unavailable']:
    assert marker in js, marker
for marker in ['function-extraction-plan.txt','function-extraction-plan.json','realm-bridge-contract.json']:
    assert marker in build, marker
print('function extraction plan smoke: PASS')
