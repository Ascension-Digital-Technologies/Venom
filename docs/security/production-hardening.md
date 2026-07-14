# Production hardening

A production build requires the real verified QuickJS/WASM runtime, denies host fallback, hardens generated JavaScript, compacts WASM exports, diversifies package structure, binds assets, removes development artifacts, and runs release leakage checks.

```powershell
venom build . --profile prod --out dist
venom analyze-dist dist
venom release-check dist
```

For repository releases, run the complete release closure script.
