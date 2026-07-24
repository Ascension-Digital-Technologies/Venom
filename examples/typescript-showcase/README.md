# TypeScript AST Showcase

A complete Venom example written in TypeScript. It demonstrates the native TypeScript erasure frontend, AST-backed runtime planning, symbol-level dependency closure, default imports, barrel re-exports, planner cycle fixtures, browser/protected separation, generated protected API declarations, and correct handling of non-JavaScript `<script>` data blocks.

## Preview

![Venom TypeScript AST showcase](screenshot.png)

The screenshot above shows the completed example running with browser-rendered TypeScript UI code and the quote calculation executing through the protected TurboJS/WASM runtime.

## Demonstrated compiler features

| Feature | Location |
|---|---|
| Type-only imports and interfaces | `protected/types.ts` |
| Typed constants and arrow helpers | `protected/constants.ts`, `protected/math.ts` |
| Default TypeScript export/import | `browser/format.ts`, `browser/app.ts` |
| Re-export planning fixture | `protected/index.ts` |
| Planner cycle and dynamic-boundary fixtures | `planner-fixtures/` |
| Browser-only DOM code | `browser/app.ts` |
| JSON and JSON-LD script data | `index.html` |
| Generated protected `.d.ts` API | production build output |

## Build

```powershell
venom build examples\typescript-showcase --profile prod --out examples\typescript-showcase\dist-venom
venom verify examples\typescript-showcase\dist-venom
```

Development server:

```powershell
venom dev examples\typescript-showcase --open
```

The protected quote engine calculates pricing, discounts, tax, a risk score, and a parity result. The UI and DOM operations remain in the browser runtime while the calculation graph executes through TurboJS/WASM.
