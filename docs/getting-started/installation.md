# Installation

> **Audience:** users building or running Venom  
> **Applies to:** Venom 1.65.3
## Windows requirements

- Windows 10 or newer
- Visual Studio with **Desktop development with C++**
- CMake 3.20 or newer
- Python 3.10 or newer
- Node.js 20 or newer and npm

```powershell
git clone <repository-url>
cd venom-secure-web-runtime
.\scripts\setup-js-hardener.ps1
.\scripts\build.ps1 -Config Release
.\build\Release\venom.exe doctor --profile production
```

The compiler is written to `build\Release\venom.exe` for Visual Studio multi-configuration generators.

## Linux/macOS requirements

Install a C++17 compiler, CMake, Python, Node.js, and npm. Then run:

```bash
./scripts/setup-js-hardener.sh
./scripts/build.sh --config Release
./build/venom doctor --profile production
```

## Runtime contributors

Ordinary users do not need to rebuild QuickJS/WASM. Contributors changing the embedded runtime must install the pinned Emscripten version and use the canonical controller documented in [QuickJS/WASM development](../development/quickjs-wasm.md).

## Verify the installation

```powershell
venom --version
venom doctor --profile development
venom doctor --profile production
```
