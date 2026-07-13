# Build and release scripts

The scripts directory contains only supported entry points. PowerShell is the primary Windows interface; `.bat` files are thin launchers, and `.sh` files provide the equivalent Linux/macOS workflow.

## Everyday workflow

```powershell
# Configure and compile Venom
.\scripts\build.ps1 -Config Release

# Build the protected chess example
.\scripts\build-site.ps1 -Site examples\protected-chess -Dist dist

# Serve the generated distribution
.\scripts\serve-site.ps1 -Dist dist -Port 8080

# Run the test suite
.\scripts\test.ps1 -Config Release

# Remove generated files
.\scripts\clean.ps1
```

## Runtime prerequisites

```powershell
.\scripts\setup-emscripten.ps1
.\scripts\setup-js-hardener.ps1
.\scripts\build-quickjs-wasm.ps1 -VerifyOnly
```

Pass `-Artifact <path>` to `build-quickjs-wasm.ps1` when embedding a newly built, verified QuickJS WASM artifact.

## Validation and release

- `analyze-dist.*` inspects a generated distribution.
- `readiness.*` performs source/distribution readiness checks.
- `check-production-leaks.py` rejects protected metadata and decoder leaks.
- `package-release.*` creates a signed or unsigned publication archive.
- `verify-release.*` verifies a release archive and its manifest.

Implementation logic belongs in `tools/`; wrappers should remain small and stable.

## Full release qualification

```powershell
.\scripts\release.ps1 -Browser all -Seed 1350001
```

This is the authoritative release gate.

## Windows build output discovery

Visual Studio is a multi-config generator and writes the CLI to `build/<Config>/venom.exe` (for example, `build/Release/venom.exe`). Ninja and Make commonly write it directly under `build/`. The PowerShell workflows use `resolve-venom.ps1` to support both layouts, including Windows PowerShell 5.1 where `$IsWindows` is unavailable.

### Repair the JavaScript hardener

```powershell
.\scripts\setup-js-hardener.ps1 -Force

The Windows installer pauses automatically on failure so the npm/Node error remains visible. Use `-NoPause` for CI:

```powershell
.\scripts\setup-js-hardener.ps1 -Force -NoPause
```
```

The installer removes partial dependencies, installs the pinned packages locally under `tools/js-hardener`, and runs an import and output self-test.
