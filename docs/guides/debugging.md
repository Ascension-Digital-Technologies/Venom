# Debugging

> **Applies to:** Venom 1.1.0

Use the production-grade build path to diagnose application behavior while still executing protected code through the real TurboJS/WASM path. Do not debug a production failure by disabling verification or introducing host-JavaScript fallback behavior.

## First checks

```powershell
venom doctor --profile production
venom compatibility check path\to\site
venom build path\to\site --profile prod --out build\dev-dist
venom analyze build\dev-dist
```

Inspect the browser console, network panel, worker errors, route requests, and generated compatibility report.

## Common failure categories

- unsupported browser API used from protected code;
- missing or incorrectly routed asset;
- non-JSON-safe bridge value;
- protected call payload exceeding limits;
- stale or mismatched runtime/package assets;
- JS hardener not installed for production;
- package, runtime, or loader integrity failure;
- incorrect annotation boundary around framework or DOM code.

## Bridge failures

Protected functions accept bounded JSON-safe values. DOM nodes, functions, symbols, cyclic objects, framework proxies, and browser-native handles must remain browser-side. Reduce the payload to plain objects, arrays, strings, booleans, finite numbers, and null.

## Production failures

Run:

```powershell
venom verify-runtime dist
venom verify dist
```

Never replace a failed production runtime with a development asset. Rebuild the complete distribution so all hashes and package bindings are generated together.

## Minimal reproductions

When isolating compatibility problems, preserve the same annotations, module graph, route shape, and browser API usage. A reduced fixture that removes the execution boundary may hide the actual issue.
