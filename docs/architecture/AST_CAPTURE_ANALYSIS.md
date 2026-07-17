# AST capture analysis

Venom classifies each free identifier captured by a candidate protected function according to its top-level declaration.

Safe dependency candidates include top-level helper functions, imports, and primitive `const` values. The dependency lowering pipeline can reproduce these bindings in the protected runtime.

Mutable `let` and `var` bindings require manual review even when a function only reads them. Moving the function into another runtime can otherwise split the state lifetime or produce stale values.

Non-primitive `const` bindings also require review because object identity, prototypes, accessors, and mutation history may not survive serialization or dependency lowering unchanged.

The effect pass detects direct writes and common indirect mutations on captured objects, including property assignments, deletion, increments, and mutating methods such as `push`, `splice`, `set`, `add`, `delete`, and `clear`.
