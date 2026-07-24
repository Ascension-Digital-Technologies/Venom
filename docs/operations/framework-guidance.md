# Framework integration guidance

Venom supports framework applications through its **hybrid execution model**. Rendering, reconciliation, DOM ownership, router bootstrap, and framework internals normally stay browser-side. Application logic that accepts and returns JSON-safe values can be moved into protected TurboJS/WASM functions or modules.

## Recommended boundary

| Responsibility | Recommended runtime |
|---|---|
| Framework runtime and renderer | Browser |
| Component lifecycle and DOM refs | Browser |
| Client router integration | Browser |
| Charting and canvas libraries | Browser |
| Validation, scoring, rules, algorithms | Protected |
| Proprietary calculations | Protected |
| Data normalization without DOM objects | Protected |

## React and Vite

Keep the React entry module and component tree browser-side. Extract pure calculations, reducers containing proprietary rules, validation engines, and scoring code into protected functions.

```javascript
// @venom: browser
import { createRoot } from "react-dom/client";

async function submitOrder(order) {
  const decision = await venom.exports.assessOrder(order);
  // Update React state with the JSON-safe result.
}
```

## Vue and Vite

Keep Vue's application bootstrap, templates, directives, reactive proxies, and component lifecycle browser-side. Protected APIs should receive plain snapshots rather than Vue proxy objects.

```javascript
const result = await venom.exports.calculateExposure({
  positions: positions.value.map(({ symbol, quantity, price }) => ({ symbol, quantity, price }))
});
```

## Svelte and Vite

Svelte's compiled DOM update code remains browser-side. Move pure business logic behind protected calls, then assign the returned JSON-safe result to component state.

## Qualification meaning

The checked-in framework fixtures validate representative **production output patterns** in source-versus-Venom browser equivalence runs. They do not claim that every framework version, plugin, SSR mode, hydration strategy, or build-tool option is automatically supported. Pin the real application toolchain and add its generated output to the compatibility corpus before making a production support claim.
