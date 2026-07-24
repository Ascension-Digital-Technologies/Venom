# Source boundaries and tooling pass — 2026-07-18

## Goals

- Replace duplicated example metadata with one canonical registry.
- Make launchers generated, semantic, and platform-symmetric.
- Establish one typed C++ error foundation without introducing dependency cycles.
- Detect generated source drift before release packaging.
- Keep shared Python tooling in an importable package instead of copying process, hashing, and JSON helpers.

## Resulting ownership changes

- Added `src/base/` as a dependency-free native foundation.
- Added `src/core/error_reporting.*` for console-aware rendering.
- Converted authored `std::runtime_error` throws to `venom::raise_error` with stable domain codes.
- Added `tools/venom_tools/` for shared example, I/O, and subprocess helpers.
- Upgraded `contracts/examples.json` to `VENOM_EXAMPLE_REGISTRY_V2`.
- Added generated launchers for all ten examples, including Vite.
- Added static checks for example registry integrity, launcher drift, error API use, and generated runtime snapshot drift.

## Dependency correction

The first implementation placed the common error type in `core`. That made `package`, `turbojs`, and `vm` depend on `core`, while `core` already consumed `package`, creating a cycle. The final design places only dependency-free error types and exit codes in `base`; rendering remains in `core` because it depends on console phase state.

## Canonical checks

```sh
python tools/architecture/check_domain_dependencies.py .
python tools/check_example_registry.py .
python tools/generate_example_launchers.py --repo-root . --check
python tools/check_generated_artifacts.py .
python tests/package/error-api-smoke.py .
```

## Windows entrypoint consolidation

- Added one shared completion policy in `tools/windows-scripts/internal/finish-command.bat`.
- PowerShell wrappers delegate through `invoke-powershell.bat`; semantic example wrappers delegate through `invoke-example.bat`.
- Success, failure, pause suppression, and CI behavior are no longer copied across public launchers.
- Added symmetric `release-closure` launchers and corrected release PowerShell repository-root resolution.

## Regression ownership updates

Static tests now follow the new ownership boundaries instead of old paths. In particular, TurboJS/WASM scripts are read from `tools/*-scripts/`, worker-runtime checks include `worker_runtime_template.hpp`, package checks accept hash-only `.vbc` names, and pipeline checks scan the promoted implementation units that own the behavior.
