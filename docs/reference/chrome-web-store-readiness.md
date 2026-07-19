# Chrome Web Store readiness

Venom verifies hardened Manifest V3 distributions before packaging them for publication.

## Verify an unpacked distribution

```bash
python tools/verify_chrome_extension_store.py dist-chrome-extension-prod
```

The gate checks the Manifest V3 schema, root-plus-assets layout, referenced resources, content security policy, inline and remotely loaded executable code, development-file leakage, web-accessible resources, permission breadth, and evidence that extension JavaScript passed through the hardener.

High-impact permissions and broad host permissions are warnings by default because some extensions legitimately need them. Pass `--strict-permissions` to treat them as release errors.

## Create a publication ZIP

Windows:

```bat
scripts\windows\package-chrome-extension.bat
```

Portable command:

```bash
python tools/package_chrome_extension.py dist-chrome-extension-prod --out-dir release/chrome-extension
```

The output contains an unpacked copy, a Chrome Web Store ZIP, `SHA256SUMS.txt`, a store-readiness report, and a build report. `manifest.json` remains the only root file in the extension package; all runtime resources remain under `assets/`.
