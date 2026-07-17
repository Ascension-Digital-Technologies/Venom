#!/usr/bin/env python3
from __future__ import annotations
import argparse, json
from pathlib import Path

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[1])
    args=ap.parse_args(); root=args.repo_root.resolve()
    data=json.loads((root/'contracts/quickjs-wasm-abi.json').read_text())
    req=data['requiredExports']; opt=data.get('optionalExports',[]); tool=data.get('allowedToolchainExports',[])
    dev=data['trustDomains']['development']; rel=data['trustDomains']['release']
    h=root/'src/generated/contracts/quickjs_wasm_abi.hpp'; h.parent.mkdir(parents=True,exist_ok=True)
    def arr(name, vals):
        return f'inline constexpr std::array<std::string_view, {len(vals)}> {name} = {{\n' + ''.join(f'  "{v}",\n' for v in vals) + '};\n'
    h.write_text('#pragma once\n// Generated from contracts/quickjs-wasm-abi.json. Do not edit.\n#include <array>\n#include <string_view>\nnamespace venom::generated::quickjs_wasm_abi {\n'
                 f'inline constexpr unsigned runtime_abi = {data["runtimeAbi"]}u;\n'
                 f'inline constexpr unsigned bridge_abi = {data["bridgeAbi"]}u;\n'
                 f'inline constexpr std::string_view bytecode_format = "{data["bytecodeFormat"]}";\n' +
                 arr('required_exports',req)+arr('optional_exports',opt)+arr('allowed_toolchain_exports',tool)+arr('development_trust_domains',dev)+arr('release_trust_domains',rel)+'}\n')
    js=root/'src/generated/runtime/quickjs_wasm_abi.js'; js.parent.mkdir(parents=True,exist_ok=True)
    js.write_text('// Generated from contracts/quickjs-wasm-abi.json. Do not edit.\nexport const QUICKJS_WASM_ABI = Object.freeze('+json.dumps(data,separators=(',',':'))+');\n')
    md=root/'docs/generated/quickjs-wasm-abi.md'; md.parent.mkdir(parents=True,exist_ok=True)
    md.write_text('# QuickJS/WASM ABI\n\nGenerated from `contracts/quickjs-wasm-abi.json`.\n\n'
                  f'- Runtime ABI: `{data["runtimeAbi"]}`\n- Bridge ABI: `{data["bridgeAbi"]}`\n- Bytecode format: `{data["bytecodeFormat"]}`\n\n## Required exports\n\n'+''.join(f'- `{x}`\n' for x in req))
    print(f'generated QuickJS/WASM ABI bindings ({len(req)} required exports)')
if __name__=='__main__': main()
