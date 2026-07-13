# Production readiness reporting

`tools/production_readiness.py` provides one conservative release-candidate report without changing Venom's compiler, package, Route VM, QuickJS/WASM, or host-bridge acceptance rules.

## Checks

When a source site is supplied, the report checks:

- source directory and root entry presence;
- availability and version of the selected Venom executable;
- Venom compatibility preflight results.

When `--dist` is supplied, it additionally checks:

- `index.html` and grouped assets;
- parseable `assets/app/build.json` provenance;
- protected `.vbc` package count;
- WebAssembly runtime presence;
- `venom verify-runtime`, optionally with `--require-real-engine`;
- deployment requirements that require operator confirmation.

## Interpretation

A normal report is ready when there are no failed checks. Warnings remain visible because MIME types, HTTPS, CSP, cache policy, atomic deployment, and complete browser behavior cannot be proven from a directory of static files.

`--strict` treats those warnings as release blockers. This is useful for CI only when the missing deployment evidence is supplied or deliberately waived by a higher-level release process.

A ready report does not claim:

- universal website or browser compatibility;
- cryptographic secrecy from the browser operator;
- publisher authenticity without trusted external signatures;
- correct production server MIME, caching, TLS, or security-header configuration;
- successful end-to-end application journeys.

## Examples

```bash
python tools/production_readiness.py examples/basic-site \
  --dist dist \
  --venom build/release/venom \
  --require-real-engine
```

```bash
python tools/production_readiness.py examples/basic-site \
  --dist dist \
  --venom build/release/venom \
  --format json
```

Exit code `0` means the selected policy passed. Exit code `60` means the release-readiness policy did not pass.

## Real-browser evidence

Use `--browser-report` to bind a Playwright validation report to the exact distribution being assessed. The readiness tool recomputes the distribution digest and fails if the report was generated for different files. Without a report, distribution assessments include a browser-validation warning.

```bash
python tools/production_readiness.py site --dist dist --venom venom \
  --require-real-engine --browser-report browser-validation.json
```
