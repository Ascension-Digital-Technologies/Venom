# JavaScript Playground isolation

Example 7 is a local development tool. Submitted source is compiled by a loopback-only native Venom process and the authenticated bytecode is executed in an ephemeral QuickJS/WASM context.

## Per-execution limits

Each run uses a fresh context with bounded heap, stack, interrupt budget, pending jobs, bridge input, result output, console event count, console bytes, serialization depth, collection sizes, and string lengths. The context and temporary buffers are released after every run.

## Loopback compiler endpoint

The launcher binds only to `127.0.0.1`, creates a random capability token for every launch, validates the request Host and Origin/Referer, requires JSON POST requests, limits source/request sizes, allows at most two concurrent compiler processes, uses a reduced environment, and enforces a compilation timeout.

This endpoint is not emitted into production distributions and is not intended to be exposed as a public multi-user service.
