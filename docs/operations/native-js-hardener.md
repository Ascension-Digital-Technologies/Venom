# Native JavaScript hardener

Venom embeds the browser distributions of Terser 5.49.0 and
`javascript-obfuscator` 5.4.7 in the native compiler and evaluates them with
the QuickJS engine already linked into `venom_core`.

## Compatibility contract

The native path preserves the established production-hardening contract:

- identical Terser options;
- identical role-specific obfuscator options;
- identical FNV-derived deterministic seed;
- identical loader binding marker extraction and append behavior;
- no source maps;
- byte-identical output for loader, engine, runtime, and worker parity fixtures.

## Build behavior

Production builds execute the hardener entirely in-process. No Node hardener scripts, npm package metadata, `node_modules`, temporary hardener files, or external hardener processes are included in the repository or release workflow.

## Updating dependencies

An upgrade must pin exact package versions, regenerate the embedded browser
bundles, preserve third-party license notices, and pass byte-level or explicitly
reviewed semantic parity tests before replacing the current payload.
