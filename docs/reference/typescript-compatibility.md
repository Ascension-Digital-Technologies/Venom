# TypeScript compatibility

Venom 1.2 uses a native type-erasure frontend before JavaScript AST planning and TurboJS compilation.

## Supported in the native frontend

- Interfaces and type aliases
- Type-only imports and exports
- Parameter, return, variable, and class-field annotations
- Generic function and class declarations
- Generic call expressions
- Type assertions, `satisfies`, `as const`, and non-null assertions
- Access modifiers and `readonly`
- Abstract classes and declaration-only abstract methods
- Constructor parameter properties
- Optional and definite-assignment class fields
- `implements` clauses and generic `extends` clauses
- Function overload signatures
- Ambient `declare` statements and blocks

## Explicitly unsupported

- Runtime enums
- Runtime namespaces

Unsupported runtime constructs fail with a source-located diagnostic instead of being emitted as broken JavaScript.


## JSX and TSX

`.tsx` files are lowered with Venom's classic JSX frontend. The emitted JavaScript calls `React.createElement`, so projects may use React or any compatible local runtime. Supported syntax includes intrinsic elements, component tags, fragments, nested elements, expression children, string/boolean/expression attributes, and spread attributes.

Generic TSX arrow functions remain distinct from JSX when written in the standard unambiguous form, such as `<T,>(value: T) => value`.
