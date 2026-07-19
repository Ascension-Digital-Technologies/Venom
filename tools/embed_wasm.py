#!/usr/bin/env python3
"""Embed a WebAssembly module into Venom's C++ constexpr blob header."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

DEFAULT_PROVENANCE = """VENOM_QJS_WASM_EMBED_V2
version=2
runtime_abi=12
package_version=83
artifact_kind=contract-scaffold
runtime_implementation=quickjs-wasm-contract-scaffold
runtime_claim=contract-boundary
contract_only=true
scaffold_runtime=true
real_engine_candidate=false
full_upstream_quickjs=false
fallback_required=false
finish_blocker=run_build_quickjs_wasm_and_embed_verified_artifact
required_exports_satisfied=false
missing_export_count=0
"""


def cxx_raw_string_literal(value: str, tag: str = "VQWASM") -> str:
    if f"){tag}\"" not in value:
        return f'R"{tag}({value}){tag}"'
    escaped = value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    return f'"{escaped}"'


def emit_header(wasm: bytes, output: Path, namespace: str, symbol: str, provenance: str) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        f"namespace {namespace} {{",
        f"inline constexpr std::uint8_t {symbol}[] = {{",
    ]
    for i in range(0, len(wasm), 12):
        chunk = wasm[i:i + 12]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.extend([
        "};",
        f"inline constexpr std::size_t {symbol}Size = sizeof({symbol});",
        f"inline constexpr const char* {symbol}Sha256 = \"{hashlib.sha256(wasm).hexdigest()}\";",
        f"inline constexpr const char* {symbol}Provenance = {cxx_raw_string_literal(provenance)};",
        f"}} // namespace {namespace}",
        "",
    ])
    output.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("wasm", type=Path, help="input .wasm file")
    ap.add_argument("--out", type=Path, default=Path("include/venom/generated/runtime/quickjs_runtime_wasm_blob.hpp"))
    ap.add_argument("--namespace", default="venom::compiler")
    ap.add_argument("--symbol", default="kQuickJsRuntimeWasmBlob")
    ap.add_argument("--manifest", type=Path, help="verified manifest to embed as provenance")
    args = ap.parse_args()

    wasm = args.wasm.read_bytes()
    if not wasm.startswith(b"\0asm"):
        raise SystemExit(f"{args.wasm} is not a WebAssembly module")
    provenance = args.manifest.read_text(encoding="utf-8") if args.manifest else DEFAULT_PROVENANCE
    for version in ("V3", "V2"):
        artifact_marker = f"VENOM_QJS_WASM_ARTIFACT_{version}"
        embed_marker = f"VENOM_QJS_WASM_EMBED_{version}"
        if artifact_marker in provenance:
            provenance = provenance.replace(artifact_marker, embed_marker, 1)
            break
    emit_header(wasm, args.out, args.namespace, args.symbol, provenance)
    print(f"embedded {len(wasm)} bytes into {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
