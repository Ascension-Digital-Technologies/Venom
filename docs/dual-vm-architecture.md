# Dual-engine architecture

Venom separates packaged route/DOM execution from general-purpose JavaScript execution. The two execution layers cooperate, but they are implemented differently and should not be described as equivalent isolation boundaries.

## Route VM

The Route VM is a narrow custom bytecode interpreter implemented by the generated browser runtime. It handles package-specific operations such as:

- route selection and fallback resolution;
- packaged page reconstruction;
- deterministic DOM construction records;
- shared string and asset references;
- route-related metadata and selected event bindings.

Protected builds can diversify the physical encoding through opcode remapping, opcode and operand masks, instruction-word reordering, string reordering, route reordering, and package section reordering.

The Route VM does not implement JavaScript language semantics. In protected output, its package parser, polymorphic instruction decoder, and route interpreter run inside WebAssembly. JavaScript remains the browser mutation boundary and applies only the validated binary DOM-command stream emitted by the Route VM.

## QuickJS in WebAssembly

Application JavaScript is compiled by real QuickJS in compile-only mode. Venom serializes QuickJS executable objects and strips source and debug records from protected output.

The browser runtime uses a real QuickJS engine compiled to WebAssembly. It reads the serialized object bytecode, evaluates it, processes pending jobs, and reports runtime failures through the Venom execution boundary.

QuickJS owns JavaScript language semantics, including functions, closures, objects, exceptions, promises, modules, and garbage collection.

## Host bridge

QuickJS does not directly receive the browser global environment. Supported browser effects pass through Venom's host bridge.

The bridge contains versioned operation metadata, argument handling, resource limits, policy checks, and denial behavior. It is not a complete browser implementation, and “capability-gated” should not be interpreted as proof of a formally verified object-capability security model.

## Data flow

```text
Source website
      |
      v
Venom compiler
  - route and HTML analysis
  - Route VM bytecode generation
  - QuickJS native bytecode compilation
      |
      v
Protected .vbc package and hashed assets
      |
      v
Generated browser runtime
      |
      +-------------------------------+
      |                               |
      v                               v
Route VM interpreter             QuickJS/WASM runtime
implemented in generated JS      real QuickJS engine
route and DOM bytecode           native object bytecode
      |                               |
      +---------------+---------------+
                      |
                      v
              Venom host bridge
                      |
                      v
             Supported browser APIs
```

## Security properties

The architecture raises static-analysis and modification cost through bytecode compilation, source stripping, per-build diversification, package checks, and fail-closed engine policy.

It does not make browser-delivered code or data secret. A determined client can instrument the loader, worker, generated runtime, WebAssembly memory, host calls, and materialized package data.

This document describes the implementation model. It does not establish a separately supported public Route VM ABI.
