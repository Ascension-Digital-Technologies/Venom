# Embedded TypeScript and TSX frontend

Venom uses the vendored TypeScript compiler API inside a dedicated native TurboJS runtime for `.ts`, `.tsx`, `.mts`, and `.cts` modules. The frontend removes type-only syntax, preserves ES modules, emits ES2022 JavaScript, and generates source maps without Node.js, package installation, subprocesses, temporary frontend files, or network access.

The TypeScript compiler is initialized once and reused for subsequent modules in the compiler process. Clean outputs participate in Venom's deterministic frontend cache. TypeScript diagnostics retain their original `TS####` code and source location and are surfaced as `VENOM-E2505`.

TSX is emitted with JSX preserved and then passed to Venom's deterministic classic JSX lowering stage. This keeps the local JSX runtime contract authoritative.

## Embedded compiler asset

The release binary contains a generated, chunked copy of the vendored TypeScript compiler source. At frontend initialization Venom verifies the embedded byte length, SHA-256 fingerprint, and expected compiler version before evaluating it in TurboJS. The small Venom adapter bridge is separately stored as pinned TurboJS bytecode with its own SHA-256, TurboJS version, and bytecode ABI metadata. The compiler therefore works outside the source tree and fails closed if generated asset metadata is stale or corrupted.

Regenerate the asset after updating the vendored compiler:

```bash
python tools/generators/typescript/embed_typescript_compiler.py \
  --input third_party/typescript/lib/typescript.js \
  --output-cpp src/generated/compiler/typescript_compiler_asset.cpp \
  --output-hpp src/generated/compiler/typescript_compiler_asset.hpp
```

The generated files are committed so normal release builds do not need Python merely to reconstruct the compiler payload.

Regenerate the pinned bridge bytecode after changing `src/templates/typescript/bridge.js` or updating TurboJS:

```bash
cmake -S . -B build-assets -DVENOM_BUILD_COMPILER_ASSET_TOOLS=ON
cmake --build build-assets --target venom_compile_typescript_bridge_bytecode
./build-assets/venom_compile_typescript_bridge_bytecode \
  src/templates/typescript/bridge.js \
  build-assets/typescript_bridge.qjbc
python tools/generators/typescript/embed_typescript_bridge_bytecode.py \
  --input build-assets/typescript_bridge.qjbc \
  --output-cpp src/generated/compiler/typescript_bridge_bytecode.cpp \
  --output-hpp src/generated/compiler/typescript_bridge_bytecode.hpp
```

`VENOM_TYPESCRIPT_BRIDGE=source` enables the verified source bridge for development and recovery testing. Release behavior defaults to the pinned bridge bytecode.

## Compatibility frontend

The lightweight native eraser remains available for diagnostics and constrained builds:

```bash
VENOM_TYPESCRIPT_FRONTEND=native venom build ...
```

The supported frontend selectors are `embedded`, `turbojs`, and `native`. The former `node` and `structural` selectors are intentionally rejected.

## Runtime requirements

The standard Venom compiler does not require Node.js. Optional Node ecosystem integrations such as Vite may still require Node because their host tools do.
