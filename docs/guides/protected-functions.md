# Protected functions

Protected functions are best suited to deterministic, data-oriented logic.

## Public call

```javascript
await venom.ready();
const result = await venom.exports.calculateRisk(payload);
```

The bridge validates input shape, serializes JSON-safe data, enforces limits, maps the public export to an opaque runtime slot, and returns a Promise.

## Design recommendations

- Use plain objects, arrays, strings, booleans, numbers, and null.
- Keep browser state outside the protected function.
- Return structured errors rather than leaking implementation details.
- Batch work when bridge overhead would dominate tiny calls.
- Treat the protected function as an asynchronous service boundary.
