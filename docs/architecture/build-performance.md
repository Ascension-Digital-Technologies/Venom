# Build performance and incremental compilation

Venom 1.3.1 records every internal build phase and uses content-addressed caches for structural TypeScript lowering, native TurboJS bytecode, and production JavaScript hardening.

## Cache identity

Cache keys include the source bytes and every option that changes emitted output. Structural TypeScript entries include the compiler/frontend version and JSX-lowering contract. TurboJS entries include bytecode ABI, module mode, source identity, metadata policy, and the complete protected module source set. Production diversification remains nondeterministic unless `SOURCE_DATE_EPOCH` or an explicit diversification seed is supplied; development builds use a project-derived identity so unchanged warm builds are reusable.

## Reports

Every successful build writes a private report beside the distribution:

```text
.venom/<distribution>/build-performance.json
```

It contains wall-clock time excluding interactive presentation delays, per-phase durations, and cache hit/miss/write counters.

## Benchmarking

```bash
python tools/benchmark_build.py \
  --venom build/venom \
  --site examples/typescript-showcase \
  --profile prod \
  --cold-budget 10 \
  --warm-budget 3
```

The benchmark creates isolated output/cache directories, performs one clean and one unchanged build, and emits machine-readable JSON. Performance budgets should be calibrated on dedicated CI runners rather than developer laptops.

## Generated runtime files

Runtime bundling and C++ embedding use write-if-changed publication. Identical generated content preserves the destination timestamp, avoiding needless recompilation of large generated translation units.
