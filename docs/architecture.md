# Architecture

Venom is organized into five implementation boundaries:

```text
compiler/   native C++ CLI, site discovery, route lowering, QuickJS compilation
package/    VBC container, section tables, compression, hashing, protection providers
vm/         logical Route VM instructions and per-build physical encoding
runtime/    generated browser runtime, C/WASM package support, host adapters
quickjs/    embedded QuickJS compiler/runtime integration and bytecode ABI
```

## Build pipeline

```text
supported website
  -> discover HTML routes, CSS, scripts, and local assets
  -> preserve script ordering and collect route metadata
  -> optionally vendor static remote HTTPS scripts
  -> lower HTML structure into logical Route VM operations
  -> generate a per-build physical Route VM encoding
  -> compile JavaScript with real QuickJS in compile-only mode
  -> serialize QuickJS executable objects with source/debug stripping
  -> build shared strings, routes, bytecode, assets, policy, and integrity sections
  -> write the protected VBC package
  -> emit index.html and grouped content-addressed assets
```

## Route VM model

The Route VM uses a narrow instruction set for deterministic page reconstruction. Logical operations include element creation, element-stack changes, attributes, text, append operations, and termination.

Protected builds can vary the physical representation without changing logical behavior:

```text
logical opcode -> remapped physical opcode
instruction fields -> reordered words
opcode/operands -> masked values
strings/routes -> reordered tables
package sections -> reordered layout
```

The production browser interpreter for this bytecode is part of the generated JavaScript runtime. The native C++ VM interpreter is development and test infrastructure, not the production browser executor.

## QuickJS model

Application JavaScript is compiled by embedded QuickJS using compile-only evaluation. Venom serializes native QuickJS object bytecode and strips source/debug records for protected output.

The browser runtime loads a real QuickJS engine compiled to WebAssembly. It deserializes compiler-produced objects, evaluates them, processes pending jobs, and reports exceptions and execution status through the Venom runtime boundary.

Production policy denies browser-host JavaScript execution and source-decode fallback when the QuickJS/WASM engine is unavailable or invalid.

## Browser runtime model

```text
index.html
  -> loader
  -> worker/runtime bootstrap
  -> fetch package and runtime assets
  -> validate required metadata and asset consistency
  -> select packaged route
  -> Route VM reconstructs deterministic page structure
  -> QuickJS/WASM executes route application bytecode
  -> host bridge performs supported browser operations
```

The loader, worker, generated runtime, Route VM interpreter, host bridge, and WebAssembly modules are all client-delivered runtime components. WebAssembly hosts QuickJS, but the overall system is not a secure enclave.

## Host bridge

The host bridge connects QuickJS semantics to selected browser services. It includes versioned operation metadata and implemented argument, size, lifecycle, resource, and policy checks.

Compatibility is limited by the bridge implementation. A valid QuickJS program may still depend on a browser API or exact DOM semantic that Venom does not currently implement.

## Deployment model

Venom emits static files suitable for an ordinary static server or CDN. Production correctness depends on deploying every generated file from the same build, preserving MIME types, avoiding partial cache mixing, and serving WebAssembly correctly.

For security properties and limitations, read [Security model](security-model.md) and [Threat model](threat-model.md). For compatibility, read [Compatibility](compatibility.md).
