# Browser runtime compatibility harness — v0.88.0

v0.88.0 expands the browser-like Node execution harness from a basic DOM smoke into a broader host API bridge fixture suite. The harness still separates compatibility behavior from protected release truth, so debug fixtures can prove browser semantics while `browser-protect` continues to prove no host source-eval fallback.

The compatibility fixture now covers:

```txt
DOM writes
querySelector / querySelectorAll / matches / closest-style selector behavior
classList add/remove/toggle
style property and setProperty mutation
innerHTML parsing for simple fragments
form submit events and preventDefault
Promise microtasks and async error propagation
setTimeout timers
fetch bridge success
fetch/capability-denied failure capture
module dependency import/export cache behavior
multi-route click navigation
```

The protected fixture separately covers:

```txt
browser-protect release-check PASS
verify-runtime browser PASS
Function/eval blocked by the harness
protected boot through the QuickJS/WASM contract boundary
source-eval fallback not used in protected mode
```

This does **not** claim full upstream QuickJS-in-WASM parity. `verify-runtime --require-real-engine` must continue to fail until `build-quickjs-wasm` embeds a verified upstream QuickJS WASM artifact with all ABI12 exports.

## Test

```txt
venom_browser_runtime_compat_smoke
```

The test performs:

```txt
venom build examples/browser-compat-site --profile debug --hashed
node tests/runtime/browser-compat-harness.mjs <compat-dist> compat
node tests/runtime/browser-compat-harness.mjs <compat-dist> host-bridge

venom build examples/browser-compat-site --profile debug --hashed
node tests/runtime/browser-compat-harness.mjs <compat-denied-dist> capability-denied --deny-fetch

venom build examples/basic-site --profile browser-protect --hashed
venom release-check <strict-dist> --target browser
venom verify-runtime <strict-dist> --target browser
node tests/runtime/browser-compat-harness.mjs <strict-dist> strict-no-eval --strict-no-source-eval
```

## Current boundary

The debug compatibility path can still use the host JS compatibility fallback. The protected path must not. That split is intentional until the upstream QuickJS WASM artifact replaces the checked-in scaffold.
