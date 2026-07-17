# AST call-graph and class capture analysis

Venom's protection planner derives direct dependency calls and capture kinds from the Terser AST and lexical scope model.

- `const` function expressions and arrow functions are classified as helper-function dependencies.
- Direct calls to captured helper/import bindings are recorded in planner reasoning.
- Captured class declarations and constructors require manual review because prototype identity, `instanceof`, private fields, and receiver semantics cannot be preserved by simple runtime extraction.
- Mutable function bindings remain mutable captures and are not treated as stable helper dependencies.

This analysis is advisory unless strict planner mode is enabled.
