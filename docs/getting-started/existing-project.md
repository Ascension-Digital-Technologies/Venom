# Protect an existing website

## 1. Initialize

```powershell
venom init path\to\site
```

This creates project configuration without replacing application source. Commit the generated configuration and lock data.

## 2. Qualify compatibility

```powershell
venom compatibility check path\to\site --format text
```

Review browser-only APIs, dynamic code construction, workers, remote assets, module graphs, and runtime capabilities before deciding protection boundaries.

## 3. Mark browser-sensitive code

```javascript
// @venom: browser
function installDomHandlers() {
  document.querySelector("#buy").addEventListener("click", submitOrder);
}
```

Keep valuable data-oriented code protected:

```javascript
// @venom: protected isolated
function scoreOrder(order) {
  return order.quantity * order.limitPrice;
}
```

## 4. Develop and inspect

```powershell
venom dev path\to\site --open
venom analyze path\to\site
```

## 5. Build and verify

```powershell
venom build path\to\site --profile prod --out dist
venom analyze-dist dist
venom release-check dist
```

Integrate incrementally. Begin with algorithms and pure functions, then expand the protected boundary after browser-equivalence testing.
