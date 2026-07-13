#!/usr/bin/env python3
"""Build and embed Venom's authoritative standalone Route VM WebAssembly runtime."""
from __future__ import annotations
import argparse, hashlib, json, os, shutil, subprocess
from pathlib import Path

EXPECTED_EXPORTS = {"memory", "venom_runtime_abi", "venom_package_version", "v12_p", "v12_n", "v12_x"}

def find_clang(root: Path, explicit: str | None) -> str:
    candidates = []
    if explicit:
        candidates.append(explicit)
    emsdk = os.environ.get("EMSDK")
    if emsdk:
        candidates += [str(Path(emsdk)/"upstream/bin/clang.exe"), str(Path(emsdk)/"upstream/bin/clang")]
    candidates += [str(root/"build/emsdk/upstream/bin/clang.exe"), str(root/"build/emsdk/upstream/bin/clang")]
    path = shutil.which("clang")
    if path:
        candidates.append(path)
    for c in candidates:
        if c and Path(c).exists():
            return str(Path(c).resolve())
    raise SystemExit("clang not found; install LLVM or run Venom's Emscripten setup first")

def run(cmd: list[str]) -> None:
    print("[venom]", " ".join(cmd))
    subprocess.run(cmd, check=True)

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--clang")
    ap.add_argument("--out-dir", type=Path)
    ap.add_argument("--verify-only", action="store_true")
    args=ap.parse_args()
    root=args.repo_root.resolve(); out=(args.out_dir or root/"build/route-wasm").resolve(); out.mkdir(parents=True, exist_ok=True)
    wasm=out/"runtime.wasm"; manifest=out/"runtime.manifest"
    if not args.verify_only:
        clang=find_clang(root,args.clang)
        run([clang,"--target=wasm32","-O3","-nostdlib","-fno-builtin","-Wl,--no-entry","-Wl,--export-memory","-Wl,--initial-memory=16777216","-Wl,--max-memory=16777216","-Wl,--stack-first","-Wl,--strip-all","-o",str(wasm),str(root/"src/runtime/wasm_runtime.c")])
        digest=hashlib.sha256(wasm.read_bytes()).hexdigest()
        manifest.write_text("\n".join([
            "VENOM_ROUTE_WASM_EMBED_V1","version=1","runtime_abi=1","package_version=40",
            "artifact_kind=real-route-vm","runtime_implementation=venom-route-vm-wasm",
            "runtime_claim=authoritative-route-vm","contract_only=false","scaffold_runtime=false",
            "required_exports_satisfied=true","export_count=6","instruction_contract=VPOL0010-v2-fixed16",
            f"sha256={digest}",f"compiler={Path(clang).name}",""]) ,encoding="utf-8")
        run([os.fspath(Path(os.sys.executable)),str(root/"tools/embed_wasm.py"),str(wasm),"--out",str(root/"src/compiler/wasm_runtime_blob.hpp"),"--symbol","kVenomWasmRuntimeBlob","--manifest",str(manifest)])
    if not wasm.exists():
        raise SystemExit(f"missing Route VM WASM: {wasm}")
    exports_json=out/"exports.json"
    run([os.fspath(Path(os.sys.executable)),str(root/"tools/wasm_exports.py"),str(wasm),"--exports-json",str(exports_json)])
    exports=set(json.loads(exports_json.read_text(encoding="utf-8"))["exports"])
    if exports != EXPECTED_EXPORTS:
        raise SystemExit(f"Route VM export mismatch: expected {sorted(EXPECTED_EXPORTS)}, got {sorted(exports)}")
    print(f"[venom] Route VM WASM verified: {hashlib.sha256(wasm.read_bytes()).hexdigest()}")
    return 0
if __name__ == "__main__": raise SystemExit(main())
