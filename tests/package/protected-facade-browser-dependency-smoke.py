#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
source=(root/'src/compiler/pipeline/js_rewriting.cpp').read_text(encoding='utf-8')
needle='chunk.flags |= JsChunkBrowser | JsChunkModule | JsChunkDependency;'
assert needle in source, 'generated protected facades must remain browser module dependencies'
print('protected facade browser dependency smoke: PASS')
