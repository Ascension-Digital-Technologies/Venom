# Protected runtime

The protected runtime is worker-owned and fail-closed. The browser does not execute protected source through `eval`, `new Function`, or a silent host fallback.

Core components:

- dedicated worker;
- QuickJS engine compiled to WebAssembly;
- WASM package decoder;
- bytecode registry and module resolver;
- capability-limited host bridge;
- resource, replay, lifecycle, and audit policies;
- sanitized result/error channel.
