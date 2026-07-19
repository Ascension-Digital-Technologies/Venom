# Chrome Extension Example

Velocity Chess v0.4.5 demonstrates a Manifest V3 extension whose chess engine runs inside Venom's protected QuickJS/WASM runtime.

## Build on Windows

```bat
scripts\windows\build-and-launch-chrome-extension.bat
```

The production distribution is written to `dist-chrome-extension-prod`. Load that directory through `chrome://extensions` using **Load unpacked**.

The output keeps only `manifest.json` at the root. All HTML, JavaScript, CSS, data, protected package, and WASM resources are emitted under `assets/`. Every emitted extension JavaScript file passes through Venom's built-in hardener.

## Store readiness

```bat
scripts\windows\package-chrome-extension.bat
```

This verifies Manifest V3 policy, CSP, referenced resources, JavaScript hardening, protected-source leakage, permissions, and standard layout before creating a publication ZIP and SHA-256 checksum.

The visible extension files are Chrome-facing adapters. Search, evaluation, move generation, transposition tables, and related Velocity engine internals are compiled into the sealed Venom package.
