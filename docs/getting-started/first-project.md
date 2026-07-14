# Your first protected project

```powershell
venom new my-site
cd my-site
venom dev . --open
```

Venom protects JavaScript by default. Keep DOM-heavy code in the browser with `// @venom: browser`, and expose data-oriented protected functions through the asynchronous public API.

```javascript
// @venom: protected isolated
function priceQuote(input) {
  return { total: input.quantity * input.price };
}
```

```javascript
await venom.ready();
const quote = await venom.exports.priceQuote({ quantity: 4, price: 12.5 });
```

Build production output:

```powershell
venom build . --profile prod --out dist
venom analyze-dist dist
venom release-check dist
```
