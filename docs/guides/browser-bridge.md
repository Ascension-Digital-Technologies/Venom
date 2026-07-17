# Browser bridge

The browser bridge is intentionally narrow. The public application sees asynchronous protected exports; it does not receive the protected registry, bytecode, decoder state, or QuickJS context.

```javascript
await venom.ready();
const value = await venom.call("calculateRisk", input);
// or
const value2 = await venom.exports.calculateRisk(input);
```

## Boundary properties

- JSON-safe values only
- argument and payload limits
- session-bound messages
- replay counters
- opaque export slots/opcodes in production
- sanitized errors
- worker-owned QuickJS context

Design protected APIs as coarse, meaningful operations rather than exposing every internal helper.
