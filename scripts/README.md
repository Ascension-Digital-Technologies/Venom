
## One-command examples

> Stable release entrypoints fail closed. Set `VENOM_RELEASE_PRIVATE_KEY_FILE` and `VENOM_RELEASE_PUBLIC_KEY_FILE` to Ed25519 PEM file paths before running `release.ps1` or `release.sh`. Unsigned stable archives are not supported.


Build a hardened production distribution and serve it immediately:

```powershell
.\scripts\protected-chess.bat
.\scripts\nova-trade.bat
.\scripts\bot-detection.bat
```

Optional development profile and custom port:

```powershell
.\scripts\nova-trade.bat -Profile dev -Port 8090
```

Unix equivalents are `scripts/protected-chess.sh`, `scripts/nova-trade.sh`, and `scripts/bot-detection.sh`.

Running `build-quickjs-wasm` without arguments now verifies and accepts the real runtime already embedded in a release. `-Artifact` is only needed when replacing the embedded runtime as a contributor.

# Scripts

The `scripts/` directory contains the supported command-line entry points for building, validating, serving, and releasing Venom. PowerShell is the primary Windows interface, `.bat` files are thin launchers, and `.sh` files provide Linux/macOS equivalents.

## Initial setup

### Windows

```powershell
.\scripts\setup-emscripten.ps1
.\scripts\build-quickjs-wasm.ps1
.\scripts\setup-js-hardener.ps1 -Force
.\scripts\build.ps1 -Config Release
```

### Linux or macOS

```bash
./scripts/setup-emscripten.sh
./scripts/build-quickjs-wasm.sh
./scripts/setup-js-hardener.sh
./scripts/build.sh Release
```

## Build profiles

Venom exposes only:

- `dev` — readable generated runtime and diagnostics, using real QuickJS/WASM protection;
- `prod` — hardened, obfuscated, hashed, stripped, fail-closed deployment output.

## Everyday workflow

```powershell
# Compile the Venom CLI
.\scripts\build.ps1 -Config Release

# Readable development distribution
.\scripts\build-site.ps1 `
  -Site examples\protected-chess `
  -Dist dist-dev `
  -Profile dev

# Hardened production distribution
.\scripts\build-site.ps1 `
  -Site examples\protected-chess `
  -Dist dist `
  -Profile prod

# Serve either distribution
.\scripts\serve-site.ps1 -Dist dist -Port 8080

# Run tests
.\scripts\test.ps1 -Config Release

# Remove generated output
.\scripts\clean.ps1
```

## JavaScript hardener repair

```powershell
.\scripts\setup-js-hardener.ps1 -Force
```

The Windows installer pauses automatically when installation fails so the npm or Node error remains visible. CI can disable the pause:

```powershell
.\scripts\setup-js-hardener.ps1 -Force -NoPause
```

Dependencies are installed locally under `tools/js-hardener`, followed by module-import, obfuscation, and generated-syntax self-tests.

## Windows build output discovery

Visual Studio writes multi-config output to paths such as:

```text
build/Release/venom.exe
```

Ninja and Make commonly write:

```text
build/venom
```

`resolve-venom.ps1` handles both layouts and is compatible with Windows PowerShell 5.1.

## Validation

```powershell
python .\scripts\check-production-leaks.py dist
.\scripts\analyze-dist.ps1 -Dist dist
.\scripts\readiness.ps1
```

## Full release qualification

```powershell
.\scripts\release.ps1 -Browser all -Seed 1350001
```

The release workflow builds the CLI and protected chess distribution, runs runtime and leak verification, checks tamper rejection and seeded reproducibility, performs browser qualification, and creates release archives.

Implementation logic belongs in `tools/`; script wrappers should remain small and stable.

## Live development

```powershell
.\scripts\dev.ps1 -Site examples\protected-chess -Port 8080 -Open
```

Or use the CLI directly:

```powershell
build\Release\venom.exe dev examples\protected-chess --open
```

The development server builds with the `dev` profile, watches source files, rebuilds changed content, serves `dist-dev`, and reloads connected pages. Use `--no-watch` for a single build-and-serve session.

## Environment diagnosis

```powershell
build\Release\venom.exe doctor
```

Use `--format json` for automation.

## Analyze a completed distribution

```powershell
build\Release\venom.exe analyze-dist dist
```

Use `--format json` for CI or tooling integration.

## Compatibility evidence

Run the canonical browser fixture suite:

```powershell
.\scripts\compatibility.ps1 -Browser all -Profile dev
```

The suite writes `compatibility-matrix.json` and `COMPATIBILITY.md` under `build/compatibility/`. Use `-SkipBuild` to re-aggregate already-built fixture distributions.


## Installable binary packages

```powershell
.\scripts\package-release.ps1 --archive zip --target-triplet windows-x64
```

Each archive contains `bin/venom.exe`, `CONTRACTS.json`, `install.ps1`, `uninstall.ps1`, documentation, licenses, and verification metadata.

## Complete release closure

Run the full clean release pipeline on Windows:

```powershell
.\scripts\release-closure.ps1
```

The command builds with warnings treated as errors, runs the complete CTest suite, validates both public examples in development and production profiles, scans production output, and creates a locally signed development release package. Results and individual command logs are written under `build/release-closure-output/`.

## Windows launcher behavior

Every `.bat` launcher uses `scripts/internal/invoke-powershell.bat`. When a launcher is double-clicked, the window remains open after both success and failure so the final status and diagnostic output can be read.

Automation can disable the pause with:

```bat
set VENOM_NO_PAUSE=1
```

CI environments are detected automatically and never pause. PowerShell and shell scripts do not pause by themselves, so they remain suitable for terminals and automation.

Console output uses consistent `STEP`, `INFO`, `SUCCESS`, `WARNING`, and `ERROR` labels.
