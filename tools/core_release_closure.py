#!/usr/bin/env python3
"""Run the authoritative Venom core release closure."""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path


@dataclass
class Stage:
    name: str
    command: list[str]
    seconds: float = 0.0
    passed: bool = False


def run_stage(stage: Stage, root: Path, env: dict[str, str]) -> None:
    print(f"\n[CORE] {stage.name}")
    started = time.perf_counter()
    result = subprocess.run(stage.command, cwd=root, env=env)
    stage.seconds = round(time.perf_counter() - started, 3)
    stage.passed = result.returncode == 0
    marker = "PASS" if stage.passed else "FAIL"
    print(f"[{marker}] {stage.name} ({stage.seconds:.3f}s)")
    if not stage.passed:
        raise SystemExit(result.returncode or 1)


def python_stage(root: Path, name: str, rel: str, *args: str) -> Stage:
    return Stage(name, [sys.executable, str(root / rel), *args])


def configure_and_build(root: Path, build_dir: Path, config: str) -> list[Stage]:
    configure = [
        "cmake", "-S", str(root), "-B", str(build_dir),
        f"-DCMAKE_BUILD_TYPE={config}",
        "-DVENOM_BUILD_TESTS=ON",
    ]
    if os.name == "nt":
        configure.extend(["-G", "Ninja"])
    return [
        Stage("Configure native release closure", configure),
        Stage("Build native compiler and closure tests", ["cmake", "--build", str(build_dir), "--config", config, "--target", "venom"]),
    ]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--build", action="store_true", help="configure and build the native compiler before source gates")
    ap.add_argument("--build-dir", type=Path, default=None)
    ap.add_argument("--config", default="Release")
    ap.add_argument("--full", action="store_true", help="run the slower final repository and publication gates")
    ap.add_argument("--report", type=Path, default=None)
    args = ap.parse_args()

    root = args.repo_root.resolve()
    build_dir = (args.build_dir or (root / "build-release-closure")).resolve()
    report_path = (args.report or (root / "artifacts/core-release-closure.json")).resolve()
    env = os.environ.copy()
    env["VENOM_TYPESCRIPT_FRONTEND"] = "embedded"
    env["VENOM_NO_PAUSE"] = "1"

    stages: list[Stage] = []
    if args.build:
        stages.extend(configure_and_build(root, build_dir, args.config))

    stages.extend([
        python_stage(root, "ABI and generated-contract lock", "tools/verify_contract_lock.py", "--repo-root", str(root)),
        python_stage(root, "Product contract generation check", "tools/generate_product_contracts.py", "--repo-root", str(root), "--check"),
        python_stage(root, "Node-free TypeScript frontend boundary", "tests/package/typescript-frontend-integration-smoke.py"),
        python_stage(root, "QuickJS/WASM authoritative contract", "tests/package/quickjs-wasm-authoritative-contract-smoke.py"),
        python_stage(root, "Generated QuickJS engine ABI", "tests/package/quickjs-engine-generated-abi-smoke.py"),
        python_stage(root, "Build-bound hardener contract", "tests/package/build-bound-hardener-smoke.py", str(root)),
        python_stage(root, "Pre-bytecode hardening boundary", "tests/package/pre-bytecode-source-hardening-smoke.py"),
        python_stage(root, "Release entrypoint policy", "tests/package/release-entrypoint-policy-smoke.py"),
    ])
    if args.build:
        executable = build_dir / ("venom.exe" if os.name == "nt" else "venom")
        closure_out = build_dir / "core-release-fixture"
        stages.extend([
            python_stage(root, "Package and metadata contract", "tests/package/product-contracts-and-minimal-metadata-smoke.py", str(executable)),
            python_stage(
                root,
                "Production release gates",
                "tests/package/production-release-gates-smoke.py",
                str(executable),
                str(root / "tests/fixtures/production-site"),
                str(closure_out),
                str(closure_out / "package.key"),
            ),
        ])

    if args.full:
        stages.extend([
            python_stage(root, "Documentation gate", "tools/documentation_gate.py"),
            python_stage(root, "Final release repository gate", "tools/final_release_gate.py", "--repo-root", str(root), "--run-smoke-tests"),
        ])

    started = time.perf_counter()
    status = "passed"
    try:
        for stage in stages:
            run_stage(stage, root, env)
    except SystemExit:
        status = "failed"
        raise
    finally:
        total = round(time.perf_counter() - started, 3)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report = {
            "schema": "VENOM_CORE_RELEASE_CLOSURE_V1",
            "status": status,
            "nodeRequired": False,
            "typescriptFrontend": "embedded",
            "buildRequested": args.build,
            "fullClosure": args.full,
            "totalSeconds": total,
            "stages": [asdict(stage) for stage in stages],
        }
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"\n[INFO] Core closure report: {report_path}")

    print(f"\n[SUCCESS] Venom core release closure passed ({total:.3f}s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
