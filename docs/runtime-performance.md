# Runtime performance and size budgets

Venom treats runtime size and startup performance as release properties. The checked-in policy is stored in `contracts/runtime-performance.json`.

Run the embedded-runtime report:

```bash
python tools/runtime_performance.py --format json --json-out runtime-performance.json
```

Include a generated distribution:

```bash
python tools/runtime_performance.py --dist dist --format json
```

The report records raw, gzip, and Brotli QuickJS/WASM sizes, the release ABI export count, and optional distribution totals. A budget violation exits with release-policy code `60`.

The default production profile remains `balanced`: Emscripten uses `-O3 -flto`, and Binaryen uses `-O3`. The size profile uses `-Oz` end to end; the performance profile uses `-O3 -flto` followed by Binaryen `-O4`.

`wasm-opt` is resolved from the active EMSDK, the repository-local SDK, or `PATH`. Its absence is reported but does not make an otherwise valid Emscripten build fail.

Size budgets are regression ceilings, not guarantees of browser startup latency. Browser timing remains fixture-, machine-, and browser-dependent and is measured separately by `tools/benchmark_runtime.py`.
