# Release and development launchers

Venom exposes a deliberately small script surface. Windows launchers are under `scripts/windows/`; Linux and macOS launchers are under `scripts/linux/`.

| Purpose | Windows | Linux/macOS |
|---|---|---|
| Build Venom | `scripts\windows\build.bat` | `scripts/linux/build.sh` |
| Build the verified Emscripten runtime | `scripts\windows\build-emsdk.bat` | `scripts/linux/build-emsdk.sh` |
| Package a release | `scripts\windows\package-release.bat` | `scripts/linux/package-release.sh` |
| Verify a release | `scripts\windows\verify-release.bat` | `scripts/linux/verify-release.sh` |
| Build and launch the Venom website | `scripts\windows\build-and-launch-website.bat` | `scripts/linux/build-and-launch-website.sh` |
| Build and launch Protected Chess | `scripts\windows\build-and-launch-example1.bat` | `scripts/linux/build-and-launch-example1.sh` |
| Build and launch NOVA TRADE | `scripts\windows\build-and-launch-example2.bat` | `scripts/linux/build-and-launch-example2.sh` |
| Build and launch Venom Sentinel | `scripts\windows\build-and-launch-example3.bat` | `scripts/linux/build-and-launch-example3.sh` |

Implementation utilities live under `tools/` and are not part of the public launcher surface.
