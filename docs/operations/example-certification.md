# Example certification

Venom 2.0.0 uses one production-only certification pipeline for every certification-eligible example. The authoritative inventory is `contracts/examples.json`.

```powershell
python tools\certify_examples.py --venom build\venom.exe
```

For every certification-eligible example the command performs a deterministic production build, package-integrity verification, real TurboJS/WASM verification, and production leak scanning. It emits per-example JSON plus consolidated JSON and Markdown reports.

The checked-in certification summary is available under `docs/audit/example-certification/`. Browser interaction remains a separate CI matrix because it requires Chromium, Firefox, and WebKit execution environments.

Launch-only examples such as the standalone TurboJS benchmark and the Chrome extension remain in the same registry but are excluded from browser certification through an explicit `certify: false` flag.
