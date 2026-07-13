# Venom Protected Chess Lab

The Protected Chess Lab is Venom's flagship example. It demonstrates a practical hybrid application in which the browser retains the parts that benefit from native browser compatibility while proprietary computation runs inside the protected QuickJS/WASM boundary.

## What the example proves

The browser is responsible for:

- chessboard rendering and animation;
- DOM events and controls;
- move history, charts, and status presentation;
- human-versus-engine and engine-versus-engine orchestration.

Venom protects:

- board evaluation and piece-square tables;
- minimax and alpha-beta search;
- legal move selection;
- search metrics and engine telemetry.

The engine is extracted from browser JavaScript and compiled into protected QuickJS bytecode. Browser code communicates with it through Venom's bounded request/response bridge.

## Source layout

```text
protected-chess/
├── index.html
├── css/
│   └── style.css
├── img/
│   └── chesspieces/
├── js/
│   ├── ai-engine.js   protected engine unit
│   ├── chess.js       browser chess rules/state
│   └── main.js        browser UI and game controller
└── vendor/            local browser dependencies
```

The protected engine is declared with Venom's isolated marker:

```javascript
// @venom: protected isolated
function runChessEngine(request) {
  // Evaluation and search logic.
}
```

Browser code calls it through the public bridge:

```javascript
await venom.ready();

const result = await venom.exports.runChessEngine({
  action: "search",
  fen: game.fen(),
  depth: 4,
  color: "w"
});
```

Only JSON values cross the boundary. DOM nodes, functions, class instances, cyclic values, and browser objects are intentionally rejected.

## Build

From the repository root:

```powershell
.\scripts\build.ps1 -Config Release
.\scripts\build-site.ps1 -Site examples\protected-chess -Dist dist
.\scripts\serve-site.ps1 -Dist dist -Port 8080
```

Open `http://127.0.0.1:8080`.

Linux/macOS:

```bash
./scripts/build.sh Release
./scripts/build-site.sh examples/protected-chess dist
./scripts/serve-site.sh 8080 dist
```

Do not open `dist/index.html` directly with a `file://` URL. Workers, WASM, module loading, and integrity checks require an HTTP origin.

## Expected protected distribution

```text
dist/
├── index.html
└── assets/
    ├── app/
    │   └── app.<hash>.vbc
    ├── images/
    ├── loader/
    │   └── loader.<hash>.js
    ├── runtime/
    │   ├── engine.<hash>.js
    │   ├── r.<hash>.js
    │   ├── runtime.<hash>.wasm
    │   └── rw.<hash>.wasm
    ├── style/
    │   └── s.<hash>.css
    └── workers/
        └── worker.<hash>.js
```

Protected releases use hashed assets, fail-closed QuickJS/WASM execution, an opaque session-bound bridge, WASM-owned package decoding, per-build section layout diversification, and aggressive generated-JavaScript hardening.

## Validation

Run the focused and full regression suites:

```powershell
.\scripts\test.ps1 -Config Release
python scripts\check-production-leaks.py dist
```

A successful protected build should verify that:

- `runChessEngine` is extracted;
- minimax and evaluation tables are absent from shipped browser JavaScript;
- no readable package decoder is present in JavaScript;
- no source paths or descriptive QuickJS ABI names leak;
- host-JavaScript fallback is denied;
- the real QuickJS/WASM backend is present.

## Performance expectations

Performance depends on browser, processor, depth, position, and power state. The example is designed to demonstrate that protected computation can remain practical; it is not intended as a competitive chess engine benchmark.

## Security model

Venom raises the cost of recovering and modifying browser-delivered logic. It does not make client-side software unrecoverable. A determined operator controls their browser, can observe bridge inputs and outputs, and can instrument WASM memory. Never embed permanent server secrets or rely on client-side protection as the sole authorization layer.

See the repository's [security model](../../docs/security-model.md), [threat model](../../docs/threat-model.md), and [protected bridge documentation](../../docs/PROTECTED-BRIDGE.md).

## Troubleshooting

### The page loads but the engine does not move

Check the browser console and verify that:

```javascript
await venom.ready();
Object.keys(venom.exports);
```

includes `runChessEngine`. Rebuild the distribution after changing protected code; do not mix assets from different builds.

### Integrity or binding mismatch

Delete the complete `dist/` directory and rebuild it. Hashed assets, package bindings, loader SRI, and runtime artifacts must come from the same build.

### QuickJS/WASM unavailable

Verify the embedded runtime:

```powershell
.\scripts\build-quickjs-wasm.ps1 -VerifyOnly
```

Protected builds intentionally fail instead of falling back to readable browser execution.

## Browser qualification manifest

`venom.browser.json` is the release E2E contract. It starts AI-vs-AI, requires at least four legal moves and nonzero throughput, then verifies Stop & Reset cancels the loop cleanly. The release workflow runs it in Chromium, Firefox, and WebKit.
