# Route hydration

Route hydration reconstructs compiled HTML and route state from a compact runtime program. It lets Venom avoid shipping the original HTML application graph as a directly readable source representation while retaining normal browser DOM ownership.

## Compile-time behavior

The compiler:

- parses and normalizes route HTML;
- applies supported browser-style implied closing rules;
- emits balanced element, text, attribute, and route operations;
- generates a build-specific physical opcode mapping;
- validates that the route program cannot pop below its synthetic root.

## Runtime behavior

The browser or WASM route executor resolves the physical opcode mapping, applies the route program, and reconstructs the expected DOM structure. Route state and asset resolution remain bound to the verified package and runtime metadata.

## Validation

Executable regressions cover malformed markup, omitted end tags, nested routes, multi-page output, asset-base resolution, and source-versus-protected browser equivalence. See [routing and hydration](../guides/routing.md) and [browser equivalence testing](../compatibility/browser-equivalence.md).
