# Source-domain promotion review — 2026-07-18

> Superseded later on 2026-07-18 by `source-architecture-enforcement-2026-07-18.md`, which promoted/split the remaining `src/compiler/` internals and removed that umbrella entirely.


## Decision

The initial five-domain source tree was clean but too conservative. Several deeply nested modules had enough source volume, tests, dependency breadth, and lifecycle independence to justify direct ownership beneath `src/`.

## Promoted domains

- `src/compiler/commands/` and `src/compiler/main.cpp` became `src/cli/`.
- `src/compiler/pipeline/` became `src/pipeline/`.
- `src/compiler/frontends/` became `src/frontends/`.
- `src/engine/turbojs/` became `src/turbojs/`.
- `src/engine/vm/` became `src/vm/`.
- Native files from `src/runtime/native/` moved directly into `src/runtime/`.
- Compiler and runtime JavaScript inputs were consolidated into `src/templates/{browser,turbojs-engine,typescript}/`.

## Domains intentionally retained beneath compiler

`core`, `graph`, `planning`, and `services` remain under `src/compiler/`. They are tightly coupled compiler implementation details and do not yet need independent top-level APIs or build lifecycles.

## Result

The source tree now has ten clear domains: `cli`, `compiler`, `frontends`, `generated`, `package`, `pipeline`, `turbojs`, `runtime`, `templates`, and `vm`.

This layout improves discoverability while preserving ownership clarity. It also leaves room for future first-class domains such as `optimizer` or `aot` only when real implementations justify them, rather than reserving empty architecture.

## Validation

- CMake configuration completed successfully.
- The production `venom` executable and native runtime probes compiled successfully.
- The executable reports version 2.0.0.
- Source inventory, source layout, CMake completeness, repository consistency, launcher structure, template ownership, browser runtime bundling, TurboJS module bundling, and TypeScript frontend integration checks passed.
