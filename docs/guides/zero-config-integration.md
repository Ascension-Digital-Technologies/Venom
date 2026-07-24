# Zero-config integration

Venom can protect an existing project without requiring the application to adopt a Venom-specific architecture.

## One-command setup

```bash
npm install --save-dev @venom/venom
npx venom
```

`@venom/venom` detects the project type, runs the existing production build when a framework compiler is present, and protects the browser-ready output.

Supported automatic paths:

| Project | Automatic input |
|---|---|
| Static website | Project directory |
| Chrome extension | Project directory |
| Vite | Compiled `dist` directory |
| React + Vite | Compiled `dist` directory |
| Vue + Vite | Compiled `dist` directory |
| Svelte + Vite | Compiled `dist` directory |

The original application continues using its normal framework, router, component model, and deployment conventions. Venom is applied after framework compilation.

## Package script

```json
{
  "scripts": {
    "build": "vite build",
    "build:protected": "venom"
  }
}
```

## Nonstandard build output

```bash
npx venom --input build --out protected-dist
```

## Optional deeper integration

Projects that need live build status or direct protected calls may additionally use:

- `@venom/vite`
- `@venom/runtime`
- `@venom/react`

These packages are optional. A site does not need to import the runtime SDK or restructure its components merely to receive normal whole-site protection.

## No global compiler requirement

The integration resolves Venom project-first. A repository can keep its compiler in `node_modules/.bin`, `bin`, `build`, `build/Release`, `build/full-release`, or `build/release`. Explicit CLI/config paths and `VENOM_BIN` are also supported.

```bash
npx venom bin
npx venom doctor
```

`doctor` reports the selected compiler and whether it came from the project, configuration, environment, or system PATH. Missing compilers fail before the framework build begins.
