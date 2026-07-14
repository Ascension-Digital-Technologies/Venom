# Configuration guide

Project configuration lives in `venom.toml`; resolved dependency and remote-source state belongs in `venom.lock`.

Validate configuration before building:

```powershell
venom config validate
venom config print --format json
```

Use only documented keys. Production policy is intentionally fail-closed: the real QuickJS/WASM runtime is required and host-JavaScript fallback is denied. See the [configuration reference](../reference/configuration.md).
