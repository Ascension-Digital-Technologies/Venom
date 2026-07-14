#!/usr/bin/env python3
"""Measure Venom artifacts, commands, browser startup, and protected-call performance."""
from __future__ import annotations

import argparse
import functools
import http.server
import json
import math
import os
import shlex
import statistics
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Any

SCHEMA = "venom.performance.v3"


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, format: str, *args: object) -> None:
        pass

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def start_server(root: Path, host: str = "127.0.0.1", port: int = 0):
    handler = functools.partial(QuietHandler, directory=str(root))
    server = http.server.ThreadingHTTPServer((host, port), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, f"http://{host}:{server.server_address[1]}/index.html"


def percentile(values: list[float], q: float) -> float | None:
    if not values:
        return None
    ordered = sorted(float(v) for v in values)
    pos = (len(ordered) - 1) * q
    lo = math.floor(pos)
    hi = math.ceil(pos)
    return ordered[lo] if lo == hi else ordered[lo] * (hi - pos) + ordered[hi] * (pos - lo)


def stats(samples: list[float]) -> dict[str, Any]:
    if not samples:
        return {"runs": 0, "median_ms": None, "mean_ms": None, "p95_ms": None, "min_ms": None, "max_ms": None}
    values = [float(x) for x in samples]
    return {
        "runs": len(values),
        "median_ms": statistics.median(values),
        "mean_ms": statistics.mean(values),
        "p95_ms": percentile(values, 0.95),
        "min_ms": min(values),
        "max_ms": max(values),
    }


def run_timed(command: list[str], runs: int, cwd: str | None = None) -> dict[str, Any]:
    samples: list[float] = []
    for _ in range(runs):
        started = time.perf_counter()
        cp = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        elapsed = (time.perf_counter() - started) * 1000
        if cp.returncode:
            raise RuntimeError(f"command failed ({cp.returncode}): {' '.join(command)}\n{cp.stderr[-2000:]}")
        samples.append(elapsed)
    return stats(samples)


def artifact_report(item: str) -> dict[str, Any]:
    root = Path(item)
    files = [f for f in root.rglob("*") if f.is_file()]
    return {
        "path": str(root),
        "bytes": sum(f.stat().st_size for f in files),
        "files": len(files),
        "wasm_bytes": sum(f.stat().st_size for f in files if f.suffix == ".wasm"),
        "js_bytes": sum(f.stat().st_size for f in files if f.suffix == ".js"),
        "package_bytes": sum(f.stat().st_size for f in files if f.suffix in (".vbc", ".pax")),
    }


def browser_samples(
    url: str,
    runs: int,
    browser: str,
    ready_expression: str,
    call_expression: str | None,
    call_argument: Any,
    warmup: int,
    calls: int,
    concurrency: int,
) -> dict[str, Any]:
    runner = r'''
const { chromium, firefox, webkit } = require('playwright');
(async () => {
  const cfg = JSON.parse(process.argv[1]);
  const launcher = {chromium, firefox, webkit}[cfg.browser];
  if (!launcher) throw new Error(`unsupported browser: ${cfg.browser}`);
  const launchStart = performance.now();
  const browser = await launcher.launch({headless:true});
  const launchMs = performance.now() - launchStart;
  const rows = [];
  for (let i=0; i<cfg.runs; i++) {
    const context = await browser.newContext({serviceWorkers:'block'});
    const page = await context.newPage();
    const navStart = performance.now();
    await page.goto(cfg.url, {waitUntil:'networkidle'});
    const navigationWall = performance.now() - navStart;
    const readyStart = performance.now();
    if (cfg.readyExpression) await page.waitForFunction(cfg.readyExpression, null, {timeout:30000});
    const readyMs = performance.now() - readyStart;
    const nav = await page.evaluate(() => {
      const n = performance.getEntriesByType('navigation')[0];
      return {
        dom: n ? n.domContentLoadedEventEnd : 0,
        load: n ? n.loadEventEnd : 0,
        responseEnd: n ? n.responseEnd : 0,
        heap: performance.memory ? performance.memory.usedJSHeapSize : null,
      };
    });
    const row = {navigation_wall_ms:navigationWall, runtime_ready_ms:readyMs, ...nav};
    if (cfg.callExpression) {
      const callResult = await page.evaluate(async cfg => {
        const invoke = new Function('arg', `return (${cfg.callExpression})(arg)`);
        for (let i=0; i<cfg.warmup; i++) await invoke(cfg.callArgument);
        const sequential=[];
        for (let i=0; i<cfg.calls; i++) {
          const t=performance.now(); await invoke(cfg.callArgument); sequential.push(performance.now()-t);
        }
        const concurrent=[];
        const batches=Math.ceil(cfg.calls/cfg.concurrency);
        const throughputStart=performance.now();
        for (let b=0; b<batches; b++) {
          const count=Math.min(cfg.concurrency,cfg.calls-b*cfg.concurrency);
          const t=performance.now();
          await Promise.all(Array.from({length:count},()=>invoke(cfg.callArgument)));
          concurrent.push(performance.now()-t);
        }
        const throughputElapsed=performance.now()-throughputStart;
        return {sequential, concurrent, throughput_calls_per_second: throughputElapsed > 0 ? cfg.calls/(throughputElapsed/1000) : null};
      }, cfg);
      row.calls=callResult;
    }
    rows.push(row);
    await context.close();
  }
  await browser.close();
  console.log(JSON.stringify({launch_ms:launchMs, rows}));
})().catch(error => { console.error(error?.stack || error); process.exit(1); });
'''
    cfg = {
        "url": url,
        "runs": runs,
        "browser": browser,
        "readyExpression": ready_expression,
        "callExpression": call_expression,
        "callArgument": call_argument,
        "warmup": warmup,
        "calls": calls,
        "concurrency": concurrency,
    }
    cp = subprocess.run(["node", "-e", runner, json.dumps(cfg)], capture_output=True, text=True)
    if cp.returncode:
        raise RuntimeError(f"browser benchmark failed ({cp.returncode}): {cp.stderr[-3000:]}")
    raw = json.loads(cp.stdout.strip().splitlines()[-1])
    rows = raw["rows"]
    result: dict[str, Any] = {
        "browser": browser,
        "browser_launch_ms": raw["launch_ms"],
        "navigation_wall": stats([r["navigation_wall_ms"] for r in rows]),
        "runtime_ready": stats([r["runtime_ready_ms"] for r in rows]),
        "dom_content_loaded": stats([r["dom"] for r in rows]),
        "load": stats([r["load"] for r in rows]),
        "response_end": stats([r["responseEnd"] for r in rows]),
    }
    heaps = [r["heap"] for r in rows if r.get("heap") is not None]
    if heaps:
        result["heap_bytes"] = {"median": statistics.median(heaps), "max": max(heaps)}
    call_rows = [r["calls"] for r in rows if r.get("calls")]
    if call_rows:
        sequential = [v for r in call_rows for v in r["sequential"]]
        concurrent = [v for r in call_rows for v in r["concurrent"]]
        throughput = [r["throughput_calls_per_second"] for r in call_rows if r["throughput_calls_per_second"] is not None]
        result["protected_calls"] = {
            "sequential_latency": stats(sequential),
            "concurrent_batch_latency": stats(concurrent),
            "throughput_calls_per_second": {
                "runs": len(throughput),
                "median": statistics.median(throughput),
                "min": min(throughput),
                "max": max(throughput),
            },
            "warmup_calls": warmup,
            "measured_calls_per_run": calls,
            "concurrency": concurrency,
        }
    return result


def get_path(obj: Any, path: str) -> Any:
    cur = obj
    for part in path.split("."):
        cur = cur[int(part)] if isinstance(cur, list) else cur[part]
    return cur


def apply_budget(report: dict[str, Any], path: str, operator: str, threshold: float) -> bool:
    actual = float(get_path(report, path))
    passed = actual <= threshold if operator == "max" else actual >= threshold
    report["gates"].append({"kind": "absolute", "metric": path, "operator": operator, "actual": actual, "threshold": threshold, "passed": passed})
    return passed


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--dist", action="append", default=[])
    p.add_argument("--url")
    p.add_argument("--serve-dist", type=Path, help="Serve this distribution locally and benchmark its index.html")
    p.add_argument("--browser", choices=("chromium", "firefox", "webkit"), default="chromium")
    p.add_argument("--runs", type=int, default=7)
    p.add_argument("--ready-expression", default="window.venom && typeof window.venom.ready === 'function'")
    p.add_argument("--call-expression", help="Async JS callable expression, e.g. 'arg => window.venom.call(\"score\", arg)'")
    p.add_argument("--call-argument", default="null", help="JSON value passed to the call expression")
    p.add_argument("--warmup", type=int, default=10)
    p.add_argument("--calls", type=int, default=100)
    p.add_argument("--concurrency", type=int, default=8)
    p.add_argument("--command", action="append", default=[])
    p.add_argument("--command-runs", type=int, default=5)
    p.add_argument("--cwd")
    p.add_argument("--baseline")
    p.add_argument("--max-regression-percent", type=float, default=10.0)
    p.add_argument("--budget", action="append", default=[], help="metric=max or metric>=min")
    p.add_argument("--budget-file", type=Path)
    p.add_argument("--json-out")
    a = p.parse_args()
    try:
        call_argument = json.loads(a.call_argument)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"invalid --call-argument JSON: {exc}")

    report: dict[str, Any] = {
        "schema": SCHEMA,
        "environment": {"platform": sys.platform, "cpu_count": os.cpu_count(), "python": sys.version.split()[0]},
        "artifacts": [artifact_report(x) for x in a.dist],
        "commands": [],
        "browser": None,
        "gates": [],
        "passed": True,
    }
    for command in a.command:
        argv = shlex.split(command, posix=os.name != "nt")
        report["commands"].append({"command": argv, "timing": run_timed(argv, a.command_runs, a.cwd)})
    server = None
    benchmark_url = a.url
    try:
        if a.serve_dist:
            dist_root = a.serve_dist.resolve()
            if not (dist_root / "index.html").is_file():
                raise RuntimeError(f"served distribution is missing index.html: {dist_root}")
            server, benchmark_url = start_server(dist_root)
        if benchmark_url:
            report["browser"] = browser_samples(benchmark_url, a.runs, a.browser, a.ready_expression, a.call_expression, call_argument, a.warmup, a.calls, a.concurrency)
    finally:
        if server is not None:
            server.shutdown(); server.server_close()

    budgets: list[tuple[str, str, float]] = []
    if a.budget_file:
        policy = json.loads(a.budget_file.read_text(encoding="utf-8"))
        report["budget_profile"] = policy.get("profile")
        for item in policy.get("budgets", []):
            budgets.append((item["metric"], item.get("operator", "max"), float(item["threshold"])))
    for spec in a.budget:
        if ">=" in spec:
            path, raw = spec.rsplit(">=", 1); budgets.append((path, "min", float(raw)))
        else:
            path, raw = spec.rsplit("=", 1); budgets.append((path, "max", float(raw)))
    for path, operator, threshold in budgets:
        try:
            report["passed"] &= apply_budget(report, path, operator, threshold)
        except (KeyError, IndexError, TypeError, ValueError) as exc:
            report["gates"].append({"kind": "absolute", "metric": path, "operator": operator, "threshold": threshold, "passed": False, "error": str(exc)})
            report["passed"] = False

    if a.baseline:
        base = json.loads(Path(a.baseline).read_text(encoding="utf-8"))
        candidates = []
        if report["browser"] and base.get("browser"):
            candidates += ["browser.navigation_wall.median_ms", "browser.runtime_ready.median_ms"]
            if report["browser"].get("protected_calls") and base["browser"].get("protected_calls"):
                candidates += ["browser.protected_calls.sequential_latency.p95_ms"]
        if report["artifacts"] and base.get("artifacts"):
            candidates += ["artifacts.0.bytes", "artifacts.0.wasm_bytes", "artifacts.0.js_bytes"]
        for path in candidates:
            before = float(get_path(base, path)); after = float(get_path(report, path))
            regression = 0 if before == 0 else (after - before) * 100 / before
            passed = regression <= a.max_regression_percent
            report["gates"].append({"kind": "baseline", "metric": path, "baseline": before, "actual": after, "regression_percent": regression, "maximum_percent": a.max_regression_percent, "passed": passed})
            report["passed"] &= passed

    text = json.dumps(report, indent=2, sort_keys=True)
    print(text)
    if a.json_out:
        out = Path(a.json_out); out.parent.mkdir(parents=True, exist_ok=True); out.write_text(text + "\n", encoding="utf-8")
    return 0 if report["passed"] else 60


if __name__ == "__main__":
    raise SystemExit(main())
