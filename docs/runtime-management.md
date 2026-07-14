# Runtime Management

Venom ships the qualified QuickJS/WASM runtime inside the compiler and can materialize a verified, versioned runtime cache without requiring Emscripten.

```text
venom runtime install
venom runtime verify
venom runtime update
venom runtime path
```

Use `--dir <path>` to manage a custom runtime directory. Installation writes atomically and verifies the WebAssembly magic, exact embedded payload size, and version manifest before reporting success.

Emscripten is required only for contributors rebuilding the QuickJS/WASM runtime itself. Normal project creation, development, and production builds use the embedded qualified runtime.
