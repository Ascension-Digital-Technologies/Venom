# Annotation Reference

> **Applies to:** Venom 1.1.0

Venom protects JavaScript by default. Annotations select browser execution where compatibility requires it or explicitly document protected intent.

## Default protected file

A JavaScript file with no Venom annotation is planned for protected TurboJS/WASM execution unless compatibility analysis requires a supported adjustment.

## Browser file

```javascript
// @venom: browser
```

At file scope, the entire file remains browser-executed. Use this for framework bootstrap, DOM rendering, browser event wiring, and browser-native APIs.

## Browser function

```javascript
// @venom: browser
function renderChart(points) {
  chart.draw(points);
}
```

The annotated function remains browser-side while other eligible logic in the file may be protected.

## Explicit protected function

```javascript
// @venom: protected
function calculateRisk(order) {
  return order.quantity * order.price;
}
```

Use this to override a broader browser-file policy where supported or to document a high-value protected boundary.

## Protected module

```javascript
// @venom: protected module
```

The module is compiled as a protected module graph and exposes supported exports through `venom.exports`.

## Placement rules

Place file annotations before executable statements. Place function annotations immediately before the declaration they govern. Avoid ambiguous blocks of unrelated comments between the annotation and declaration.

See [Annotations guide](../guides/annotations.md) for examples and [Protected modules](../guides/protected-modules.md) for module behavior.
