# Runtime performance benchmarking

> **Audience:** integrators, release engineers, and performance reviewers  
> **Applies to:** Venom 1.0.1
Venom measures runtime performance as evidence tied to a specific distribution, browser, machine, payload, and protected entry point. The benchmark tooling is designed for regression detection—not universal latency claims.

## Metrics

`tools/benchmark_runtime.py` can record:

- distribution, JavaScript, WASM, and package size;
- browser launch time;
- navigation and runtime-ready latency;
- DOM content-loaded, load, and response-end timing;
- JavaScript heap usage when the browser exposes it;
- protected-call sequential latency;
- concurrent batch latency;
- protected-call throughput;
- arbitrary command timing;
- comparison against a previous baseline.

## Benchmark a production distribution

```powershell
python tools\benchmark_runtime.py `
  --dist dist `
  --serve-dist dist `
  --browser chromium `
  --runs 7 `
  --ready-expression "window.venom && window.venom.exports && typeof window.venom.exports.assessOrder === 'function'" `
  --call-expression "arg => window.venom.exports.assessOrder(arg)" `
  --call-argument '{"side":"buy","orderType":"market","price":100,"quantity":1,"equity":10000,"currentPrice":100,"openPositions":1}' `
  --warmup 10 `
  --calls 100 `
  --concurrency 8 `
  --budget-file contracts\runtime-benchmark.json `
  --json-out build\runtime-benchmark.json
```

The tool starts a no-cache local HTTP server when `--serve-dist` is supplied.

## Baseline comparison

```powershell
python tools\benchmark_runtime.py `
  --serve-dist dist `
  --baseline evidence\previous-runtime-benchmark.json `
  --max-regression-percent 10 `
  --json-out evidence\current-runtime-benchmark.json
```

Baseline comparisons cover startup, readiness, protected-call latency, and artifact sizes when those measurements are available in both reports.

## Release closure

Browser-enabled release closure benchmarks NOVA TRADE automatically:

```powershell
.\scripts\release-closure.ps1 -BrowserRuntimeTests
```

The report is written to:

```text
build/release-closure-output/performance/nova-trade-runtime.json
```

## Interpreting results

A benchmark result is meaningful only with its context:

- Venom version and build profile;
- source and distribution hashes;
- protected function and payload;
- browser name and version;
- operating system and CPU;
- warmup count, measured calls, and concurrency;
- whether developer tools, antivirus, or power-saving modes were active.

The checked-in budgets are intentionally broad CI regression ceilings. They are not promises that every application or machine will achieve the same latency.
