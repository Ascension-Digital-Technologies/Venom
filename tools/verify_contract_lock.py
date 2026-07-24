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
    turbojs_path = root / "contracts/turbojs-wasm-abi.json"
    runtime_path = root / "contracts/runtime-api.json"
    product = json.loads(product_path.read_text(encoding="utf-8"))
    turbojs = json.loads(turbojs_path.read_text(encoding="utf-8"))
    runtime = json.loads(runtime_path.read_text(encoding="utf-8"))
    return {
        "schema": LOCK_SCHEMA,
        "packageFormatVersion": product["package_format_version"],
        "packageRuntimeAbi": product["package_runtime_abi"],
        "turboJsRuntimeAbi": product["turbojs_runtime_abi"],
        "turboJsWasmRuntimeAbi": turbojs["runtimeAbi"],
        "turboJsWasmBridgeAbi": turbojs["bridgeAbi"],
        "hostBridgeAbi": product["host_bridge_abi"],
        "bytecodeFormat": turbojs["bytecodeFormat"],
        "moduleBundleFormat": product["turbojs_module_bundle_contract"],
        "routeVmContract": product["route_vm_contract"],
        "domCommandContract": product["dom_command_contract"],
        "contractDigests": {
            "product-contracts.json": sha256(product_path),
            "turbojs-wasm-abi.json": sha256(turbojs_path),
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
        for rel in ("contracts/turbojs-wasm-abi.json",):
            target = shadow / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((root / rel).read_bytes())
        subprocess.run(
            [sys.executable, str(root / "tools/generate_turbojs_wasm_abi.py"), "--repo-root", str(shadow)],
            cwd=root,
            check=True,
            stdout=subprocess.DEVNULL,
        )
        for rel in (
            "src/generated/contracts/turbojs_wasm_abi.hpp",
            "src/generated/runtime/javascript/turbojs_wasm_abi.js",
            "docs/generated/turbojs-wasm-abi.md",
        ):
            expected = shadow / rel
            actual = root / rel
            if not actual.is_file() or actual.read_bytes() != expected.read_bytes():
                fail(f"stale generated TurboJS/WASM ABI binding: {rel}")



def verify_export_ownership(root: Path) -> None:
    """Reject stray source-tree ABI export manifests and legacy naming."""
    stray = root / "-"
    if stray.exists():
        fail("stray root ABI export manifest '-' must not be committed")

    contract = json.loads((root / "contracts/turbojs-wasm-abi.json").read_text(encoding="utf-8"))
    required = contract["requiredExports"]
    for name in required:
        if name != "memory" and not name.startswith("venom_tjs_"):
            fail(f"required ABI export is outside the TJS namespace: {name}")

    approved = {
        root / "contracts/turbojs-wasm-abi.json",
        root / "src/generated/runtime/javascript/turbojs_wasm_abi.js",
        root / "src/generated/contracts/turbojs_wasm_abi.hpp",
        root / "docs/generated/turbojs-wasm-abi.md",
    }
    for path in root.iterdir():
        if not path.is_file() or path in approved:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        if text.lstrip().startswith("[") and "_venom_tjs_" in text:
            fail(f"stray top-level ABI export manifest: {path.name}")

def verify_runtime_literals(root: Path, lock: dict[str, object]) -> None:
    package_abi = lock["packageRuntimeAbi"]
    turbojs_abi = lock["turboJsRuntimeAbi"]
    bytecode = lock["bytecodeFormat"]
    module_bundle = lock["moduleBundleFormat"]
    package_version = lock["packageFormatVersion"]

    active_runtime_files = [
        root / "src/generated/runtime/javascript/browser_runtime.js",
        root / "src/templates/browser/10-package-loader.js",
        root / "src/templates/browser/20-runtime-contracts.js",
        root / "src/runtime/turbojs_runtime_scaffold.c",
        root / "src/pipeline/build.cpp",
        root / "src/pipeline/build_support.cpp",
    ]
    for path in active_runtime_files:
        if not path.is_file():
            fail(f"missing active runtime contract source: {path.relative_to(root)}")

    require_text(root / "src/generated/runtime/javascript/browser_runtime.js", f"pkg.runtimeAbi !== {package_abi}", "package runtime ABI")
    require_text(root / "src/templates/browser/10-package-loader.js", f"pkg.runtimeAbi !== {package_abi}", "browser package runtime ABI")
    require_text(root / "src/templates/browser/20-runtime-contracts.js", f"runtimeAbi: {turbojs_abi}", "TurboJS runtime ABI")
    require_text(root / "src/generated/runtime/javascript/browser_runtime.js", f"runtimeAbi: {turbojs_abi}", "runtime template TurboJS ABI")
    require_text(root / "src/pipeline/build.cpp", f'bytecode_format != "{bytecode}"', "compiler bytecode format")
    require_text(root / "src/runtime/turbojs_runtime_scaffold.c", f'"bytecode_format={bytecode}\\n"', "WASM bytecode format")
    require_text(root / "src/pipeline/build_runtime_module_metadata.cpp", f"{bytecode}|{module_bundle}", "module bundle envelope")
    require_text(root / "tools/product_contracts.py", f"'package_format_version': {package_version}", "generated package format")

    # Legacy format names may remain in explicit rejection/audit checks. Ensure no
    # active decoder advertises them as accepted formats.
    for path in (
        root / "src/templates/browser/20-runtime-contracts.js",
        root / "src/generated/runtime/javascript/browser_runtime.js",
        root / "src/runtime/turbojs_runtime_scaffold.c",
    ):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for legacy in ("VTJSBC01", "VTJSBC02"):
            accepted_patterns = (
                f"storedMagic === '{legacy}'",
                f"bytecodeMagic === '{legacy}'",
                f'strcmp(magic, "{legacy}") == 0',
            )
            if any(pattern in text for pattern in accepted_patterns):
                fail(f"legacy bytecode format {legacy} is still accepted by {path.relative_to(root)}")
        if path.suffix == ".js" and "source-preserving TurboJS bytecode record rejected" not in text:
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
    if lock["turboJsRuntimeAbi"] != lock["turboJsWasmRuntimeAbi"]:
        fail("product TurboJS runtime ABI differs from TurboJS/WASM runtime ABI")
    verify_generated_bindings(root)
    verify_export_ownership(root)
    verify_runtime_literals(root, lock)
    verify_lock_file(root, lock, args.update)
    print(
        "[PASS] ABI lock: "
        f"package={lock['packageFormatVersion']}/{lock['packageRuntimeAbi']} "
        f"turbojs={lock['turboJsRuntimeAbi']} bridge={lock['turboJsWasmBridgeAbi']} "
        f"bytecode={lock['bytecodeFormat']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
