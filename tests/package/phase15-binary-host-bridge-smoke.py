#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
s=(root/'src/generated/runtime/runtime_js.cpp').read_text()
s += (root / 'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
for x in ['function createDomMutationBatch(','new Uint32Array(','OP_SET_ATTRIBUTE = 1','OP_APPEND_CHILD = 2','OP_APPEND_TEXT = 3','DOM batch string budget exceeded','domBatch.queueSetAttribute(','domBatch.queueAppendChild(','domBatch.queueAppendText(','domBatch.flush();','data-venom-dom-batches']:
    assert x in s, x
assert re.search(r'VERSION\s+1\.(?:[1-9][0-9]|[2-9][0-9])\.[0-9]+\b',(root/'CMakeLists.txt').read_text())
print('phase15 binary host bridge smoke passed')
