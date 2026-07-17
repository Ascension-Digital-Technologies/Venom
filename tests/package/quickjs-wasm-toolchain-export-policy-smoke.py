import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(sys.argv[1])
validator = root / 'tools' / 'wasm_exports.py'


def leb(value: int) -> bytes:
    out = bytearray()
    while True:
        b = value & 0x7F
        value >>= 7
        if value:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)


def name(value: str) -> bytes:
    raw = value.encode('utf-8')
    return leb(len(raw)) + raw


def fake_wasm(exports: list[str]) -> bytes:
    body = bytearray(leb(len(exports)))
    for export_name in exports:
        body += name(export_name)
        body.append(0)  # function export
        body += leb(0)
    return b'\x00asm\x01\x00\x00\x00' + bytes([7]) + leb(len(body)) + body

with tempfile.TemporaryDirectory() as tmp_name:
    tmp = Path(tmp_name)
    required = tmp / 'required.json'
    required.write_text(json.dumps(['venom_required']), encoding='utf-8')

    accepted = tmp / 'accepted.wasm'
    accepted.write_bytes(fake_wasm([
        'venom_required',
        '_initialize',
        '__cxa_increment_exception_refcount',
    ]))
    accepted_report = tmp / 'accepted.json'
    proc = subprocess.run([
        sys.executable, str(validator), str(accepted),
        '--require-json', str(required), '--exact-exports',
        '--exports-json', str(accepted_report), '--fail-missing',
    ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if proc.returncode != 0:
        raise SystemExit('known Emscripten support exports should pass exact verification:\n' + proc.stdout)
    info = json.loads(accepted_report.read_text(encoding='utf-8'))
    if info['missing_exports'] or info['unexpected_exports']:
        raise SystemExit(f'accepted support exports were misclassified: {info}')
    if not info['required_exports_satisfied'] or not info['exact_exports_satisfied']:
        raise SystemExit(f'accepted export report should satisfy both policies: {info}')

    rejected = tmp / 'rejected.wasm'
    rejected.write_bytes(fake_wasm(['venom_required', 'unknown_toolchain_export']))
    rejected_report = tmp / 'rejected.json'
    proc = subprocess.run([
        sys.executable, str(validator), str(rejected),
        '--require-json', str(required), '--exact-exports',
        '--exports-json', str(rejected_report), '--fail-missing',
    ], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if proc.returncode != 2:
        raise SystemExit('unknown export should fail exact verification:\n' + proc.stdout)
    info = json.loads(rejected_report.read_text(encoding='utf-8'))
    if info['missing_exports']:
        raise SystemExit(f'unexpected export must not be reported as missing: {info}')
    if info['unexpected_exports'] != ['unknown_toolchain_export']:
        raise SystemExit(f'unknown export classification mismatch: {info}')

text = validator.read_text(encoding='utf-8')
if "missing.extend('unexpected:'" in text:
    raise SystemExit('unexpected exports must not be merged into missing ABI exports')

print('quickjs wasm toolchain export policy smoke: PASS')
