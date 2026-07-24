# TurboJS Long-Run Benchmark

This example provides a one-click benchmark dashboard for Venom's protected TurboJS/WASM runtime. It executes a sequence of independent workloads inside protected engine code and updates the browser after every completed subsystem.

## What it measures

- floating-point arithmetic
- integer and bitwise operations
- branches and control flow
- function-call throughput
- JavaScript arrays
- typed arrays
- object property access and mutation
- strings and regular expressions
- JSON serialization and parsing
- `Map` and `Set`
- `Math` built-ins
- array sorting
- allocation and garbage-collection pressure

The displayed duration includes the protected browser bridge overhead as well as TurboJS execution. The checksum column prevents the benchmark body from becoming dead work and helps detect output drift between builds.

## Presets

- **Quick:** short engine health check.
- **Standard:** balanced full-engine profile.
- **Endurance:** sustained long-running workload intended for optimization comparisons.

Each case runs as a separate protected call. This keeps the UI responsive between tests and identifies the exact subsystem that failed or exceeded a runtime limit.

## Run on Windows

```bat
scripts\windows\build-and-launch-turbojs-benchmark.bat
```

The launcher builds the native compiler, builds and verifies the production distribution, opens the benchmark in the browser, and remains open if any build step fails.

## Interpreting results

Compare results using the same machine, browser, Venom build profile, preset, and power state. Run the suite at least twice; the second run is usually the better steady-state comparison because the worker and protected runtime are already initialized. This is an engineering benchmark rather than a standardized cross-engine score.
