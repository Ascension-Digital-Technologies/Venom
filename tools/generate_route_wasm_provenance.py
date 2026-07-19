#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, re, sys
from pathlib import Path

REQUIRED_EXPORTS = [
    'memory', 'venom_runtime_abi', 'venom_package_version',
    'v12_p', 'v12_n', 'v12_x',
]

def embedded_bytes(header: Path) -> bytes:
    text = header.read_text(encoding='utf-8')
    data = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', text))
    if not data.startswith(b'\0asm'):
        raise ValueError('embedded route runtime is not a WebAssembly module')
    return data

def exports(data: bytes) -> list[str]:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from wasm_exports import wasm_exports
    return sorted(set(wasm_exports(data)))

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    root = args.repo_root.resolve()
    header = root / 'src/generated/include/venom/generated/runtime/wasm_runtime_blob.hpp'
    source = root / 'src/runtime/wasm_runtime.c'
    out = root / 'src/generated/runtime/metadata/wasm_runtime_provenance.json'
    data = embedded_bytes(header)
    exp = exports(data)
    missing = [x for x in REQUIRED_EXPORTS if x not in exp]
    provenance = {
        'schema': 'venom.route-wasm-provenance.v1',
        'artifact_kind': 'real-route-vm',
        'runtime_implementation': 'venom-route-vm-wasm',
        'scaffold_runtime': False,
        'required_exports_satisfied': not missing,
        'missing_required_exports': missing,
        'instruction_contract': 'VPOL0010-v2-fixed16',
        'dom_command_contract': 'VDOP0020',
        'artifact_size': len(data),
        'artifact_sha256': hashlib.sha256(data).hexdigest(),
        'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'exports': exp,
        'source': 'src/runtime/wasm_runtime.c',
        'embedded_header': 'src/generated/include/venom/generated/runtime/wasm_runtime_blob.hpp',
    }
    rendered = json.dumps(provenance, indent=2, sort_keys=True) + '\n'
    if args.check:
        if not out.is_file() or out.read_text(encoding='utf-8') != rendered:
            print('route WASM provenance is stale; run tools/generate_route_wasm_provenance.py', file=sys.stderr)
            return 1
        if missing:
            print('route WASM required exports missing: ' + ', '.join(missing), file=sys.stderr)
            return 1
        print('route WASM provenance check: PASS')
        return 0
    out.write_text(rendered, encoding='utf-8')
    print(out)
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
