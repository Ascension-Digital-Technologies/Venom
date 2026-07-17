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


## Typed bridge contracts

Protected functions may declare an object-shaped input and output contract immediately before the `@venom: protected` directive:

```javascript
// @venom: input value:number, bonus?:integer
// @venom: output result:number
// @venom: protected
export async function score(input) {
  return { result: input.value * 2 + (input.bonus || 0) };
}
```

Venom validates the input before protected execution and validates both synchronous and asynchronous results before they cross back into the browser runtime. A contract violation fails the call instead of allowing malformed values to reach protected logic.

Supported field types are:

- `string`, `number`, `integer`, and `boolean`;
- `object`, `array`, and `any`;
- `string[]`, `number[]`, `integer[]`, and `boolean[]`.

Append `?` to a field name to make it optional. Contract declarations are included in the private project IR and in generated developer artifacts:

```text
.venom/<dist-name>/api/protected-contracts.json
.venom/<dist-name>/api/venom-exports.d.ts
```

These files are build-time integration artifacts and are not copied into hardened production output.
