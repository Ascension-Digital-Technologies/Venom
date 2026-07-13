# Real-browser validation

Venom separates static compatibility analysis from behavioral browser evidence.
A site can compile successfully and still depend on browser semantics that the host bridge does not reproduce correctly. The real-browser validator loads a generated distribution through Chromium, Firefox, or WebKit and verifies observable DOM, event, asynchronous, fetch, route, and error behavior.

## Requirements

- Node.js 20 or newer
- the Playwright package
- installed Playwright browser binaries
- a generated Venom distribution built with the real QuickJS/WASM runtime

```bash
npm install --no-save playwright@1.52.0
npx playwright install chromium firefox webkit
```

## Run

```bash
./scripts/browser-validate.sh dist --browser chromium --manifest examples/browser-compat-site/venom.browser.json
./scripts/browser-validate.sh dist --browser all --manifest examples/browser-compat-site/venom.browser.json --json-out browser-validation.json
```

PowerShell:

```powershell
.\scripts\browser-validate.ps1 -Dist .\dist -Browser all -JsonOut .\browser-validation.json
```

The validator starts a loopback HTTP server, disables service workers for the test context, loads the production distribution, waits for runtime completion, exercises form and click events, confirms fetch and promise completion, checks route history behavior, and fails on browser page or console errors.

## Report binding

The JSON report includes a deterministic digest of every file in the tested distribution. The production-readiness tool rejects a browser report when its digest does not match the supplied distribution.

```bash
python tools/production_readiness.py examples/browser-compat-site \
  --dist dist \
  --venom build/release/venom \
  --require-real-engine \
  --browser-report browser-validation.json
```

## Interpretation

A passing report proves only the checked fixture behavior in the named browser engines. It does not prove full Web Platform compatibility, production server configuration, device-specific behavior, accessibility, performance, or every application user journey. Production applications should add project-specific browser scenarios rather than relying solely on the repository fixture.
