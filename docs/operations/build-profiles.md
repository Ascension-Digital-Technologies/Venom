# Build profiles

Venom exposes exactly two profiles.

## `dev`

Use `dev` during implementation and integration.

- real QuickJS/WASM protected execution;
- readable generated JavaScript;
- stable asset names where practical;
- external asset manifest and build reports;
- unstripped diagnostic metadata;
- no production AST obfuscation;
- fail-closed host fallback policy remains active.

```powershell
.\scripts\build-site.ps1 -Site examples\protected-chess -Dist dist-dev -Profile dev
```

## `prod`

Use `prod` for deployment.

- protected bytecode and WASM-owned package decoding;
- aggressive generated-JavaScript hardening;
- compact WebAssembly ABI aliases;
- hashed assets and SRI;
- stripped reports and source metadata;
- per-build protocol, candidate, and package-layout diversification;
- production leak scanning;
- no browser qualification JSON or vendor notice text files in the site output;
- fail-closed runtime verification.

```powershell
.\scripts\build-site.ps1 -Site examples\protected-chess -Dist dist -Profile prod
```

The two profiles share the same public protected API and QuickJS/WASM execution model. Code that works in `dev` should behave identically in `prod`; only diagnostics, naming, diversification, and hardening differ.
