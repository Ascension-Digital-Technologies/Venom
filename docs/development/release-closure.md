# Release closure

```powershell
.\scripts\release-closure.ps1
```

The closure validates repository state, signed release policy, strict embedded runtime provenance, JavaScript hardener functionality, clean Release build, complete CTest execution, example compatibility, dev/prod example builds, distribution analysis, leakage scans, and local signed packaging. Logs and a JSON summary are written beneath `build/release-closure-output/`.


## Real-browser equivalence

```powershell
.\scripts\release-closure.ps1 -BrowserRuntimeTests
```

This optional strict tier builds every fixture in `tests/compatibility-suite.json` and compares the original website with the Venom production distribution in Chromium. Each report is hash-bound to the source tree, generated distribution, and scenario manifest. See [browser equivalence testing](../compatibility/browser-equivalence.md).
