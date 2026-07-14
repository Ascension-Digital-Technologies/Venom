# Protection planner

> **Applies to:** Venom 2.0.0-alpha.2

The Venom v2 planner is a local static-analysis tool that recommends whether JavaScript files and functions should execute in the protected QuickJS/WASM realm, remain browser-side, or receive manual review. It never contacts a server and never overrides an explicit developer decision.

## Run the planner

```powershell
venom plan path\to\site
venom plan path\to\site --format json
venom plan path\to\site --report build\protection-plan.json
```

The text report is intended for developers. JSON schema version 2 includes file and function recommendations, confidence values, purity scores, reason arrays, summary counts, precedence metadata, and the strict-mode result.

## Decision precedence

1. Function or file `@venom` annotations.
2. CLI `--protect` and `--browser` patterns.
3. `venom.toml` planner rules.
4. Automatic recommendations.
5. Protected-by-default fallback.

A lower-precedence source cannot replace a higher-precedence decision. Protected CLI rules are applied after browser CLI rules when the same path is intentionally supplied to both, so an explicit protection request wins that direct conflict.

## Manual customization

```powershell
venom plan . `
  --protect src\risk `
  --protect src\pricing `
  --browser src\ui
```

The same path rules are accepted by `venom build`. Existing annotations remain available and take precedence over automatic recommendations.

Configuration rules use arrays:

```toml
[planner]
mode = "recommend"
minimum_confidence = 80
protect = ["src/risk", "src/pricing"]
browser = ["src/ui", "src/rendering"]
```

## Function scoring

The planner scans named function declarations and arrow functions. It considers:

- direct DOM, window, navigator, storage, canvas, network, and event APIs;
- dynamic evaluation and runtime source loading;
- deterministic computation signals;
- observable side effects such as timers, logging, randomness, and message dispatch;
- explicit function annotations.

A purity score is a planning signal, not a security guarantee. Browser capability use generally produces a browser recommendation. Dynamic source generation produces manual review. Computation without browser-only dependencies generally produces a protected recommendation.

## Strict mode

```powershell
venom plan . --mode strict --min-confidence 85
venom build . --planner strict --planner-min-confidence 85
```

Strict mode fails when an unresolved manual-review item remains or an automatic recommendation falls below the configured confidence threshold. Resolve the finding with an annotation, CLI path rule, or `venom.toml` rule.

## Limitations

The alpha planner is conservative and lexical. It does not yet perform complete interprocedural data-flow analysis, alias resolution, or automatic source rewriting. Review recommendations before relying on them for production partitioning.
