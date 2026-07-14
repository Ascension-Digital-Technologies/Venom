# Roadmap

## Current priorities

- Complete browser-equivalence coverage for representative static, module, SPA, worker, and framework sites.
- Expand compatibility evidence and framework guidance.
- Measure startup, bridge, route-hydration, package-size, and build-time performance.
- Improve QuickJS native and WASM build caching.
- Continue reducing generated-runtime build cost without weakening integrity checks.
- Publish stable package-format and bridge-protocol compatibility guarantees.

## Release criteria

A stable public release requires:

- clean Windows and Linux Release builds;
- complete CTest closure;
- successful dev/prod builds for all flagship examples;
- production leak scans;
- verified real QuickJS/WASM provenance;
- signed release metadata;
- current documentation and compatibility evidence.
