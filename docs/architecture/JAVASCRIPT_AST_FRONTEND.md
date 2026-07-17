# JavaScript AST frontend

Venom uses the embedded Terser 5.49 AST frontend inside native QuickJS for JavaScript parsing, module discovery, scope analysis, protected-unit lowering, and protection planning.

The protection planner no longer discovers functions with regular expressions or balances braces manually. It consumes top-level function records emitted by the parser and uses lexical-scope analysis for browser-global captures and unsupported semantics.

Recognized top-level callable forms include named function declarations, exported function declarations, function expressions assigned to a single binding, and arrow functions, including multiline and async forms.

TypeScript inputs are transpiled through Venom's TypeScript frontend before AST planning.
