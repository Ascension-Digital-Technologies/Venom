#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path

CHUNK_SIZE = 8192


def cpp_escape(data: bytes) -> str:
    out: list[str] = []
    for b in data:
        if b == 0x5C:
            out.append('\\\\')
        elif b == 0x22:
            out.append('\\"')
        elif b == 0x3F:
            out.append('\\?')
        elif b == 0x0A:
            out.append('\\n')
        elif b == 0x0D:
            out.append('\\r')
        elif b == 0x09:
            out.append('\\t')
        elif 0x20 <= b <= 0x7E:
            out.append(chr(b))
        else:
            out.append(f'\\{b:03o}')
    return ''.join(out)


def main() -> int:
    parser = argparse.ArgumentParser(description='Embed the vendored TypeScript compiler as chunked C++ source.')
    parser.add_argument('--input', required=True, type=Path)
    parser.add_argument('--output-cpp', required=True, type=Path)
    parser.add_argument('--output-hpp', required=True, type=Path)
    args = parser.parse_args()

    data = args.input.read_bytes()
    text = data.decode('utf-8')
    match = re.search(r'versionMajorMinor\s*=\s*"([^"]+)"', text)
    version = match.group(1) if match else 'unknown'
    digest = hashlib.sha256(data).hexdigest()
    chunks = [data[i:i + CHUNK_SIZE] for i in range(0, len(data), CHUNK_SIZE)]

    args.output_hpp.parent.mkdir(parents=True, exist_ok=True)
    args.output_cpp.parent.mkdir(parents=True, exist_ok=True)

    hpp = '''#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace venom::generated::typescript {

std::string compiler_source();
std::size_t compiler_source_size() noexcept;
std::string_view compiler_source_sha256() noexcept;
std::string_view compiler_version() noexcept;

} // namespace venom::generated::typescript
'''
    args.output_hpp.write_text(hpp, encoding='utf-8', newline='\n')

    lines = [
        '#include "generated/compiler/typescript_compiler_asset.hpp"',
        '',
        '#include <array>',
        '#include <string>',
        '#include <string_view>',
        '',
        'namespace venom::generated::typescript {',
        'namespace {',
        'struct Chunk { const char* data; std::size_t size; };',
        '',
    ]
    for i, chunk in enumerate(chunks):
        escaped = cpp_escape(chunk)
        lines.append(f'static const char kChunk{i}[] = "{escaped}";')
    lines += ['', f'static constexpr std::array<Chunk, {len(chunks)}> kChunks = {{{{']
    for i, chunk in enumerate(chunks):
        lines.append(f'  {{kChunk{i}, {len(chunk)}}},')
    lines += [
        '}};',
        f'static constexpr std::size_t kSourceSize = {len(data)};',
        f'static constexpr std::string_view kSourceSha256 = "{digest}";',
        f'static constexpr std::string_view kCompilerVersion = "{version}";',
        '} // namespace',
        '',
        'std::string compiler_source() {',
        '  std::string source;',
        '  source.reserve(kSourceSize);',
        '  for (const auto& chunk : kChunks) source.append(chunk.data, chunk.size);',
        '  return source;',
        '}',
        '',
        'std::size_t compiler_source_size() noexcept { return kSourceSize; }',
        'std::string_view compiler_source_sha256() noexcept { return kSourceSha256; }',
        'std::string_view compiler_version() noexcept { return kCompilerVersion; }',
        '',
        '} // namespace venom::generated::typescript',
        '',
    ]
    args.output_cpp.write_text('\n'.join(lines), encoding='utf-8', newline='\n')
    print(f'embedded {len(data)} bytes in {len(chunks)} chunks; sha256={digest}; version={version}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
