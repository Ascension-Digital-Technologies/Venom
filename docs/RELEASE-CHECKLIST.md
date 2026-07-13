# Public release checklist

A Venom release is publishable only when every required gate passes.

## Repository

- Version matches across CMake, generated headers, README, changelog, and package metadata.
- The public example set contains only `examples/protected-chess`.
- No build directories, distributions, caches, secrets, signing keys, or crash artifacts are tracked.
- Documentation links resolve.
- `LICENSE`, `NOTICE.md`, `SECURITY.md`, `SUPPORT.md`, and `CODE_OF_CONDUCT.md` are present.

## Build and runtime

- Clean native configure and Release build pass.
- Real QuickJS/WASM runtime is embedded and verified.
- Protected Chess builds with a fixed seed.
- Chromium, Firefox, and WebKit qualification passes where supported.
- Protected engine starts, completes multiple moves, and reset/cancellation works.
- No host-JavaScript fallback is available in the release profile.

## Security and integrity

- Production leak scanner passes.
- Package, loader, and runtime-WASM tampering fail closed.
- Missing runtime assets fail closed.
- Protected source names, source paths, reports, decoder markers, and descriptive production ABI names are absent.
- Same-seed builds are byte-identical.
- Different-seed builds are structurally different.

## Supply chain

- Release manifest is generated and verified.
- SBOM and provenance metadata are included for stable releases.
- Third-party licenses and `NOTICE.md` are included.
- Archive checksum is generated.
- Signing is enabled for official stable artifacts.

## Commands

```powershell
python .\tools\public_release_gate.py .
.\scripts\release.ps1 -Browser all -Seed 1350001
```

Never bypass a failed gate for an official release. Fix the cause or document a
clearly scoped pre-release exception.
