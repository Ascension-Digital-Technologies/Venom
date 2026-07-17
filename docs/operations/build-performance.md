# Build performance and compiler caching

> **Audience:** contributors, CI maintainers, and release engineers  
> **Applies to:** Venom 1.1.0
Venom keeps the production runtime fully optimized while avoiding unnecessary local rebuild cost. The build system supports precompiled standard-library headers, MSVC multi-process compilation, compiler launchers, and independent QuickJS IPO control.

## Recommended Windows build

```powershell
cmake -S . -B build -DVENOM_COMPILER_CACHE=auto -DVENOM_ENABLE_PCH=ON
cmake --build build --config Release --parallel
```

`sccache` is preferred on Windows and CI. When no supported cache is installed, `auto` continues without one.

## Important controls

| Option | Default | Purpose |
|---|---:|---|
| `VENOM_COMPILER_CACHE` | `auto` | Select `sccache`, `ccache`, `none`, or automatic discovery |
| `VENOM_ENABLE_PCH` | `ON` | Precompile stable C++ standard-library headers for `venom_core` |
| `VENOM_ENABLE_MSVC_MP` | `ON` | Enable `/MP` for Visual Studio targets |
| `VENOM_ENABLE_IPO` | `ON` | Enable release IPO/LTO for Venom targets |
| `VENOM_ENABLE_QUICKJS_IPO` | `OFF` | Opt the large vendored QuickJS library into IPO explicitly |

QuickJS IPO is intentionally independent. Turning it off improves clean-link and local iteration time while retaining ordinary release optimization. Enable it only after measuring a workload-specific runtime benefit.

## Reproducible measurement

```powershell
python tools/build_performance.py --generator "Visual Studio 18 2026" --parallel 8 --json-out build/performance.json
```

The report records clean configure/build time, no-op rebuild time, touched-source incremental rebuild time, executable size, build settings, platform details, and available cache statistics.

For comparable measurements, use a clean checkout, close unrelated build processes, keep the same generator and parallelism, and run at least three samples. Publish medians rather than the best result.

## Measuring cold and warm builds

Use `venom build <site> --profile prod --no-cache` for a cold compiler measurement. Run the normal command twice to measure warm-cache performance. `--verbose` prints native-hardener and QuickJS bytecode cache hits and misses.
