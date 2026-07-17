# Site Configuration

> **Applies to:** Venom 1.1.0

Venom uses `venom.toml` to describe the site root, build profile defaults, route behavior, runtime limits, asset handling, and production policy. Start from `venom.toml.example` or run `venom init` against an existing site.

## Recommended workflow

```powershell
venom init path\to\site
venom compatibility check path\to\site
venom dev path\to\site --open
venom build path\to\site --profile prod --out dist
```

## Configuration principles

- keep browser compatibility choices explicit;
- use production defaults unless a documented application requirement needs an override;
- avoid weakening fail-closed runtime checks;
- keep route and asset paths relative to the site root;
- qualify all browser APIs used by protected code;
- treat resource limits as security and availability controls.

## Profiles

Development output favors diagnostics and readable generated runtime assets. Production output requires the hardener, hashed assets, polymorphic package generation, runtime verification, and leakage checks. See [Build profiles](../operations/build-profiles.md).

## Runtime limits

Configure only limits that match the application's expected workload. Excessively low limits cause legitimate calls to fail; excessively high limits increase memory and denial-of-service exposure. Measure representative payload sizes and protected execution times before changing defaults.

## Complete key reference

The supported keys and accepted value forms are listed in [Configuration reference](../reference/configuration.md). Unknown or invalid production keys should be treated as configuration errors rather than silently ignored.
