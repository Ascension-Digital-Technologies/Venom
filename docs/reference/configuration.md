# Configuration Reference

> **Applies to:** Venom 1.1.0

The canonical project configuration file is `venom.toml`. Generate a starting file with `venom init` or copy `venom.toml.example`.

## Configuration categories

- project and source root;
- route and entry behavior;
- browser/protected execution policy;
- runtime resource limits;
- asset handling and remote vendoring;
- production hardening and verification;
- compatibility qualification;
- output location and hosting base path.

## Production behavior

Production configuration must preserve fail-closed runtime operation, verified TurboJS/WASM execution, locked hardener use, source-leakage scanning, and asset binding. Unsupported keys or invalid values should fail configuration rather than silently weakening the build.

## Path values

Paths are resolved relative to the project or site root documented by the relevant key. Prefer forward-compatible relative paths and do not depend on machine-specific drive letters or parent-directory traversal.

## Limits

Bridge payload, execution, memory, and queue limits should reflect measured application behavior. Resource limits are part of both availability and security policy.

## Capability policy

The `[capabilities]` table controls application-specialized browser host modules:

```toml
[capabilities]
fetch = "auto"
timers = "auto"
storage = "auto"
crypto = "auto"
```

Use `auto` for least-privilege specialization, `allow` to force inclusion, and `deny` to fail closed when the project requires a capability. `read-only` is reserved for restricted storage policy. See [Capability modules](../guides/capability-modules.md).

## Public JavaScript hardening

Browser-executed JavaScript that is not compiled to protected TurboJS bytecode is hardened by default across website, framework, and Chrome-extension targets. This includes files selected with `@venom: browser` and stable JavaScript adapters that must remain physical files.

```toml
[build]
harden_public_javascript = true
```

Disable it explicitly for debugging or compatibility work:

```powershell
venom build . --no-harden-public-js
```

The runtime, loader, worker, and TurboJS engine assets remain hardened regardless of this setting. Re-enable the policy with `--harden-public-js`.

## Validation

Run:

```powershell
venom compatibility check path\to\site
venom doctor --profile production
```

The exact accepted keys are also illustrated in `venom.toml.example`. See [Configuration guide](../guides/configuration.md) for deployment recommendations.
