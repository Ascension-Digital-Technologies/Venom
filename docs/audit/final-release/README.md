# Final release readiness

Venom 2.0.0 uses `contracts/final-release.json` as the source-tree release contract. Generate final readiness evidence with:

```bash
python tools/final_readiness_report.py \
  --json-out build/final-readiness.json \
  --markdown-out build/final-readiness.md
```

The report validates version consistency, production-only profile policy, the eight-example inventory, browser-engine requirements, runtime and release contracts, startup telemetry, SDK compatibility, C++17 portability, documentation, and repository structure. Binary, browser, and platform certification evidence remains produced by the dedicated certification workflows.
