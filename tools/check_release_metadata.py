#!/usr/bin/env python3
"""Clean-room release metadata leak checker for a Venom VBC artifact."""
from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

MASK = (1 << 64) - 1
FNV_OFFSET = 1469598103934665603
FNV_PRIME = 0x100000001B3


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from('<I', data, offset)[0]


def u64(data: bytes, offset: int) -> int:
    return struct.unpack_from('<Q', data, offset)[0]


def fnv1a64(data: bytes, *, zero_package_hash: bool = False) -> int:
    value = FNV_OFFSET
    for index, byte in enumerate(data):
        if zero_package_hash and 72 <= index < 80:
            byte = 0
        value ^= byte
        value = (value * FNV_PRIME) & MASK
    return value


def fnv_u64(value: int, item: int) -> int:
    for index in range(8):
        value ^= (item >> (index * 8)) & 0xFF
        value = (value * FNV_PRIME) & MASK
    return value


def fnv_bytes(value: int, data: bytes) -> int:
    for byte in data:
        value ^= byte
        value = (value * FNV_PRIME) & MASK
    return value


def mix64(value: int) -> int:
    value = (value + 0x9E3779B97F4A7C15) & MASK
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & MASK
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & MASK
    return (value ^ (value >> 31)) & MASK


def xor_stream(data: bytes, seed: int) -> bytes:
    output = bytearray(len(data))
    state = seed
    word = 0
    for index, byte in enumerate(data):
        if index & 7 == 0:
            state = (state + 0x9E3779B97F4A7C15) & MASK
            word = mix64(state)
        output[index] = byte ^ ((word >> ((index & 7) * 8)) & 0xFF)
    return bytes(output)


def open_aead(data: bytes, section_type: int, name: str) -> bytes:
    if data[:8] != b'VAEAD001':
        raise ValueError(f'unsupported protected section wrapper {data[:8]!r}')
    version, stored_type = struct.unpack_from('<II', data, 8)
    plain_size = u64(data, 16)
    name_hash = u64(data, 24)
    nonce_a = u64(data, 32)
    nonce_b = u64(data, 40)
    tag_a = u64(data, 48)
    tag_b = u64(data, 56)
    ciphertext = data[64:]
    if version != 1 or stored_type != section_type or plain_size != len(ciphertext):
        raise ValueError('invalid VAEAD001 header')
    if name_hash != fnv1a64(name.encode('utf-8')):
        raise ValueError('section name hash mismatch')

    check_a = FNV_OFFSET ^ 0x3D6F0A9B8C71E245
    for item in (section_type, name_hash, plain_size, nonce_a, nonce_b):
        check_a = fnv_u64(check_a, item)
    check_a = mix64(fnv_bytes(check_a, ciphertext) ^ 0x9B62D14FA7C8305D)

    check_b = FNV_OFFSET ^ 0x6C8E9CF570932BD5
    for item in (nonce_b, nonce_a, plain_size, name_hash, section_type):
        check_b = fnv_u64(check_b, item)
    check_b = mix64(fnv_bytes(check_b, ciphertext) ^ 0xA41BC927F35D69E1)
    if (check_a, check_b) != (tag_a, tag_b):
        raise ValueError('section authentication tag mismatch')

    seed = FNV_OFFSET ^ 0xA41BC927F35D69E1
    for item in (section_type, name_hash, plain_size, nonce_a, nonce_b):
        seed = fnv_u64(seed, item)
    seed = mix64(seed ^ 0x6C8E9CF570932BD5)
    return xor_stream(ciphertext, seed)


def decompress(data: bytes, expected_size: int) -> bytes:
    if data[:8] != b'VCLZ0008' or u32(data, 8) != 1 or u64(data, 12) != expected_size:
        raise ValueError('invalid VCLZ0008 stream')
    output = bytearray()
    cursor = 20
    while cursor < len(data):
        token = data[cursor]
        cursor += 1
        if token & 0x80:
            length = (token & 0x7F) + 4
            if cursor + 2 > len(data):
                raise ValueError('truncated compressed back-reference')
            distance = data[cursor] | (data[cursor + 1] << 8)
            cursor += 2
            if distance == 0 or distance > len(output):
                raise ValueError('invalid compressed back-reference')
            start = len(output) - distance
            for index in range(length):
                output.append(output[start + index])
        else:
            length = token + 1
            if cursor + length > len(data):
                raise ValueError('truncated compressed literal')
            output.extend(data[cursor:cursor + length])
            cursor += length
    if len(output) != expected_size:
        raise ValueError(f'decoded size mismatch: {len(output)} != {expected_size}')
    return bytes(output)


def decoded_sections(package: bytes) -> list[tuple[str, bytes]]:
    if package[:8] != b'VENOMVBC':
        return [('artifact', package)]
    if len(package) < 80 or fnv1a64(package, zero_package_hash=True) != u64(package, 72):
        raise ValueError('invalid package header or package hash')
    count = u32(package, 20)
    table_offset = u64(package, 24)
    table_size = u64(package, 32)
    names_offset = u64(package, 40)
    names_size = u64(package, 48)
    if table_size != count * 48 or table_offset + table_size > len(package) or names_offset + names_size > len(package):
        raise ValueError('invalid section table bounds')

    sections: list[tuple[str, bytes]] = [('package', package)]
    for index in range(count):
        base = table_offset + index * 48
        section_type, flags, name_offset, name_size = struct.unpack_from('<IIII', package, base)
        data_offset, stored_size, raw_size, stored_hash = struct.unpack_from('<QQQQ', package, base + 16)
        if name_offset + name_size > names_size or data_offset + stored_size > len(package):
            raise ValueError(f'invalid section {index} bounds')
        name = package[names_offset + name_offset:names_offset + name_offset + name_size].decode('utf-8')
        data = package[data_offset:data_offset + stored_size]
        if fnv1a64(data) != stored_hash:
            raise ValueError(f'section {index} stored hash mismatch')
        if flags & 2:
            data = open_aead(data, section_type, name)
        if flags & 1:
            data = decompress(data, raw_size)
        if len(data) != raw_size:
            raise ValueError(f'section {index} decoded size mismatch')
        sections.append((f'{index}:{name}', data))
    return sections


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('artifact', type=Path)
    parser.add_argument('--forbid', action='append', default=[])
    parser.add_argument('--strict', action='store_true')
    args = parser.parse_args()

    try:
        sections = decoded_sections(args.artifact.read_bytes())
    except (OSError, ValueError, struct.error, UnicodeDecodeError) as exc:
        print(f'[venom] release metadata check failed: {exc}', file=sys.stderr)
        return 2

    violations: list[str] = []
    for value in args.forbid:
        needle = value.encode('utf-8')
        for section_name, data in sections:
            if needle in data:
                violations.append(f'{value!r} in {section_name}')
                break

    if args.strict:
        forbidden_tokens = (b'word_layout=', b'opcode_xor=', b'operand_xor=', b'VENOM_POLYMORPH 8')
        for token in forbidden_tokens:
            for section_name, data in sections:
                if token in data:
                    violations.append(f'{token.decode()} in {section_name}')
                    break
        for section_name, data in sections:
            if re.search(rb'source_bytes=[1-9][0-9]*', data):
                violations.append(f'non-redacted source_bytes in {section_name}')
            if re.search(rb'source_hash=0x[1-9a-fA-F][0-9a-fA-F]*', data):
                violations.append(f'non-redacted source_hash in {section_name}')
        if not any(b'VPOL0010' in data for _, data in sections):
            violations.append('VPOL0010 binary plan missing')

    if violations:
        print('[venom] release metadata leak:', file=sys.stderr)
        for violation in violations:
            print(f'  - {violation}', file=sys.stderr)
        return 2

    print(f'[venom] release metadata check: PASS ({args.artifact}, {len(sections) - 1} decoded sections)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
