# Release qualification

Venom's release gate validates the real `examples/protected-chess` application rather than only compiler metadata.

## One-command workflow

Windows:

```powershell
.\scripts\release.ps1 -Browser all -Seed 1350001
```

Linux/macOS:

```bash
BROWSER=all SEED=1350001 ./scripts/release.sh
```

## Required evidence

The qualification tool:

1. Builds the protected chess distribution with a deterministic diversification seed.
2. Verifies package, runtime, binding, and real QuickJS/WASM requirements.
3. Runs the production metadata/source leak scanner.
4. Mutates the package, loader, and runtime WASM and requires verification to fail.
5. Removes the runtime WASM and requires verification to fail.
6. Launches the requested Playwright browser matrix.
7. Starts AI-vs-AI and waits for at least four legal moves.
8. Requires nonzero engine positions and at least 1,000 reported positions/second.
9. Resets the game and verifies autoplay cancellation and idle state.
10. Builds twice with the same seed and requires byte-identical trees.
11. Builds with another seed and requires a different tree.

## Seeded builds

```powershell
venom build examples\protected-chess --out dist --seed 1350001
```

The same source, toolchain, `SOURCE_DATE_EPOCH`, options, and seed must produce the same distribution. Changing the seed changes diversification while preserving behavior.

## CI

The `release-qualification.yml` workflow runs the flagship qualification on Linux and Windows. The complete three-browser Playwright matrix runs on Linux; Windows validates Chromium and the native toolchain path.
