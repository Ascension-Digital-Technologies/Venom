#!/usr/bin/env python3
"""Generate or verify provenance for the embedded Venom package/route WASM runtime."""
from __future__ import annotations
import argparse, hashlib, json, re
from pathlib import Path

EXPECTED_EXPORTS = ["memory", "venom_runtime_abi", "venom_package_version", "v12_p", "v12_n", "v12_x"]

def uleb(data: bytes, pos: int) -> tuple[int, int]:
    value = shift = 0
    while True:
        if pos >= len(data): raise ValueError("truncated ULEB128")
        byte = data[pos]; pos += 1; value |= (byte & 0x7f) << shift
        if byte < 0x80: return value, pos
        shift += 7
        if shift > 63: raise ValueError("oversized ULEB128")

def embedded_bytes(header: Path) -> bytes:
    text = header.read_text(encoding="utf-8")
    data = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", text))
    if not data.startswith(b"\0asm\x01\0\0\0"): raise ValueError("embedded blob is not WebAssembly v1")
    return data

def exports(data: bytes) -> list[str]:
    result=[]; pos=8
    while pos < len(data):
        section_id=data[pos]; pos += 1
        size,pos=uleb(data,pos); end=pos+size
        if end > len(data): raise ValueError("truncated WASM section")
        if section_id == 7:
            count,p=uleb(data,pos)
            for _ in range(count):
                length,p=uleb(data,p); name=data[p:p+length].decode("utf-8"); p += length
                p += 1; _,p=uleb(data,p); result.append(name)
        pos=end
    return result

def manifest(data: bytes) -> dict:
    found=exports(data)
    return {
      "schema_version": 1,
      "artifact_kind": "venom-package-route-runtime-wasm",
      "runtime_implementation": "venom-native-wasm-runtime",
      "scaffold_runtime": False,
      "wasm_version": 1,
      "byte_length": len(data),
      "sha256": hashlib.sha256(data).hexdigest(),
      "exports": found,
      "required_exports": EXPECTED_EXPORTS,
      "required_exports_satisfied": all(x in found for x in EXPECTED_EXPORTS),
      "package_runtime_abi": 12,
      "dom_command_contract": "VDOP0020",
      "dom_command_contract_embedded": b"VDOP0020" in data,
      "route_vm_contract": "VPOL0010-v2-fixed16",
      "route_vm_contract_location": "protected package plan section",
      "route_vm_contract_not_claimed_as_wasm_literal": True,
    }

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--header", type=Path, default=Path("include/venom/generated/runtime/wasm_runtime_blob.hpp"))
    ap.add_argument("--out", type=Path, default=Path("src/generated/runtime/metadata/wasm_runtime_provenance.json"))
    ap.add_argument("--check", action="store_true")
    args=ap.parse_args()
    current=manifest(embedded_bytes(args.header))
    if not current["required_exports_satisfied"]: raise SystemExit("embedded runtime is missing required exports")
    if not current["dom_command_contract_embedded"]: raise SystemExit("embedded runtime is missing VDOP0020")
    rendered=json.dumps(current, indent=2, sort_keys=True)+"\n"
    if args.check:
        if not args.out.is_file(): raise SystemExit(f"missing provenance manifest: {args.out}")
        if args.out.read_text(encoding="utf-8") != rendered: raise SystemExit("route WASM provenance is stale; regenerate it")
        print("route WASM provenance: PASS")
        return 0
    args.out.parent.mkdir(parents=True, exist_ok=True); args.out.write_text(rendered, encoding="utf-8")
    print(f"wrote {args.out}")
    return 0
if __name__ == "__main__": raise SystemExit(main())
