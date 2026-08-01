# Installation

> **Audience:** users installing or building Venom  
> **Applies to:** Venom 3.5.0

## Binary release layout

Every Venom 3.5.0 binary release contains two required executables:

```text
bin/
├── venom[.exe]
└── venom_hardener_worker[.exe]
```

The `venom` executable is the public CLI. `venom_hardener_worker` is an internal companion process used for process-isolated JavaScript hardening. Keep both files in the same directory. Copying only the main executable is not a complete installation.

## Windows x86-64

1. Extract `venom-3.5.0-windows-x86_64.zip`.
2. Keep `venom.exe` and `venom_hardener_worker.exe` together in the extracted `bin` directory.
3. Add that `bin` directory to `PATH`, or invoke `venom.exe` by its full path.

Verify:

```powershell
venom --version
venom doctor --profile production
```

## Linux x86-64

1. Extract `venom-3.5.0-linux-x86_64.tar.gz`.
2. Keep `venom` and `venom_hardener_worker` together in `bin/`.
3. Add that directory to `PATH`, or invoke the binary directly.

```bash
tar -xzf venom-3.5.0-linux-x86_64.tar.gz
cd venom-3.5.0-linux-x86_64
chmod +x bin/venom bin/venom_hardener_worker
./bin/venom --version
./bin/venom doctor --profile production
```

## Build from source

### Windows requirements

- Windows 10 or newer
- Visual Studio with **Desktop development with C++**
- CMake 3.20 or newer
- Python 3.10 or newer

```powershell
git clone https://github.com/Ascension-Digital-Technologies/Venom.git
cd Venom
.\scripts\windows\build.ps1 -Config Release
.\build\Release\venom.exe doctor --profile production
Get-Item .\build\Release\venom_hardener_worker.exe
```

### Linux requirements

Install a C++17 compiler, CMake, Ninja or Make, and Python 3.10 or newer. Then run:

```bash
git clone https://github.com/Ascension-Digital-Technologies/Venom.git
cd Venom
bash scripts/linux/build.sh --config Release
./build/venom doctor --profile production
test -x ./build/venom_hardener_worker
```

Depending on the generator, binaries may be placed in a configuration-specific directory. The build script reports the resolved paths.

## Runtime contributors

Ordinary users do not need to rebuild TurboJS/WASM. Contributors changing the embedded runtime must install the pinned Emscripten version and use the canonical controller documented in [Build from source](build-from-source.md).

## Troubleshooting

When a protected build reports that the hardener worker is missing, verify that the companion executable remains beside the main compiler and has execute permission. Do not work around the error by disabling hardening.
