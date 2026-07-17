# Lazy protected loading

Venom can keep protected QuickJS registries outside the main VBC package and activate only the registry group required by a protected call.

```toml
[runtime.lazy_loading]
enabled = true
preload = ["verifySession"]
```

## Chunk layout

A project with both protected modules and individually protected functions can emit:

```text
assets/app/
├── <hash>.vqc
└── <hash>.vqc
```

When only one registry group exists, the compact name remains `registry.<hash>.vqc`.

The protected manifest maps every opaque candidate ID to its owning chunk. On the first call, the worker fetches only that chunk, verifies its SHA-256 digest, activates the native QuickJS record, and caches the ready state in memory. Concurrent calls share one activation promise per chunk.

Protected-module dependencies remain in the same module-graph chunk so dependency order is preserved. Individually lowered protected functions are held in a separate function registry chunk. Preload hints activate only the chunk containing the named export.

Missing, oversized, malformed, or digest-mismatched chunks fail closed with `lazy-chunk-failed`.
