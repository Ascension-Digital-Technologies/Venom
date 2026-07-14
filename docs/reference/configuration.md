# Configuration reference

Use `venom.toml` for project configuration and `venom.lock` for resolved runtime/remote inputs.

```powershell
venom config validate [path]
venom config print [path] --format json
```

The supported build profiles are `dev` and `prod`. Production always selects external packaging, the real WASM QuickJS backend, runtime-owned cryptographic handling, strict release policy, and denied host fallback. See `venom.toml.example` for the current schema.
