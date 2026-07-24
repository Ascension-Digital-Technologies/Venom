#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[2])
js_cpp = ((root / 'src/pipeline/js.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/js_bundle_encoding.cpp').read_text(encoding='utf-8'))
runtime = (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
metadata = (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
security = (root / 'src/pipeline/security_analysis.cpp').read_text(encoding='utf-8')

required_cpp = [
    'VTJSE006', 'wrap_turbojs_record', 'envelope_seed', 'xorshift32',
    'bytecode_lane_map', 'bytecode_lane_fingerprint', 'lane_width = 16u',
    'diversification_salt', 'fnv1a64(raw)', '0x01000300u'
]
for marker in required_cpp:
    assert marker in js_cpp, f'missing bytecode envelope compiler marker: {marker}'
required_runtime = [
    'decodeTurboJsEnvelope', "asciiOf(bytes.slice(0, 8)) !== 'VTJSE006'",
    'turboJsEnvelopeLaneMap', 'turboJsEnvelopeLaneFingerprint',
    'expectedLaneFingerprint', 'laneWidth !== 16',
    'bytecodeAbi !== 0x01000300', 'fnv1a64(decoded) !== expectedHash',
    "storedCodeBytes.fill(0)", "bytecodeMagic !== 'VTJSMB04'"
]
for marker in required_runtime:
    assert marker in runtime, f'missing bytecode envelope runtime marker: {marker}'
for marker in [
    'package_envelope=build-specific-turbojs-envelope-v2',
    'package_envelope_magic=VTJSE006',
    'package_envelope_permutation=per-build-16-byte-lane-map'
]:
    assert marker in metadata, f'missing bytecode envelope metadata: {marker}'
assert 'payload_starts_with(section.data, "VTJSE006")' in security
assert 'count_js_bundle_flagged_payload_prefix' in security
assert 'contains_bytes(section.data, "VTJSBC03")' not in security

MASK = 0xffffffff

def xs(state: int) -> int:
    state ^= (state << 13) & MASK
    state ^= state >> 17
    state ^= (state << 5) & MASK
    return state & MASK

def seed(salt: str, route: str, source: str, order: int) -> int:
    h = 2166136261
    for b in (salt + route + source).encode():
        h ^= b
        h = (h * 16777619) & MASK
    for shift in range(0, 32, 8):
        h ^= (order >> shift) & 0xff
        h = (h * 16777619) & MASK
    h ^= 0xA5C31F27
    return h or 0x6D2B79F5

def lane_map(value: int) -> list[int]:
    out = list(range(16))
    state = value ^ 0xC6EF3720
    for i in range(15, 0, -1):
        state = xs(state)
        j = state % (i + 1)
        out[i], out[j] = out[j], out[i]
    return out

def transform(raw: bytes, value: int) -> bytes:
    lanes = lane_map(value)
    state = value ^ 0x9E3779B9
    out = bytearray()
    for block in range(0, len(raw), 16):
        size = min(16, len(raw) - block)
        block_map = [lane for lane in lanes if lane < size]
        for stored_lane in range(size):
            pos = block + stored_lane
            if pos & 3 == 0:
                state = xs(state)
            byte = raw[block + block_map[stored_lane]]
            out.append(byte ^ ((state >> ((pos & 3) * 8)) & 0xff))
    return bytes(out)

def inverse(encoded: bytes, value: int) -> bytes:
    lanes = lane_map(value)
    state = value ^ 0x9E3779B9
    out = bytearray(len(encoded))
    for block in range(0, len(encoded), 16):
        size = min(16, len(encoded) - block)
        block_map = [lane for lane in lanes if lane < size]
        for stored_lane in range(size):
            pos = block + stored_lane
            if pos & 3 == 0:
                state = xs(state)
            byte = encoded[pos] ^ ((state >> ((pos & 3) * 8)) & 0xff)
            out[block + block_map[stored_lane]] = byte
    return bytes(out)

raw = b'VTJSBC03' + bytes(range(73))
a = seed('build-a', '/', 'protected.js', 1)
b = seed('build-b', '/', 'protected.js', 1)
assert a != b and lane_map(a) != lane_map(b)
encoded = transform(raw, a)
assert encoded != raw and inverse(encoded, a) == raw
assert transform(raw, a) != transform(raw, b)
assert inverse(encoded, b) != raw
print('build-specific bytecode envelope smoke: PASS')
