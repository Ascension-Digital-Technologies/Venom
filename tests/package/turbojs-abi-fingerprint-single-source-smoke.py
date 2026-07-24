#!/usr/bin/env python3
from pathlib import Path
import json, re, sys
root=Path(__file__).resolve().parents[2]
contract=json.loads((root/'contracts/turbojs-wasm-abi.json').read_text(encoding='utf-8'))
required=sorted(contract['requiredExports'])
prefix=f"VTJSABI{contract['runtimeAbi']}|VRDIV003|VRC3|"
h=0x811c9dc5
for byte in (prefix+'|'.join(required)).encode('utf-8'):
    h ^= byte
    h=(h*0x01000193)&0xffffffff
header=(root/'src/generated/contracts/turbojs_wasm_abi.hpp').read_text(encoding='utf-8')
match=re.search(r'release_abi_fingerprint\s*=\s*0x([0-9a-fA-F]+)u', header)
if not match or int(match.group(1),16)!=h:
    raise SystemExit('generated C++ ABI fingerprint does not match authoritative contract')
js=json.loads((root/'src/generated/runtime/javascript/turbojs_wasm_abi.js').read_text(encoding='utf-8').split('Object.freeze(',1)[1].rsplit(');',1)[0])
if js.get('releaseAbiFingerprint') != h:
    raise SystemExit('generated JavaScript ABI fingerprint does not match authoritative contract')
package=(root/'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8')
if 'turbojs_wasm_abi::release_abi_fingerprint' not in package:
    raise SystemExit('package writer does not consume generated ABI fingerprint')
engine=(root/'tools/generators/runtime/templates/turbojs_engine_module.suffix.cpp').read_text(encoding='utf-8')
if 'turbojs_wasm_abi::release_abi_fingerprint' not in engine:
    raise SystemExit('browser engine generator does not consume generated ABI fingerprint')
context=(root/'src/templates/turbojs-engine/20-context-execution.js').read_text(encoding='utf-8')
if 'RELEASE_ABI_FINGERPRINT' not in context or 'validateReleaseAbiExports(module);' not in context:
    raise SystemExit('browser runtime does not validate the actual WASM surface and package fingerprint')
if 'releaseAbiHash(canonicalNames)' in context:
    raise SystemExit('browser runtime still performs the fragile post-hardening ABI re-hash')
print(f'TurboJS ABI fingerprint single-source contract: PASS (0x{h:08x})')
