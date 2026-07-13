# Project configuration

Venom 1.1 introduces `venom.toml`, a source-controlled project policy file. The CLI searches from the site directory toward the filesystem root, or accepts an explicit path with `--config`.

```toml
[project]
name = "example-site"
entry = "site"
output = "dist"

[build]
vendor_cache = ".venom/vendor-cache"
vendor_lock = "site/venom.lock"
offline = false
refresh_vendors = false

[runtime]
engine = "quickjs-wasm"
fail_closed = true

[security]
deny_host_js_fallback = true
require_audited_crypto = false
```

Command-line values override configuration values:

```bash
venom build --config venom.toml --out dist-ci --format json
```

Production safety settings cannot be disabled through configuration. Venom rejects a configuration that requests a non-QuickJS/WASM engine, an open-failure policy, or host-JavaScript fallback.

The generated distribution includes `assets/app/build.json`, which records the product version, package and runtime ABI versions, source/config paths, selected backend, dependency lock digest, and package digest.
