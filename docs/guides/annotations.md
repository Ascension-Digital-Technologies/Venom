# Annotation guide

Venom protects JavaScript by default. Annotations define exceptions and explicit boundaries.

## Default protected execution

A file without annotations is planned for protected TurboJS/WASM execution where compatible.

## Entire browser file

```javascript
// @venom: browser
```

Use for framework bootstrap code, direct DOM manipulation, graphics, browser-only APIs, and compatibility-sensitive integration code.

## Browser function

```javascript
// @venom: browser
function renderChart(data) { /* DOM/chart library */ }
```

## Explicit protected function

```javascript
// @venom: protected isolated
function calculateRisk(order) { /* proprietary logic */ }
```

Prefer JSON-safe inputs and outputs. Avoid passing DOM nodes, functions, cyclic objects, class instances, or browser handles across the worker boundary.
