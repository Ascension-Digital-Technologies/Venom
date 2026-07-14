# Browser equivalence testing

> **Audience:** integrators, release engineers, and compatibility contributors  
> **Applies to:** Venom 1.0.1
Venom browser equivalence testing executes the same compatibility scenario against an original website and its Venom production distribution in a real Playwright browser.

The gate checks three things for every declared observation:

1. The original website satisfies the expected behavior in `venom.browser.json`.
2. The Venom distribution satisfies the same expected behavior.
3. The observed source and distribution values are equivalent, unless the check explicitly uses threshold semantics or opts out of direct comparison.

It also records source-only and distribution-only console errors, page errors, scenario snapshots, tree hashes, manifest hashes, and browser identity.

## Run a comparison

```powershell
python tools/browser_equivalence.py `
  tests/fixtures/sites/browser-compat-site `
  build/compatibility/browser-api-and-modules `
  --manifest tests/fixtures/sites/browser-compat-site/venom.browser.json `
  --browser chromium `
  --json-out build/evidence/browser-api-and-modules.json
```

Use `--browser all` to run Chromium, Firefox, and WebKit.

## Scenario manifests

Equivalence uses the existing `venom.browser.json` format. Standard checks are compared exactly after whitespace normalization. Numeric minimum checks are considered equivalent when both source and protected builds satisfy the declared threshold.

A check may set:

```json
{
  "id": "generated-token",
  "type": "text",
  "selector": "#token",
  "equals": "ready",
  "equivalence": "ignore"
}
```

Use `equivalence: "ignore"` only when the value is intentionally nondeterministic and the expected-behavior assertion remains meaningful.

A scenario may set `compare_snapshot: true` to additionally compare normalized page title, path, body text, and element count.

## Release closure

Enable real-browser equivalence for the complete compatibility corpus with:

```powershell
.\scripts\release-closure.ps1 -BrowserRuntimeTests
```

This builds every fixture in `tests/compatibility-suite.json` with the production profile and writes evidence beneath:

```text
build/release-closure-output/compatibility/
├── browser-api-and-modules.equivalence.json
├── static-multi-route.equivalence.json
└── ...
```

The release closure fails when either side violates the manifest, when observable values diverge, or when either page emits a console or page error.
