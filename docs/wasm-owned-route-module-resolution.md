# v0.88.0 WASM-owned route and module boundary

v0.88.0 moves authoritative route normalization and selection into the WASM runtime. The loader copies the requested path into bounded WASM memory and calls `venom_wasm_resolve_route`; WASM decodes and validates the string and route tables, normalizes common HTML/index path forms, applies deterministic root fallback, validates string references, and returns a compact `VROUTE01` record.

Protected boot and history navigation consume this bounded record rather than performing route-table search in readable JavaScript. The record contains only the selected route, source path, route index, bytecode range, instruction count, and flags.

The boundary is fail-closed: malformed tables, invalid string IDs, oversized paths, missing routes, and invalid record ranges stop boot. JavaScript retains the complete route table only for host-navigation enumeration and compatibility metadata; it is no longer authoritative for route selection in WASM builds.

This pass also establishes the ABI surface needed for the next module-graph pass, where package-relative module specifier resolution and dependency ordering can reuse the same WASM-owned string/table state.
