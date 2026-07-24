#!/usr/bin/env python3
"""Build and verify every certifiable Venom example from the canonical registry."""
from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

from venom_tools.examples import ExampleSpec, load_example_registry
from venom_tools.io import sha256_file, write_json
from venom_tools.process import CommandResult, run_command


def result_dict(result: CommandResult) -> dict:
    return {
        "command": list(result.command),
        "passed": result.passed,
        "returnCode": result.returncode,
        "durationMs": result.duration_ms,
        "output": result.output[-30000:],
    }


def write_markdown(result: dict, path: Path) -> None:
    lines = [
        "# Venom Example Certification",
        "",
        f"**Result:** {'PASS' if result['passed'] else 'FAIL'}  ",
        f"**Generated:** {result['generatedAt']}  ",
        "",
        "| Example | Build | Verify | Runtime | Leak scan | Duration |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for item in result["examples"]:
        status = lambda key: "PASS" if item.get(key) else "FAIL"
        lines.append(
            f"| `{item['id']}` | {status('buildPassed')} | {status('verifyPassed')} | "
            f"{status('runtimePassed')} | {status('leakScanPassed')} | {item['durationMs'] / 1000:.2f}s |"
        )
    lines += ["", "## Failures", ""]
    failed = [item for item in result["examples"] if not item["passed"]]
    if not failed:
        lines.append("No failures.")
    for item in failed:
        lines += [f"### {item['id']}", "", "```text", item.get("failureOutput", "")[-8000:], "```", ""]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def skipped_step(reason: str) -> dict:
    return {"passed": True, "returnCode": 0, "durationMs": 0, "output": reason, "command": []}


def certify_example(root: Path, venom: Path, dist_root: Path, spec: ExampleSpec, seed: int, env: dict[str, str]) -> dict:
    site = root / spec.path
    dist = dist_root / spec.id
    shutil.rmtree(dist, ignore_errors=True)
    started = time.perf_counter()
    steps: list[tuple[str, dict]] = []

    build_result = result_dict(run_command(
        [venom, "build", site, "--out", dist, "--profile", "prod", "--seed", str(seed)],
        cwd=root,
        timeout=600,
        env=env,
    ))
    steps.append(("build", build_result))

    verify = {"passed": False, "output": "build failed"}
    runtime = {"passed": False, "output": "build failed"}
    leak = {"passed": False, "output": "build failed"}
    if build_result["passed"]:
        verify = result_dict(run_command([venom, "verify", dist, "--target", "browser"], cwd=root, timeout=600, env=env))
        steps.append(("verify", verify))
        if spec.requires_real_turbojs:
            runtime = result_dict(run_command(
                [venom, "verify-runtime", dist, "--target", "browser", "--require-real-engine"],
                cwd=root,
                timeout=600,
                env=env,
            ))
        else:
            runtime = skipped_step("runtime verification not required by registry")
        steps.append(("runtime", runtime))
        if spec.leak_scan:
            leak = result_dict(run_command([sys.executable, root / "tools/check_production_leaks.py", dist], cwd=root, timeout=600, env=env))
        else:
            leak = skipped_step("leak scan not required by registry")
        steps.append(("leak", leak))

    passed = all(step["passed"] for _, step in steps)
    failure = "\n\n".join(f"[{name}]\n{step['output']}" for name, step in steps if not step["passed"])
    return {
        "id": spec.id,
        "displayName": spec.display_name,
        "path": spec.path,
        "passed": passed,
        "buildPassed": build_result["passed"],
        "verifyPassed": verify["passed"],
        "runtimePassed": runtime["passed"],
        "leakScanPassed": leak["passed"],
        "durationMs": round((time.perf_counter() - started) * 1000),
        "steps": dict(steps),
        "failureOutput": failure,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--venom", type=Path, required=True)
    parser.add_argument("--contract", type=Path, default=Path("contracts/examples.json"))
    parser.add_argument("--output-dir", type=Path, default=Path("build/example-certification"))
    parser.add_argument("--keep-dists", action="store_true")
    args = parser.parse_args()

    root = args.repo_root.resolve()
    venom = args.venom.resolve()
    contract_path = args.contract if args.contract.is_absolute() else root / args.contract
    output = args.output_dir if args.output_dir.is_absolute() else root / args.output_dir
    registry = load_example_registry(root, contract_path)
    output.mkdir(parents=True, exist_ok=True)
    dist_root = output / "distributions"
    dist_root.mkdir(exist_ok=True)
    env = os.environ.copy()
    env.setdefault("SOURCE_DATE_EPOCH", "1704067200")

    results = []
    for index, spec in enumerate(registry.certifiable()):
        item = certify_example(root, venom, dist_root, spec, 2_000_000 + index, env)
        results.append(item)
        write_json(output / f"{spec.id}.json", item)
        print(f"[{'PASS' if item['passed'] else 'FAIL'}] {spec.id} ({item['durationMs'] / 1000:.2f}s)", flush=True)

    result = {
        "schema": "VENOM_EXAMPLE_CERTIFICATION_RESULT_V2",
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "passed": all(item["passed"] for item in results),
        "profile": registry.build_profile,
        "contract": {
            "path": str(contract_path.relative_to(root)),
            "sha256": sha256_file(contract_path),
            "schema": "VENOM_EXAMPLE_REGISTRY_V2",
        },
        "examples": results,
    }
    write_json(output / "summary.json", result)
    write_markdown(result, output / "summary.md")
    if not args.keep_dists:
        shutil.rmtree(dist_root, ignore_errors=True)
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
