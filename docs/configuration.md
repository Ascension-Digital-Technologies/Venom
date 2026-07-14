# Configuration

Venom uses `venom.toml` at the project root. CLI arguments override project configuration.

```toml
[project]
entry = "."
output = "dist"

[build]
profile = "dev"

[runtime]
engine = "quickjs-wasm"
fail_closed = true

[security]
deny_host_js_fallback = true
```

Only `dev` and `prod` are valid profiles. Validate and inspect the resolved configuration with:

```powershell
venom config validate
venom config print
venom config print --format json
```

The runtime and fail-closed security values are mandatory. Unknown fields inside recognized sections are reserved for forward-compatible versions; unknown sections fail validation.
