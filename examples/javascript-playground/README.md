# JavaScript Playground

A browser-based JavaScript compiler and execution playground powered directly by Venom's embedded QuickJS/WASM source-execution ABI.

This is intentionally a **development-mode example**. Venom production releases are bytecode-only and reject runtime source compilation by design. The example preserves that security boundary rather than weakening production verification.

The browser submits source to `__venomRuntime.executeQuickJsChunk`; QuickJS parses and executes it in WASM and returns engine records, console capture, evaluation data, timing, or an exception. Browser `eval` and `Function` are never used.

## Run

PowerShell from the repository root:

```powershell
.\\scripts\\windows\\build-and-launch-javascript-playground.bat
```

Linux/macOS:

```bash
./scripts/linux/build-and-launch-javascript-playground.sh
```

Open `http://127.0.0.1:8086/`.

## Security boundary

- QuickJS/WASM performs source parsing and execution.
- Browser source evaluation is not used.
- Host-JavaScript fallback remains denied.
- DOM, storage, and network objects are not injected into user source.
- Production builds remain bytecode-only and fail closed.

## Development security profile

This example uses the `dev` profile because it intentionally accepts source text for QuickJS/WASM compilation. The package is still integrity-checked, runtime-hardened, fail-closed, and denies host-JavaScript fallback. Release-only diversification and ABI fingerprint contracts are reserved for packages carrying the release profile.

## Compilation architecture

The browser does not use `eval`, `Function`, or host JavaScript source execution. The local JavaScript Playground server exposes a loopback-only development endpoint that compiles the submitted source with Venom's native QuickJS compiler into an authenticated `VQJSBC03` record. The browser then executes that bytecode through the embedded QuickJS/WASM runtime.

This endpoint is enabled only by the JavaScript Playground launcher and is intended for local development demonstrations. It is not emitted into production distributions.

## Isolation and quotas

Every submitted script runs in a disposable QuickJS/WASM context. The default local-development policy limits source to 256 KiB, JSON input to 64 KiB, heap to 8 MiB, stack to 256 KiB, pending jobs to 128, console capture to 128 events / 128 KiB, and returned data to 256 KiB. An interrupt budget terminates runaway execution, and the context is destroyed after every run.

The loopback compiler endpoint uses a random per-launch capability token, same-origin Host/Origin validation, JSON-only requests, a two-process concurrency ceiling, a reduced subprocess environment, and a ten-second compilation timeout. It is a local development service and is not included in production distributions.
