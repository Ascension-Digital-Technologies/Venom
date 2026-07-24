# Package Section Planning

Venom constructs every VBC package through two explicit layers:

1. `package_sections::make_plan()` derives the complete logical section set from the canonical module graph, compiled routes, runtime artifacts, security policy, and public assets.
2. `package_plan::Plan::write_verified()` applies writer policy, emits the VBC artifact, and immediately reopens it through the canonical reader.

The build orchestrator does not construct individual package sections. This prevents development, production, and lazy-route paths from maintaining independent section assembly behavior.

## Inputs

The section planner receives a typed, read-only `Inputs` object containing the build profile, module graph outputs, route bytecode, runtime artifacts, vendor policy, asset pipeline, and polymorphic plan.

## Outputs

The planner returns:

- the complete logical `PackagePlan`;
- the package binding token used by the bootstrap loader;
- package feature flags;
- the layout padding budget used by the writer.

## Section groups

The planner owns four logical groups:

- release, integrity, and binding metadata;
- TurboJS/WASM runtime contracts and host policies;
- application routes, browser modules, protected registries, CSS, and assets;
- layout, lazy-loading, feature, and integrity seals.

All sections pass through the same encryption decision and the same canonical plan before writing.
