# Optional ecosystem packages

This directory contains independently consumable packages that integrate with the Venom compiler or runtime.

- `runtime/` provides the JavaScript runtime API package.
- `vite/` provides the `@venom/vite` integration.

Core native compiler builds do not require Node.js or these packages.

## `@venom/react`

Typed React hooks for runtime readiness, protected calls, preloading, cancellation, and Strict Mode-safe component integration.

## Zero-config integration

Use `@venom/venom` when a project should adopt Venom without framework-specific wiring:

```bash
npm install --save-dev @venom/venom
npx venom
```

It detects the project, runs the existing production build when needed, and protects the compiled output.

## Packages

- `auto/` — zero-config project detection, local compiler discovery, framework build orchestration, and protection.
- `runtime/` — protected runtime client.
- `vite/` — direct Vite lifecycle integration.
- `react/` — typed React hooks for protected calls.
