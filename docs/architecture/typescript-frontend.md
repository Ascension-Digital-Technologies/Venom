# Structural TypeScript and TSX frontend

Venom 1.3.0 uses the vendored TypeScript compiler API as its default frontend for `.ts`, `.tsx`, `.mts`, and `.cts` modules. The frontend parses TypeScript structurally, removes type-only syntax, preserves ES modules, emits ES2022 JavaScript, and generates source maps without package installation or network access.

TSX is emitted with JSX preserved and then passed to Venom's deterministic classic JSX lowering stage. This keeps `React.createElement` behavior and the local JSX runtime contract stable.

## Compatibility frontend

The former native eraser remains available temporarily:

```bash
VENOM_TYPESCRIPT_FRONTEND=native venom build ...
```

The compatibility frontend is intended for migration diagnostics only. New projects should use the structural default.

## Runtime requirement

The structural driver requires Node.js 18 or newer. Set `VENOM_NODE` to an explicit Node executable when it is not discoverable through `PATH`.
