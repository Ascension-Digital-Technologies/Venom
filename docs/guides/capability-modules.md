# Capability Modules

> **Applies to:** Venom 1.1.0

Venom specializes the browser host surface around the capabilities a project actually uses. Capability policy is declared in `venom.toml` and enforced during the build before a distribution is emitted.

## Configuration

```toml
[capabilities]
fetch = "auto"
timers = "auto"
storage = "auto"
crypto = "auto"
```

Supported policy values:

| Value | Behavior |
|---|---|
| `auto` | Enable the module only when source analysis finds that the project requires it. |
| `allow` | Include the module even when current source analysis does not require it. |
| `deny` | Fail the build when the project requires the capability. |
| `read-only` | Reserve a restricted read-only policy. It is currently meaningful for storage policy metadata; writes remain subject to runtime enforcement. |

## Initial modules

### Fetch

Covers browser network requests discovered through `fetch` or compatible request APIs. Denying this module causes a fail-closed build error when network access is detected.

### Timers

Covers timeout, interval, and animation-frame scheduling required by the generated runtime or application.

### Storage

Covers `localStorage`, `sessionStorage`, and IndexedDB discovery. Storage remains a browser-owned resource and should not contain credentials, private keys, or server-authoritative state.

### Crypto

Covers Web Crypto usage such as `crypto.getRandomValues` and `crypto.subtle`. This capability does not replace Venom's audited package-crypto boundary.

## Verification

Inspect resolved configuration with:

```bash
venom config print venom.toml
```

A denied capability produces a non-zero build result before output generation:

```text
venom: capability 'fetch' is required by the project but denied in venom.toml
```

Capability decisions are also represented in runtime-module planning and the private host-capability metadata embedded in the protected package.
