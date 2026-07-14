# Building from source

## Standard contributor build

```powershell
.\scripts\setup-js-hardener.ps1
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Use `venom doctor --profile development` for compiler work and `--profile production` before creating hardened sites. Keep generated build directories outside release archives.
