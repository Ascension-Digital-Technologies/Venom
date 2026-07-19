#!/usr/bin/env python3
from pathlib import Path
import json, py_compile, sys
root=Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tools"))
from venom_tools.examples import load_example_registry
contract=json.loads((root/'contracts/browser-certification.json').read_text(encoding='utf-8'))
assert contract['schema']=='VENOM_BROWSER_CERTIFICATION_V2'
assert contract['browsers']==['chromium','firefox','webkit']
examples=load_example_registry(root).certifiable()
assert {x['id'] for x in contract['examples']} == {x.id for x in examples}
loader=(root/'src/pipeline/js.cpp').read_text(encoding='utf-8')
for token in ['__venomBootStatus','venom:boot-ready','venom:boot-error',"'ready','complete'","'error','application'"]:
    assert token in loader,token
browser=(root/'tests/browser/venom_examples_e2e.py').read_text(encoding='utf-8')
for token in ["VENOM_BROWSER_CERTIFICATION_RESULT_V2",'__venomBootStatus',"encode('utf-8')",'東京','verify-runtime','boot timeline missing phases','boot-timeline']:
    assert token in browser,token
py_compile.compile(str(root/'tests/browser/venom_examples_e2e.py'),doraise=True)
print('browser certification v2 smoke passed')
