#!/usr/bin/env python3
"""Run the complete local Venom release-closure pipeline.

This command is intentionally cross-platform.  The PowerShell/BAT/shell wrappers
are thin entry points so Windows and CI exercise the same implementation.
"""
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Sequence


@dataclass
class StepResult:
    name: str
    status: str
    seconds: float
    command: list[str]
    log: str
    detail: str = ""


def run_step(name: str, command: Sequence[str], *, cwd: Path, log_dir: Path,
             env: dict[str, str] | None = None) -> StepResult:
    log_path = log_dir / f"{len(list(log_dir.glob('*.log'))) + 1:02d}-{name}.log"
    started = time.monotonic()
    print(f"[venom] {name}: RUN")
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)
    with log_path.open("w", encoding="utf-8", errors="replace") as log:
        log.write("$ " + subprocess.list2cmdline(list(command)) + "\n\n")
        proc = subprocess.Popen(
            list(command), cwd=str(cwd), env=merged_env,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, encoding="utf-8", errors="replace",
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            print(line, end="")
            log.write(line)
        code = proc.wait()
    elapsed = time.monotonic() - started
    status = "PASS" if code == 0 else "FAIL"
    print(f"[venom] {name}: {status} ({elapsed:.1f}s)")
    return StepResult(name, status, elapsed, list(command), str(log_path), f"exit_code={code}")


def find_program(names: Sequence[str]) -> str:
    for name in names:
        found = shutil.which(name)
        if found:
            return found
    raise RuntimeError(f"required program not found: {', '.join(names)}")


def find_venom(build_dir: Path, config: str) -> Path:
    candidates = [
        build_dir / config / "venom.exe",
        build_dir / config / "venom",
        build_dir / "venom.exe",
        build_dir / "venom",
        build_dir / "bin" / config / "venom.exe",
        build_dir / "bin" / "venom",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise RuntimeError("compiled venom executable was not found under " + str(build_dir))


def main() -> int:
    parser = argparse.ArgumentParser(description="Run clean build, tests, examples, verification, and local release packaging.")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-dir", type=Path, default=Path("build/release-closure"))
    parser.add_argument("--out-dir", type=Path, default=Path("build/release-closure-output"))
    parser.add_argument("--config", default="Release")
    parser.add_argument("--generator", default="", help="optional CMake generator; empty uses the platform default")
    parser.add_argument("--parallel", type=int, default=max(1, min(8, os.cpu_count() or 2)))
    parser.add_argument("--clean", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--run-tests", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--build-examples", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--package", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--werror", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--browser-runtime-tests", action="store_true")
    parser.add_argument("--keep-going", action="store_true", help="continue independent checks after a failure")
    args = parser.parse_args()

    root = args.repo_root.resolve()
    build_dir = (root / args.build_dir).resolve() if not args.build_dir.is_absolute() else args.build_dir.resolve()
    out_dir = (root / args.out_dir).resolve() if not args.out_dir.is_absolute() else args.out_dir.resolve()
    logs = out_dir / "logs"
    if args.clean:
        shutil.rmtree(build_dir, ignore_errors=True)
        shutil.rmtree(out_dir, ignore_errors=True)
    logs.mkdir(parents=True, exist_ok=True)

    python = sys.executable
    cmake = find_program(["cmake"])
    ctest = find_program(["ctest"])
    results: list[StepResult] = []

    def step(name: str, command: Sequence[str], cwd: Path = root, env: dict[str, str] | None = None) -> bool:
        result = run_step(name, command, cwd=cwd, log_dir=logs, env=env)
        results.append(result)
        if result.status == "FAIL" and not args.keep_going:
            raise RuntimeError(f"step failed: {name}")
        return result.status == "PASS"

    status = "FAIL"
    failure = ""
    try:
        step("repository-gate", [python, str(root / "tools/final_release_gate.py"), "--repo-root", str(root), "--run-smoke-tests"])
        step("runtime-closure", [python, str(root / "tools/release_closure.py"), "--repo-root", str(root), "--require-real"])

        configure = [cmake, "-S", str(root), "-B", str(build_dir)]
        if args.generator:
            configure += ["-G", args.generator]
        configure += [f"-DVENOM_ENABLE_WERROR={'ON' if args.werror else 'OFF'}"]
        configure += [f"-DVENOM_REQUIRE_BROWSER_RUNTIME_TESTS={'ON' if args.browser_runtime_tests else 'OFF'}"]
        if platform.system() != "Windows" or args.generator.lower() in {"ninja", "unix makefiles"}:
            configure += [f"-DCMAKE_BUILD_TYPE={args.config}"]
        step("configure", configure)
        step("build", [cmake, "--build", str(build_dir), "--config", args.config, "--parallel", str(args.parallel)])

        venom = find_venom(build_dir, args.config)
        step("doctor-production", [str(venom), "doctor", "--profile", "production"])

        if args.run_tests:
            step("ctest", [ctest, "--test-dir", str(build_dir), "--build-config", args.config,
                           "--output-on-failure", "--no-tests=error"])

        if args.browser_runtime_tests:
            node = find_program(["node.exe", "node"])
            qualification_json = out_dir / "compatibility" / "framework-qualification.json"
            qualification_md = out_dir / "compatibility" / "framework-qualification.md"
            step("framework-qualification", [
                python, str(root / "tools/framework_qualification.py"),
                "--repo-root", str(root),
                "--json-out", str(qualification_json),
                "--markdown-out", str(qualification_md),
            ])
            suite_path = root / "tests/compatibility-suite.json"
            suite = json.loads(suite_path.read_text(encoding="utf-8"))
            equivalence_tool = root / "tools/browser_equivalence.py"
            for fixture in suite.get("fixtures", []):
                fixture_id = str(fixture["id"])
                site = root / str(fixture["site"])
                manifest = site / "venom.browser.json"
                dist = out_dir / "compatibility" / fixture_id
                report = out_dir / "compatibility" / f"{fixture_id}.equivalence.json"
                shutil.rmtree(dist, ignore_errors=True)
                step(f"compat-build-{fixture_id}", [str(venom), "build", str(site), "--out", str(dist), "--profile", "prod"])
                step(f"compat-equivalence-{fixture_id}", [
                    python, str(equivalence_tool), str(site), str(dist),
                    "--manifest", str(manifest), "--browser", "chromium",
                    "--node", node, "--json-out", str(report),
                ])

        if args.build_examples:
            leak_scan = root / "scripts/check-production-leaks.py"
            for example in ("protected-chess", "nova-trade", "bot-detection"):
                site = root / "examples" / example
                step(f"analyze-{example}", [str(venom), "analyze", str(site), "--format", "json"])
                for profile in ("dev", "prod"):
                    dist = out_dir / "examples" / example / profile
                    shutil.rmtree(dist, ignore_errors=True)
                    step(f"build-{example}-{profile}", [str(venom), "build", str(site), "--out", str(dist), "--profile", profile])
                    step(f"analyze-dist-{example}-{profile}", [str(venom), "analyze-dist", str(dist), "--format", "json"])
                    if profile == "prod":
                        step(f"leak-scan-{example}", [python, str(leak_scan), str(dist)])

            if args.browser_runtime_tests:
                nova_dist = out_dir / "examples" / "nova-trade" / "prod"
                benchmark_report = out_dir / "performance" / "nova-trade-runtime.json"
                step("runtime-performance-nova-trade", [
                    python, str(root / "tools/benchmark_runtime.py"),
                    "--dist", str(nova_dist), "--serve-dist", str(nova_dist),
                    "--browser", "chromium", "--runs", "5",
                    "--ready-expression", "window.venom && window.venom.exports && typeof window.venom.exports.assessOrder === 'function'",
                    "--call-expression", "arg => window.venom.exports.assessOrder(arg)",
                    "--call-argument", '{"side":"buy","orderType":"market","price":100,"quantity":1,"equity":10000,"currentPrice":100,"openPositions":1}',
                    "--warmup", "10", "--calls", "100", "--concurrency", "8",
                    "--budget-file", str(root / "contracts/runtime-benchmark.json"),
                    "--json-out", str(benchmark_report),
                ])

        if args.package:
            package_out = out_dir / "package"
            step("package-release", [
                python, str(root / "tools/package_release.py"),
                "--repo-root", str(root), "--build-dir", str(build_dir),
                "--venom", str(venom), "--config", args.config,
                "--out", str(package_out), "--archive", "zip",
                "--sign", "dev-sha256", "--dev-insecure-key", "release-closure-local-only",
                "--release-channel", "development", "--release-sequence", "1",
            ])

        status = "PASS" if all(r.status == "PASS" for r in results) else "FAIL"
    except Exception as exc:
        failure = str(exc)
        status = "FAIL"

    version_command: list[str] = []
    try:
        version_command = [str(find_venom(build_dir, args.config)), "--version"]
    except RuntimeError:
        pass

    report = {
        "schema": "VENOM_RELEASE_CLOSURE_RUN_V1",
        "status": status,
        "version_command": version_command,
        "platform": platform.platform(),
        "build_dir": str(build_dir),
        "output_dir": str(out_dir),
        "failure": failure,
        "steps": [asdict(r) for r in results],
    }
    report_path = out_dir / "release-closure-report.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"[venom] RELEASE CLOSURE: {status}")
    print(f"[venom] report: {report_path}")
    if failure:
        print(f"[venom] failure: {failure}", file=sys.stderr)
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
