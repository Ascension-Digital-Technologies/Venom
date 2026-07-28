# Venom 3.0.0 Example Review

## Fixes

- Windows subprocess execution now resolves command shims such as `npm.cmd`, `npx.cmd`, `.bat`, and `.exe` without enabling a shell.
- Missing executables now return a controlled exit code 127 and a useful `command not found` diagnostic instead of leaking `[WinError 2]`.
- The canonical example certification workflow now installs and builds Vite dependencies before invoking Venom, and requires `dist/index.html`.

## Verification completed in this review

The native Release compiler built successfully.

The following examples passed production build, package integrity verification, real TurboJS runtime verification, and production leak scanning:

- protected-chess
- nova-trade
- bot-detection
- typescript-showcase
- tsx-showcase
- aegis-operations
- javascript-playground
- turbojs-benchmark

The shared launcher/runtime bootstrap smoke test passed for all 11 registered examples.

## Environment-limited checks

- `react-vite-showcase` and `vite-framework-showcase`: dependency installation did not complete against the available package registry in this Linux verification environment. Their Windows launcher failure is fixed by explicit `npm.cmd` resolution, and the certification workflow now performs the required frontend build.
- `chrome-extension`: the production build was started, but the complete extension verification chain exceeded the command execution window in this environment.

Run on Windows with Node.js/npm on PATH:

```bat
scripts\windows\build-and-launch-react-vite-showcase.bat
scripts\windows\build-and-launch-vite-framework-showcase.bat
scripts\windows\build-and-launch-chrome-extension.bat
```
