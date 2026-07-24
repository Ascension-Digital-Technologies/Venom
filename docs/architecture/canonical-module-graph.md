# Canonical Module Graph

Venom builds one canonical module graph after JavaScript/TypeScript discovery and protected-module rewriting. The graph is the authoritative source for module identity, runtime, resolution, dependency edges, and runtime source labels.

## Responsibilities

The graph owns:

- stable compiler module IDs;
- opaque production runtime IDs;
- normalized source paths;
- browser and protected runtime identity;
- static, dynamic, and preload dependency edges;
- extensionless JavaScript and TypeScript resolution;
- browser module-map generation.

Both the general JavaScript bundle serializer and lazy route serializer consume the same graph. They do not independently infer module paths or generate opaque aliases.

## Resolution

Relative module requests are resolved through exact paths, JavaScript/TypeScript extensions, and index modules. A `.js` request may resolve to a TypeScript source when that source was the compiler input.

Unresolved and external boundaries remain explicit graph edges and are handled by planner policy rather than guessed by the package writer.

## Runtime identity

Development output retains readable source labels. Protected production output uses graph-owned `s_<digest>` aliases. The same alias is used by browser module maps, route bundles, TurboJS module records, and diagnostics.
