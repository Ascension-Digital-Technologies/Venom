# Capability analysis

Venom 1.26 introduces a syntax-aware application capability graph used by compatibility diagnostics.

```bash
venom analyze site
venom analyze site --format json
venom compatibility check site --format json
```

The analyzer tokenizes JavaScript while excluding comments and string literals, scans inline HTML scripts, and records precise file, line, and column locations. It identifies module patterns, selected browser APIs, Node-only dependencies, dynamic source execution, and framework signatures.

The capability graph currently reports:

- syntax features such as classes, async functions, templates, `await`, dynamic evaluation, and the Function constructor;
- module behavior including static imports, dynamic imports, exports, and `import.meta`;
- selected browser APIs such as fetch, observers, workers, WebSocket, WebRTC, WebGPU, and shared memory;
- Node-only dependencies such as unguarded `require()` and `process`;
- framework signatures for React, Vue, Svelte, Alpine, and HTMX.

Guarded CommonJS branches inside UMD bundles are classified separately from unconditional Node dependencies. Comments and ordinary string contents do not produce capability findings.

## Scope

This is a syntax-aware capability graph foundation, not a complete JavaScript semantic analyzer. It does not yet perform full scope resolution, type inference, control-flow analysis, or package-manager dependency resolution. Real-browser fixture evidence remains the authority for compatibility claims.

Future work will use the graph to select modular host-runtime components and generate tighter per-route capability policies.
