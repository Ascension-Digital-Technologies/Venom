# CLI reference

## Project lifecycle

```text
venom new <path> [--force]
venom init [path] [--force]
venom dev [path] [--out <dir>] [--host <host>] [--port <port>] [--open] [--no-watch]
venom build <path> --profile dev|prod --out <dir> [--verbose|--quiet]
```

## Build output controls

`venom build` prints structured phase progress by default, including major analysis, bytecode, packaging, hardening, asset-binding, and verification stages.

```text
--verbose, -v  Include detailed planner, runtime, package, seed, and hardener diagnostics.
--quiet, -q    Suppress phase progress while retaining errors and the final result.
```

`--verbose` and `--quiet` are mutually exclusive. Machine-readable output remains clean when `--format json` is selected.

## Analysis and verification

```text
venom doctor --profile development|production [--format text|json]
venom analyze <site-dir> [--format text|json]
venom compatibility check <site-dir> [--format text|json]
venom analyze-dist <dist-dir> [--format text|json]
venom release-check <dist-dir>
venom verify-runtime <dist-dir> --require-real-engine
venom inspect <package> [--key-file <path>]
```

## Runtime, configuration, contracts, and updates

```text
venom runtime install|verify|update|path [--dir <path>] [--force]
venom config validate|print [path] [--format text|json]
venom contracts [--format text|json]
venom update check|install|rollback|status [options]
```

Run `venom --help` for the exact commands supported by the current binary.
