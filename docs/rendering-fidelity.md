# HTML/CSS rendering fidelity — v1.0.1

The protected document compiler must preserve browser-visible source semantics before JavaScript runs. This pass corrects four production regressions found by comparing the original Intoli page with the protected distribution.

## Corrected behavior

- Inline `<style>` blocks are extracted and emitted into `assets/style/s.<hash>.css`.
- Local `<link rel="stylesheet">` files are emitted in their original document order.
- CSS `url(...)` values continue through the compiled asset mapping.
- Venom no longer injects `system-ui`, `2rem` margins, or other visual defaults.
- Text around inline elements keeps a single meaningful boundary space.
- Formatting-only indentation between block tags is discarded.
- `<pre>`, `<textarea>`, `<listing>`, and `<xmp>` text is preserved exactly.
- The source `<title>` and safe attributes from `<html>` and `<body>` are retained.

The production smoke gate asserts the exact visible strings `User Agent (Old)` and `Fingerprint Scanner tests`, preserves a multi-line JSON block, and verifies that source CSS—not Venom defaults—controls the result.

## Runtime boundary

Static fidelity and dynamic JavaScript fidelity are separate release requirements. The current QuickJS/WASM path executes protected code inside the isolated engine, but its browser-side effect replay is not yet a complete generic DOM host-call implementation. A result such as `navigator.webdriver` can therefore execute in the isolated context without updating the corresponding real DOM node.

Production completion requires a typed host-call ABI for browser property reads, DOM lookup and mutation, events, timers, promises, network responses, and persistent object handles. Compatibility-specific source-pattern replay is not sufficient and must not be used as the final correctness boundary.
