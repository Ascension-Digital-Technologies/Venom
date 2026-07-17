# Protected Modules

> **Applies to:** Venom 1.1.0

Protected modules package related JavaScript logic into the QuickJS/WASM execution runtime while exposing only declared asynchronous exports to browser code.

## Declaring a protected module

```javascript
// @venom: protected module

export function calculateRisk(order) {
  return order.quantity * order.price;
}
```

The module implementation is compiled to QuickJS bytecode and stored in the protected package. Browser code does not import the source module directly; it calls the generated Venom export surface.

## Browser invocation

```javascript
await venom.ready();
const risk = await venom.exports.calculateRisk({ quantity: 5, price: 120 });
```

## Suitable module content

Protected modules work best for deterministic business logic, calculations, scoring, validation, parsing, transformations, search, and algorithms that communicate using JSON-safe values.

Keep DOM access, framework lifecycle state, event objects, browser handles, and compatibility-sensitive APIs in browser modules. Pass only the minimal plain-data input required by the protected operation.

## Module graphs

Venom resolves supported local protected-module dependencies, binds the module graph to the package metadata, and validates the QuickJS bytecode ABI at runtime. Dynamic module behavior should be qualified with the compatibility suite before production use.

## Errors

Protected errors are sanitized before they cross the bridge. Applications should handle stable error categories rather than depending on internal QuickJS stack details or source filenames.

See [Protected functions](protected-functions.md), [Browser bridge](browser-bridge.md), and [JavaScript API](../reference/javascript-api.md).
