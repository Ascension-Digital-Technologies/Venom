# Private Report Migration

Venom build diagnostics are no longer written into public distributions.

For an output directory such as:

```
/path/to/project/dist-venom
```

private diagnostics are written to:

```
/path/to/project/.venom/dist-venom/
```

The private tree contains:

- `build.json`
- `build-performance.json`
- `protection-intents.json`
- `reports/execution-plan.{txt,json}`
- `reports/function-plan.{txt,json}`
- `reports/function-extraction-plan.{txt,json}`
- `reports/runtime-bridge-contract.json`
- `reports/bridge-rewrite-plan.json`
- `api/` generated private API contracts and clients

Release tests and release tooling now resolve this private sibling location instead of requiring diagnostics under public `dist` assets.

## Completion verification

The migration was completed against the native Release binary. The JSON build result now returns the explicit private `build.json` path, while public distributions remain free of internal reports. Current v3 runtime-specialization and bridge-extraction tests were also updated to validate behavior and generated contracts rather than obsolete asset directories or implementation-symbol names.

Verified gates include production readiness, minimal public metadata, runtime module specialization, function runtime/extraction/bridge plans, clean production layout, dependency lifting, JavaScript AST lowering/scope/planning/module closure, TypeScript directive preservation, and final readiness reporting.
