#!/usr/bin/env python3
"""Verify Venom's release ABI contracts and generated bindings are synchronized."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

LOCK_SCHEMA = "VENOM_ABI_LOCK_V1"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(message: str) -> None:
    print(f"[FAIL] {message}", file=sys.stderr)
    raise SystemExit(1)


def require_text(path: Path, token: str, label: str) -> None:
    if token not in path.read_text(encoding="utf-8"):
        fail(f"{label} is not synchronized: missing {token!r} in {path}")


def canonical_lock(root: Path) -> dict[str, object]:
    product_path = root / "contracts/product-contracts.json"
    quickjs_path = root / "contracts/quickjs-wasm-abi.json"
    runtime_path = root / "contracts/runtime-api.json"
    product = json.loads(product_path.read_text(encoding="utf-8"))
    quickjs = json.loads(quickjs_path.read_text(encoding="utf-8"))
    runtime = json.loads(runtime_path.read_text(encoding="utf-8"))
    return {
        "schema": LOCK_SCHEMA,
        "packageFormatVersion": product["package_format_version"],
        "packageRuntimeAbi": product["package_runtime_abi"],
        "quickJsRuntimeAbi": product["quickjs_runtime_abi"],
        "quickJsWasmRuntimeAbi": quickjs["runtimeAbi"],
        "quickJsWasmBridgeAbi": quickjs["bridgeAbi"],
        "hostBridgeAbi": product["host_bridge_abi"],
        "bytecodeFormat": quickjs["bytecodeFormat"],
        "moduleBundleFormat": product["quickjs_module_bundle_contract"],
        "routeVmContract": product["route_vm_contract"],
        "domCommandContract": product["dom_command_contract"],
        "contractDigests": {
            "product-contracts.json": sha256(product_path),
            "quickjs-wasm-abi.json": sha256(quickjs_path),
            "runtime-api.json": sha256(runtime_path),
        },
        "runtimeApiSchema": runtime.get("schema"),
    }


def verify_generated_bindings(root: Path) -> None:
    subprocess.run(
        [sys.executable, str(root / "tools/generate_product_contracts.py"), "--repo-root", str(root), "--check"],
        cwd=root,
        check=True,
    )
    with tempfile.TemporaryDirectory(prefix="venom-abi-bindings-") as td:
        shadow = Path(td)
        for rel in ("contracts/quickjs-wasm-abi.json",):
            target = shadow / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((root / rel).read_bytes())
        subprocess.run(
            [sys.executable, str(root / "tools/generate_quickjs_wasm_abi.py"), "--repo-root", str(shadow)],
            cwd=root,
            check=True,
            stdout=subprocess.DEVNULL,
        )
        for rel in (
            "src/generated/include/venom/generated/contracts/quickjs_wasm_abi.hpp",
            "src/generated/runtime/javascript/quickjs_wasm_abi.js",
            "docs/generated/quickjs-wasm-abi.md",
        ):
            expected = shadow / rel
            actual = root / rel
            if not actual.is_file() or actual.read_bytes() != expected.read_bytes():
                fail(f"stale generated QuickJS/WASM ABI binding: {rel}")


def verify_runtime_literals(root: Path, lock: dict[str, object]) -> None:
    package_abi = lock["packageRuntimeAbi"]
    quickjs_abi = lock["quickJsRuntimeAbi"]
    bytecode = lock["bytecodeFormat"]
    module_bundle = lock["moduleBundleFormat"]
    package_version = lock["packageFormatVersion"]

    active_runtime_files = [
        root / "src/generated/runtime/javascript/browser_runtime.js",
        root / "src/templates/browser/10-package-loader.js",
        root / "src/templates/browser/20-runtime-contracts.js",
        root / "src/runtime/quickjs_runtime_scaffold.c",
        root / "src/pipeline/build.cpp",
        root / "src/pipeline/build_support.cpp",
    ]
    for path in active_runtime_files:
        if not path.is_file():
            fail(f"missing active runtime contract source: {path.relative_to(root)}")

    require_text(root / "src/generated/runtime/javascript/browser_runtime.js", f"pkg.runtimeAbi !== {package_abi}", "package runtime ABI")
    require_text(root / "src/templates/browser/10-package-loader.js", f"pkg.runtimeAbi !== {package_abi}", "browser package runtime ABI")
    require_text(root / "src/templates/browser/20-runtime-contracts.js", f"runtimeAbi: {quickjs_abi}", "QuickJS runtime ABI")
    require_text(root / "src/generated/runtime/javascript/browser_runtime.js", f"runtimeAbi: {quickjs_abi}", "runtime template QuickJS ABI")
    require_text(root / "src/pipeline/build.cpp", f'bytecode_format != "{bytecode}"', "compiler bytecode format")
    require_text(root / "src/runtime/quickjs_runtime_scaffold.c", f'"bytecode_format={bytecode}\\n"', "WASM bytecode format")
    require_text(root / "src/pipeline/build_runtime_module_metadata.cpp", f"{bytecode}|{module_bundle}", "module bundle envelope")
    require_text(root / "tools/product_contracts.py", f"'package_format_version': {package_version}", "generated package format")

    # Legacy format names may remain in explicit rejection/audit checks. Ensure no
    # active decoder advertises them as accepted formats.
    for path in (
        root / "src/templates/browser/20-runtime-contracts.js",
        root / "src/generated/runtime/javascript/browser_runtime.js",
        root / "src/runtime/quickjs_runtime_scaffold.c",
    ):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for legacy in ("VQJSBC01", "VQJSBC02"):
            accepted_patterns = (
                f"storedMagic === '{legacy}'",
                f"bytecodeMagic === '{legacy}'",
                f'strcmp(magic, "{legacy}") == 0',
            )
            if any(pattern in text for pattern in accepted_patterns):
                fail(f"legacy bytecode format {legacy} is still accepted by {path.relative_to(root)}")
        if path.suffix == ".js" and "source-preserving QuickJS bytecode record rejected" not in text:
            fail(f"legacy bytecode rejection guard is missing from {path.relative_to(root)}")


def verify_lock_file(root: Path, expected: dict[str, object], update: bool) -> None:
    path = root / "contracts/abi-lock.json"
    rendered = json.dumps(expected, indent=2, sort_keys=True) + "\n"
    if update:
        path.write_text(rendered, encoding="utf-8")
        print(f"[PASS] wrote {path.relative_to(root)}")
        return
    if not path.is_file():
        fail("missing contracts/abi-lock.json; run tools/verify_contract_lock.py --update")
    if path.read_text(encoding="utf-8") != rendered:
        fail("contracts/abi-lock.json is stale; an ABI-affecting contract changed without updating the lock")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--update", action="store_true", help="rewrite contracts/abi-lock.json from authoritative contracts")
    args = ap.parse_args()
    root = args.repo_root.resolve()
    lock = canonical_lock(root)
    if lock["quickJsRuntimeAbi"] != lock["quickJsWasmRuntimeAbi"]:
        fail("product QuickJS runtime ABI differs from QuickJS/WASM runtime ABI")
    verify_generated_bindings(root)
    verify_runtime_literals(root, lock)
    verify_lock_file(root, lock, args.update)
    print(
        "[PASS] ABI lock: "
        f"package={lock['packageFormatVersion']}/{lock['packageRuntimeAbi']} "
        f"quickjs={lock['quickJsRuntimeAbi']} bridge={lock['quickJsWasmBridgeAbi']} "
        f"bytecode={lock['bytecodeFormat']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
