# @venom/venom

Drop-in Venom protection for existing projects.

```bash
npm install --save-dev @venom/venom
npx venom
```

No application rewrite, custom runtime bootstrap, or framework-specific API is required. The command detects static websites, Chrome extensions, Vite, React, Vue, and Svelte projects. Framework projects run their normal production build first; Venom protects the compiled output.

## Guided setup

```bash
npx venom init
npm run build:protected
```

`init` writes `venom.config.json` and adds a `build:protected` package script when a `package.json` is present.

## Diagnostics

```bash
npx venom doctor
npx venom detect
```

## Optional configuration

```json
{
  "$schema": "https://venom.dev/schemas/venom-config-v1.json",
  "input": "dist",
  "output": "dist-venom",
  "profile": "prod",
  "build": true,
  "buildScript": "build"
}
```

CLI options always override configuration:

```bash
npx venom --input build --out protected-dist --profile prod
```

## Compiler discovery

`@venom/venom` does not require a global Venom installation. It searches in this order:

1. `--venom <path>`
2. `venom` in `venom.config.json`
3. `VENOM_BIN`
4. Project-local `node_modules/.bin`, `bin`, and common `build` directories
5. System `PATH`

Inspect the selected compiler with:

```bash
npx venom bin
npx venom doctor
```

## Build a project-local compiler

When Venom is available as a source checkout, bootstrap a compiler and save its path automatically:

```bash
npx venom setup --source ../venom
```

This configures a Release build under the source checkout, builds the `venom` target, discovers the resulting executable, and writes the relative path to `venom.config.json`. A global Venom installation is not required.

Useful overrides:

```bash
npx venom setup --source ../venom --build-dir build/auto --config RelWithDebInfo
```


## Reproducible compiler pinning

Pin the exact compiler used by the project:

```bash
npx venom lock
```

This writes `.venom/toolchain.lock.json` with the compiler version and SHA-256 digest. `protect` and `doctor` reject a different compiler until the lock is intentionally refreshed. `venom setup` creates the lock automatically after building a project-local compiler.

## Continuous protected development

Run the normal framework build and Venom protection automatically whenever project files change:

```bash
npx venom dev
```

`dev` performs an initial protected build, watches the project with a portable polling loop, debounces editor save bursts, and ignores generated directories such as `node_modules`, `.venom`, `.venom-cache`, framework output, and `dist-venom`.

Useful controls:

```bash
npx venom dev --interval 500 --debounce 150
npx venom dev --once
```
