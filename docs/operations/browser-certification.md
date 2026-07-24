# Browser certification

Venom browser certification is governed by `contracts/browser-certification.json`. It builds every included example through the production pipeline, verifies the real TurboJS/WASM runtime, serves each distribution on loopback, and opens it in Chromium, Firefox, and WebKit.

Certification waits for `globalThis.__venomBootStatus.state` to become `ready` or `error`. The ready state is authoritative only after package verification, browser module linking, route execution, and startup scripts complete. Failures include structured phase, message, code, stack, browser console errors, page exceptions, and HTTP failures.

```bash
python tests/browser/venom_examples_e2e.py \
  --venom build/venom \
  --browser all \
  --json-out build/browser-certification.json
```
