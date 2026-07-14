# Compatibility evidence

Venom validates browser and framework-output compatibility through executable fixtures rather than broad marketing claims.

## Canonical suite

The suite is declared in `tests/compatibility-suite.json`. Each entry identifies a source fixture, its build output, and a browser contract stored in `venom.browser.json`.

Run the complete suite on Windows:

```powershell
.\scripts\compatibility.ps1 -Browser all -Profile dev
```

Linux or macOS:

```bash
BROWSER=all PROFILE=dev ./scripts/compatibility.sh
```

The command:

1. Builds each fixture with Venom.
2. Serves and validates it with Playwright.
3. Aggregates Chromium, Firefox, and WebKit results.
4. Enforces support-tier promotion rules.
5. Writes machine-readable and human-readable evidence.

Generated files:

```text
build/compatibility/
├── <fixture>.browser.json
├── compatibility-matrix.json
└── COMPATIBILITY.md
```

## Evidence levels

- `probe` means a narrow capability has been observed.
- `candidate` means behavioral validation has passed in at least one required browser.
- `supported` requires every required browser, the minimum check count, and behavioral or conformance evidence.

A framework fixture proves only the tested framework version, delivery form, and behaviors. It does not imply universal compatibility with every application using that framework.

## Release policy

Compatibility claims must be backed by archived browser reports and a matrix generated from the exact compiler/runtime revision being released. Missing browsers, insufficient checks, or an unsupported promotion cause the suite to fail.
