# Application-specialized runtime modules

Venom 1.27 derives a runtime-module plan from the syntax-aware capability graph before generating protected browser JavaScript.

The always-present core is:

- `core`
- `route`
- `dom`
- `quickjs`

Optional modules currently include:

- `network`
- `timers`
- `events`
- `storage`
- `navigation`

Network, timer, and event implementations are physically specialized in v1.27. When unused, their full implementation is replaced before minification with a compact fail-closed stub. The resulting protected runtime is smaller and exposes fewer recognizable browser adapters. Storage and navigation are recorded in the plan and remain conservative compatibility modules in this release.

The selected module list is stored in `assets/app/build.json` as `runtime_modules`. This is build provenance, not a browser-controlled policy override. Unknown or omitted operations remain denied.

The capability graph is intentionally conservative. Real-browser fixtures remain required because syntax analysis cannot prove runtime behavior, dynamically constructed access, or all framework-generated code paths.

## DOM compatibility modules

The runtime planner now treats these as independently selectable compatibility surfaces:

- `forms`: `FormData`, `SubmitEvent`, and form/property reflection patterns;
- `observers`: `MutationObserver`, `ResizeObserver`, and `IntersectionObserver`;
- `animation`: `requestAnimationFrame` and `cancelAnimationFrame`;
- `events`: event constructors, listener registration, dispatch, and propagation-related usage.

Selected browser constructors are copied into the QuickJS execution binding table only when their module is enabled. The engine module preserves all compiler-selected bindings rather than reconstructing a fixed global list.
