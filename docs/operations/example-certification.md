# Example certification

Venom 2.0.0 uses one production-only certification pipeline for every included example. The authoritative inventory is `contracts/examples.json`.

```powershell
python tools\certify_examples.py --venom build\venom.exe
```

For every example the command performs a deterministic production build, package-integrity verification, real QuickJS/WASM verification, and production leak scanning. It emits per-example JSON plus consolidated JSON and Markdown reports.

The checked-in certification summary is available under `docs/audit/example-certification/`. Browser interaction remains a separate CI matrix because it requires Chromium, Firefox, and WebKit execution environments.
