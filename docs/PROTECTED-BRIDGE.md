# Venom protected bridge

Venom exposes a deliberately small browser-to-protected API for explicit protected exports.

## Protected export

```javascript
// @venom: browser

// @venom: protected isolated
async function calculatePrice(input) {
  "use strict";
  /* @venom-isolated */
  return { total: input.quantity * input.unitPrice };
}
```

A successfully extracted protected function is registered as a browser-callable export. Internal helper functions remain private.

## Browser API

```javascript
await venom.ready();

const result = await venom.call("calculatePrice", {
  quantity: 4,
  unitPrice: 19.95
});

// Equivalent generated facade:
const sameResult = await venom.exports.calculatePrice({
  quantity: 4,
  unitPrice: 19.95
});
```

The supported public surface is intentionally limited to:

- `venom.ready()`
- `venom.call(name, input, options)`
- `venom.exports.<name>(input, options)`
- `venom.info()`

Raw workers, candidate identifiers, QuickJS handles, and package internals are not part of the public API.

## Values

Inputs and results must be JSON values: `null`, booleans, finite numbers, strings, arrays, and plain objects composed from those values.

Functions, symbols, `BigInt`, cycles, DOM objects, class instances, `Map`, `Set`, `Date`, `NaN`, and infinities are rejected or normalized by JSON encoding before transport. The default maximum encoded payload is 1 MiB.

## Timeout and cancellation

```javascript
const controller = new AbortController();

const work = venom.call("calculatePrice", payload, {
  timeout: 3000,
  signal: controller.signal
});

controller.abort();
```

Timeouts are clamped to 30 seconds. Cancellation rejects the browser promise and marks the request cancelled so late results are discarded. It is not a substitute for protected execution budgets; long-running protected code must still use QuickJS execution limits.

## Errors

Expected bridge failures use stable browser-facing errors:

- `VenomBridgeError`
- `VenomTimeoutError`
- `AbortError`

Unexpected protected exceptions are sanitized to `Protected operation failed` in release output. Protected stack traces and source positions are not exposed to browser code.

## Security model

Every protected export is callable by page code and therefore by an attacker controlling that page. Keep the export surface narrow, validate all inputs, bound work, and never embed permanent server secrets or rely on protected client code as the sole authorization layer.

## Production transport hardening

Hardened builds compile each protected export to a compact numeric slot. Browser-to-worker calls do not transmit internal candidate identifiers. Every worker boot also creates a fresh session token, and all call, cancel, result, and error envelopes carry a monotonic per-session counter. The worker rejects stale, duplicated, replayed, or cross-session envelopes. These controls reduce accidental cross-talk and generic protocol replay; they do not create a secret channel from an attacker who controls the client.

## File-level protected modules

```javascript
// @venom: protected module

function privateHelper(value) { return value * 2; }
export function calculate(input) { return { result: privateHelper(input.value) }; }
```

The compiler keeps private declarations inside QuickJS and emits an ES-module facade for browser entry exports. Static relative imports between protected modules are resolved at build time. Each module executes inside an independent lexical closure, so identical private helper and constant names cannot collide. Dependency-module exports are stored only in the internal protected module namespace and are not registered on `venom.exports`. Named imports and namespace imports are supported. Cycles, dynamic imports, default imports, side-effect-only imports, and non-function exports are rejected during compilation.
