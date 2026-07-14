# Protected API contracts

Venom 1.42 adds optional contracts for browser-callable exports in protected modules.
Contracts use a compact field-list syntax immediately before an exported function.

```javascript
// @venom: protected module

// @venom: input query:string, limit?:integer
// @venom: output items:array, total:integer
export function search(input) {
  return { items: [], total: 0 };
}
```

Supported field types are `string`, `number`, `integer`, `boolean`, `object`, `array`, `any`, and arrays of primitive types such as `string[]` and `number[]`. Add `?` to a field name to make it optional.

Contracts are enforced inside QuickJS before the protected implementation runs and again before a result crosses back to the browser. Development facades perform the same checks early for clearer diagnostics. Production keeps the authoritative protected-side validation but does not ship readable contract declarations in browser JavaScript.

A development build generates:

```text
assets/app/venom-exports.d.ts
```

This declaration file can be copied into an application TypeScript project or consumed by editor tooling.

Contract errors fail closed with `VENOM-E2301` through `VENOM-E2303` during compilation, or a precise `TypeError` during a protected call.
