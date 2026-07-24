# Compiler Pipeline

> **Applies to:** Venom 1.1.0

Venom transforms a normal web project into a static production distribution while separating browser-native responsibilities from protected TurboJS/WASM execution.

## 1. Site discovery

The compiler reads HTML, CSS, JavaScript, modules, routes, and static assets under the configured site root. It resolves local module references, route entry points, stylesheets, images, fonts, and browser-side dependencies.

## 2. Compatibility analysis

Venom identifies code that must remain in the browser because it depends on the DOM, framework lifecycle, browser objects, event instances, or APIs not available in the protected runtime. Explicit annotations override or confirm the planned runtime where supported.

## 3. Protected-runtime planning

Unannotated JavaScript is protected by default. Function and module plans determine which declarations are extracted, which dependencies must move with them, and which exports become available through the public asynchronous API.

## 4. JavaScript rewriting

Browser chunks are rewritten to call protected exports. Protected modules are transformed into TurboJS-compatible records. Generated bridge registries use opaque build-specific capability identities instead of source-level function names on the transport.

## 5. Bytecode compilation

Protected scripts and modules are compiled with the pinned TurboJS engine. Canonical records are validated against the expected TurboJS ABI, wrapped in build-specific `VTJSE006` envelopes, and bound to route, source, and execution order.

## 6. Package construction

The compiler writes the VBC package with polymorphic section ordering, padding, identifiers, route mappings, metadata, and integrity records. Production builds can vary physical layout while preserving application behavior.

## 7. Runtime generation

Venom emits the loader, worker, runtime JavaScript, TurboJS engine bridge, package/route WASM assets, stylesheets, and browser chunks. Production JavaScript is minified, mangled, role-specifically obfuscated, and stripped of source-only diagnostics.

## 8. Asset binding

Generated assets are content addressed and hash-bound. The loader, package, workers, runtime modules, stylesheets, and WebAssembly binaries must agree on the exact build identity.

## 9. Release verification

Production qualification checks hardener output, runtime provenance, source leakage, package contracts, compatibility scenarios, browser equivalence, and signed release metadata.

See [Architecture overview](overview.md), [Package format](package-format.md), and [Production release verification](../operations/release-verification.md).

## JavaScript AST frontend

Venom parses every discovered JavaScript source with the pinned Terser 5.49.0 parser embedded in the native compiler. The frontend validates syntax and derives static imports, re-exports, dynamic literal imports, declarations, functions, and source locations from parsed nodes instead of scanning raw text. Comments and string literals therefore cannot create false module-graph edges. Parse failures use the stable `VENOM-E2101` diagnostic and identify the source file.

The frontend also performs lexical-scope analysis for protected function extraction. Dependency lifting now uses parser-owned symbol definitions rather than identifier text scanning, so parameters, destructuring, nested functions, local bindings, imports, mutable captures, and browser globals are classified according to actual scope semantics.

Protected-function lowering is AST-owned as well. Named declarations, async functions, variable-bound arrows, and function expressions are normalized into protected units with parser-derived source ranges, parameters, callable expressions, export state, and source locations. Browser stubs and TurboJS registry entries are generated from those units rather than regex or balanced-brace source slicing. Generator functions fail closed with `VENOM-E2303`.

## Versioned project IR

TypeScript sources are first erased in memory with line-preserving transformations and then enter the same AST pipeline as JavaScript. After site discovery and JavaScript planning, Venom constructs private `venom-project-ir` metadata. IR version 9 records source fingerprints, TypeScript-erasure metrics, routes, assets, protected modules, parser-derived metrics including top-level declaration counts, module edges, protected exports, typed bridge contracts, and deterministic project/plan fingerprints. Protected-module imports and named function exports are lowered from AST-derived statement boundaries rather than a separate source-text lexer. It is written only to the compiler cache and is never copied into production output.

## Content-addressed incremental cache

Generated runtime assets are cached only after their complete hardener identity is known. The key includes the embedded Terser and javascript-obfuscator versions, asset role, deterministic seed, and full input bytes. Native TurboJS bytecode records are cached separately using the TurboJS bytecode ABI, source identity, module mode, metadata policy, complete source bytes, and the complete module-source graph. Cache hits reuse byte-identical compiler outputs; project discovery, protection planning, package diversification, integrity binding, encryption, and final verification still run on every build. `--no-cache` disables reuse and `--cache-dir` selects an isolated cache root.

### Source-aware diagnostics

Frontend, scope-analysis, and lowering failures use a shared diagnostic renderer. Diagnostics include stable error codes, source locations, a source excerpt with a caret, technical detail, an actionable fix, and a documentation reference. See [Compiler diagnostics](../reference/diagnostics.md).
