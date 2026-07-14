#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[2]
runtime=(root/'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
engine=(root/'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')
required_runtime=["bytecodeTrustHandoff", "producer: 'package-decoder-wasm'", "consumer: 'quickjs-execution-wasm'", "bindingHash: fnv1a32"]
required_engine=["validateBytecodeTrustHandoff", "QuickJS trust-domain handoff integrity mismatch", "validateBytecodeTrustHandoff(chunk)"]
missing=[x for x in required_runtime if x not in runtime]+[x for x in required_engine if x not in engine]
if missing:
    print('split trust-domain smoke: FAIL missing '+', '.join(missing), file=sys.stderr); sys.exit(1)
print('split trust-domain smoke: PASS')
