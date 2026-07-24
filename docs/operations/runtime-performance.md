# Runtime startup performance

Venom 2.0.0 emits preload hints for the loader, browser runtime, protected package, TurboJS/WASM module, and worker. These hints start immutable asset transfers earlier without introducing a plaintext application cache. Normal HTTP caching remains available for content-addressed production assets.

The authoritative performance policy is `contracts/runtime-performance.json`. Browser evidence can be summarized with:

```bash
python tools/summarize_startup.py browser-results.json \
  --json-out startup-summary.json \
  --markdown-out startup-summary.md
```

Certification fails when required startup phases are missing or when median or p95 budgets regress. Performance data is advisory on developer machines and authoritative in controlled browser CI.
