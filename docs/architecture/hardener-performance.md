# JavaScript hardener performance

Venom uses one persistent, isolated TurboJS runtime for the native JavaScript hardener during a compiler process. The runtime loads the embedded Terser and JavaScript Obfuscator bundles once, then reuses them for protected registries, runtime assets, the TurboJS engine adapter, workers, and the bootstrap loader.

## Safety boundaries

- Calls are serialized because a TurboJS runtime is not entered concurrently.
- Invocation source and output globals are cleared after every call.
- Garbage collection runs periodically.
- Any hardener exception destroys the runtime; the next call starts from a clean realm.
- The compiler cache identity includes the hardener policy schema and tool versions.

## Size-aware policy

Generated browser assets can exceed hundreds of kilobytes. Transformations such as control-flow flattening and dead-code injection have nonlinear costs on large generated programs. Venom therefore keeps Terser compression, mangling, identifier diversification, string protection, and release validation while reducing pathological transforms for large assets.

Protected source that is immediately compiled into authenticated TurboJS bytecode uses a bytecode-oriented policy. Expensive browser-source obfuscation is avoided because the source is not shipped as executable browser JavaScript; protection is provided by native TurboJS bytecode, VBC section encryption, integrity binding, and fail-closed runtime verification.

## Telemetry

The build summary reports:

- hardener calls
- runtime initializations
- hardener execution time
- cache hits and misses

Verbose builds also print input sizes for cache misses, which makes unexpectedly expensive assets easy to identify.
