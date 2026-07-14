# Installation

> **Audience:** users building or running Venom  
> **Applies to:** Venom 1.0.1
## Windows requirements

- Windows 10 or newer
- Visual Studio with **Desktop development with C++**
- CMake 3.20 or newer
- Python 3.10 or newer

```powershell
git clone https://github.com/Ascension-Digital-Technologies/Venom.git
cd Venom
.\scripts\build.ps1 -Config Release
.\build\Release\venom.exe doctor --profile production
```

The compiler is written to `build\Release\venom.exe` for Visual Studio multi-configuration generators.

## Linux/macOS requirements

Install a C++17 compiler, CMake, and Python. Then run:

```bash
./scripts/build.sh --config Release
./build/venom doctor --profile production
```

## Runtime contributors

Ordinary users do not need to rebuild QuickJS/WASM. Contributors changing the embedded runtime must install the pinned Emscripten version and use the canonical controller documented in [QuickJS/WASM development](build-from-source.md).

## Verify the installation

```powershell
venom --version
venom doctor --profile development
venom doctor --profile production
```
