#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib
from pathlib import Path

def main() -> int:
    p=argparse.ArgumentParser(); p.add_argument('--input',required=True,type=Path); p.add_argument('--output-cpp',required=True,type=Path); p.add_argument('--output-hpp',required=True,type=Path); args=p.parse_args()
    data=args.input.read_bytes(); digest=hashlib.sha256(data).hexdigest(); values=','.join(str(x) for x in data)
    args.output_hpp.write_text('''#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace venom::generated::typescript {
const std::uint8_t* bridge_bytecode_data() noexcept;
std::size_t bridge_bytecode_size() noexcept;
std::string_view bridge_bytecode_sha256() noexcept;
std::string_view bridge_bytecode_turbojs_version() noexcept;
std::uint32_t bridge_bytecode_abi() noexcept;
} // namespace venom::generated::typescript
''')
    args.output_cpp.write_text(f'''#include "generated/compiler/typescript_bridge_bytecode.hpp"
#include <array>
namespace venom::generated::typescript {{ namespace {{
static constexpr std::array<std::uint8_t, {len(data)}> kData = {{{values}}};
static constexpr std::string_view kSha = "{digest}";
}} const std::uint8_t* bridge_bytecode_data() noexcept {{ return kData.data(); }}
std::size_t bridge_bytecode_size() noexcept {{ return kData.size(); }}
std::string_view bridge_bytecode_sha256() noexcept {{ return kSha; }}
std::string_view bridge_bytecode_turbojs_version() noexcept {{ return "0.15.1"; }}
std::uint32_t bridge_bytecode_abi() noexcept {{ return 27u; }}
}} // namespace venom::generated::typescript
''')
    print(f'embedded {len(data)} bridge bytecode bytes; sha256={digest}'); return 0
if __name__=='__main__': raise SystemExit(main())
