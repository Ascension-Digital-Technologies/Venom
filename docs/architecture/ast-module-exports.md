# AST module export closure

Venom's protection planner resolves ES module dependencies at symbol granularity.

Supported static forms include:

- `export function name() {}` and exported arrow/function bindings
- `export default function name() {}`
- `export { local as publicName }`
- `export { source as publicName } from "./module.js"`
- `export { default as publicName } from "./module.js"`
- `export * from "./module.js"`

Named and default imports inherit the runtime of the exact exported symbol. A barrel module may therefore expose both protected-compatible and browser-bound exports without forcing every named importer into the browser runtime.

Namespace exports/imports, anonymous default functions, default expressions, unresolved symbols, and conflicting export-star resolutions remain conservative because callable identity or property selection cannot be proven statically.

Explicit Venom annotations and planner overrides remain authoritative.
