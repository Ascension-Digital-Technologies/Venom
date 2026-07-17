# CLI reference

## Project lifecycle

```text
venom new <path> [--force]
venom init [path] [--force]
venom dev [path] [--out <dir>] [--host <host>] [--port <port>] [--open] [--no-watch]
venom build <path> --profile prod --out <dir> [--verbose|--quiet]
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
venom doctor --profile prodelopment|production [--format text|json]
venom compatibility check <site-dir> [--format text|json]
venom analyze <dist-dir> [--format text|json]
venom verify <dist-dir>
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

### Build cache controls

```text
--no-cache
--cache-dir <path>
```

Venom caches deterministic native-hardener results by content hash. Cache identity includes the hardener versions, asset role, deterministic seed, and complete input source. Final package polymorphism, integrity binding, and release verification still run for every build.
