# Compiler Pipeline

> **Applies to:** Venom 1.0.1

Venom transforms a normal web project into a static production distribution while separating browser-native responsibilities from protected QuickJS/WASM execution.

## 1. Site discovery

The compiler reads HTML, CSS, JavaScript, modules, routes, and static assets under the configured site root. It resolves local module references, route entry points, stylesheets, images, fonts, and browser-side dependencies.

## 2. Compatibility analysis

Venom identifies code that must remain in the browser because it depends on the DOM, framework lifecycle, browser objects, event instances, or APIs not available in the protected runtime. Explicit annotations override or confirm the planned realm where supported.

## 3. Protected-realm planning

Unannotated JavaScript is protected by default. Function and module plans determine which declarations are extracted, which dependencies must move with them, and which exports become available through the public asynchronous API.

## 4. JavaScript rewriting

Browser chunks are rewritten to call protected exports. Protected modules are transformed into QuickJS-compatible records. Generated bridge registries use opaque build-specific capability identities instead of source-level function names on the transport.

## 5. Bytecode compilation

Protected scripts and modules are compiled with the pinned QuickJS engine. Canonical records are validated against the expected QuickJS ABI, wrapped in build-specific `VQJSE006` envelopes, and bound to route, source, and execution order.

## 6. Package construction

The compiler writes the VBC package with polymorphic section ordering, padding, identifiers, route mappings, metadata, and integrity records. Production builds can vary physical layout while preserving application behavior.

## 7. Runtime generation

Venom emits the loader, worker, runtime JavaScript, QuickJS engine bridge, package/route WASM assets, stylesheets, and browser chunks. Production JavaScript is minified, mangled, role-specifically obfuscated, and stripped of source-only diagnostics.

## 8. Asset binding

Generated assets are content addressed and hash-bound. The loader, package, workers, runtime modules, stylesheets, and WebAssembly binaries must agree on the exact build identity.

## 9. Release verification

Production qualification checks hardener output, runtime provenance, source leakage, package contracts, compatibility scenarios, browser equivalence, and signed release metadata.

See [Architecture overview](overview.md), [Package format](package-format.md), and [Production release verification](../operations/release-verification.md).
