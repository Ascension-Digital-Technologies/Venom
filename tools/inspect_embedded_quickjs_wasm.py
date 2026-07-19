#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, re, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from wasm_exports import wasm_exports

def embedded_bytes(header: Path) -> bytes:
    text=header.read_text(encoding='utf-8', errors='replace')
    start=text.find('kQuickJsRuntimeWasmBlob[]')
    if start < 0: raise RuntimeError('embedded QuickJS/WASM byte array not found')
    end=text.find('};', start)
    if end < 0: raise RuntimeError('embedded QuickJS/WASM byte array is unterminated')
    return bytes(int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})', text[start:end]))

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--format', choices=['text','json'], default='text')
    args=ap.parse_args(); root=args.repo_root.resolve()
    contract=json.loads((root/'contracts/quickjs-wasm-abi.json').read_text())
    data=embedded_bytes(root/'src/generated/include/venom/generated/runtime/quickjs_runtime_wasm_blob.hpp')
    exports=wasm_exports(data); missing=[x for x in contract['requiredExports'] if x not in exports]
    report={'schema':'venom.quickjs-wasm-export-check.v1','runtimeAbi':contract['runtimeAbi'],'bridgeAbi':contract['bridgeAbi'],'bytecodeFormat':contract['bytecodeFormat'],'wasmBytes':len(data),'exports':exports,'missing':missing,'passed':not missing}
    if args.format=='json': print(json.dumps(report, indent=2))
    else:
        print(f"QuickJS/WASM ABI {contract['runtimeAbi']} / bridge {contract['bridgeAbi']}")
        print(f"WASM bytes: {len(data)}; exports: {len(exports)}")
        print('ABI contract: PASS' if not missing else 'ABI contract: FAIL')
        for name in missing: print(f'  missing: {name}')
    raise SystemExit(0 if report['passed'] else 1)
if __name__=='__main__': main()
