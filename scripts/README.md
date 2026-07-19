# Public build and example launchers

The `scripts/` directory is the small, user-facing command surface. Implementation logic belongs in `tools/`; these files only resolve the repository root, invoke the canonical Python controller, propagate its exit code, and apply one shared completion/pause policy for double-clicked Windows consoles.

Windows launchers live in `scripts/windows/`. Linux and macOS launchers live in `scripts/linux/`. Example launchers use semantic names rather than unstable sequence numbers.

## Build and release

| Purpose | Windows | Linux/macOS |
|---|---|---|
| Build Venom | `scripts\windows\build.bat` | `scripts/linux/build.sh` |
| Build the verified Emscripten runtime | `scripts\windows\build-emsdk.bat` | `scripts/linux/build-emsdk.sh` |
| Package a release | `scripts\windows\package-release.bat` | `scripts/linux/package-release.sh` |
| Verify a release | `scripts\windows\verify-release.bat` | `scripts/linux/verify-release.sh` |
| Run the complete release closure | `scripts\windows\release-closure.bat` | `scripts/linux/release-closure.sh` |
| Build and launch the Venom website | `scripts\windows\build-and-launch-website.bat` | `scripts/linux/build-and-launch-website.sh` |

## Examples

| Example | Windows | Linux/macOS |
|---|---|---|
| Protected Chess | `scripts\windows\build-and-launch-chess.bat` | `scripts/linux/build-and-launch-chess.sh` |
| NOVA TRADE | `scripts\windows\build-and-launch-nova-trade.bat` | `scripts/linux/build-and-launch-nova-trade.sh` |
| Bot Detection | `scripts\windows\build-and-launch-bot-detection.bat` | `scripts/linux/build-and-launch-bot-detection.sh` |
| TypeScript Showcase | `scripts\windows\build-and-launch-typescript-showcase.bat` | `scripts/linux/build-and-launch-typescript-showcase.sh` |
| TSX Showcase | `scripts\windows\build-and-launch-tsx-showcase.bat` | `scripts/linux/build-and-launch-tsx-showcase.sh` |
| Aegis Operations | `scripts\windows\build-and-launch-aegis-operations.bat` | `scripts/linux/build-and-launch-aegis-operations.sh` |
| JavaScript Playground | `scripts\windows\build-and-launch-javascript-playground.bat` | `scripts/linux/build-and-launch-javascript-playground.sh` |
| QuickJS Benchmark | `scripts\windows\build-and-launch-quickjs-benchmark.bat` | `scripts/linux/build-and-launch-quickjs-benchmark.sh` |
| Vite Framework Showcase | `scripts\windows\build-and-launch-vite-framework-showcase.bat` | `scripts/linux/build-and-launch-vite-framework-showcase.sh` |
| Chrome Extension | `scripts\windows\build-and-launch-chrome-extension.bat` | `scripts/linux/build-and-launch-chrome-extension.sh` |

All named wrappers are generated from `contracts/examples.json` by `tools/generate_example_launchers.py` and delegate to `tools/launch_example.py`. Run the generator with `--check` to detect launcher drift without rewriting files. Numbered example identifiers are intentionally not part of the public or internal launcher interface.
