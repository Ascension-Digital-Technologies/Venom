# Release certification

Venom 1.6.0 introduces a single fail-closed certification contract for platform, browser, runtime, security, framework, and example compatibility evidence.

## Contract

The authoritative policy is `contracts/release-certification.json`. It declares:

- required release platforms;
- required browser engines;
- static repository and contract gates;
- examples that must build successfully;
- production examples that must pass real TurboJS/WASM verification;
- production examples that must pass protected-source leak scanning;
- browser fixtures and public compatibility claims.

Changing a release claim requires changing this versioned contract and its certification tests.

## Platform certification

After building Venom, run:

```bash
python tools/certify_release.py \
  --venom build/release/venom \
  --output-dir build/release-certification
```

On Windows, pass the path to `venom.exe`.

The command emits:

```text
build/release-certification/certification.json
build/release-certification/certification.md
```

The JSON report includes command output, duration, environment identity, contract hash, example build status, runtime verification status, leak-scan status, and browser evidence hashes when supplied.

## Static qualification

Repository-only qualification is available without a native compiler:

```bash
python tools/certify_release.py --static-only
```

Static qualification is useful for fast pull-request feedback, but it is not a complete release certificate.

## Browser certification

The browser suite builds the JavaScript Playground and Aegis Operations and executes them in Chromium, Firefox, and WebKit:

```bash
python tests/browser/venom_examples_e2e.py \
  --venom build/release/venom \
  --browser all \
  --json-out build/browser-certification.json
```

It verifies real TurboJS/WASM execution, result transfer, context isolation, circular-value handling, console limits, interrupt recovery, protected application boot, route navigation, and browser error cleanliness.

## Aggregation

A public release is certified only when evidence exists for:

- Linux x64;
- Windows x64;
- macOS arm64;
- Chromium;
- Firefox;
- WebKit.

Aggregate evidence with:

```bash
python tools/aggregate_certification.py \
  --contract contracts/release-certification.json \
  --platform-report linux-certification.json \
  --platform-report windows-certification.json \
  --platform-report macos-certification.json \
  --browser-matrix browser-certification.json \
  --json-out final-certification.json \
  --markdown-out final-certification.md
```

Aggregation fails when evidence is missing, a platform gate failed, a browser failed, or an evidence report is malformed. Every input report is recorded by SHA-256.

## CI

`.github/workflows/certification.yml` runs platform qualification on all supported release hosts, provisions all three Playwright engines, and publishes an aggregated certification artifact. Pull requests run the component jobs; branch and scheduled runs also perform final aggregation.
