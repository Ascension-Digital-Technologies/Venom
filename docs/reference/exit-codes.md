# Exit Codes

> **Applies to:** Venom 1.0.1

Venom returns nonzero process codes for configuration, build, runtime-verification, compatibility, packaging, and release-policy failures. Automation should treat any nonzero result as failure and preserve standard output and error logs.

## Stable categories

| Category | Meaning |
|---|---|
| Success | The requested command completed and all required checks passed. |
| Usage/configuration | Arguments, paths, configuration, or required inputs are invalid. |
| Toolchain | A compiler, Python, Node, npm, hardener, CMake, or runtime prerequisite is unavailable. |
| Build | Compilation, transformation, packaging, or asset generation failed. |
| Compatibility | The site uses unsupported behavior or a required browser qualification failed. |
| Verification | Runtime provenance, package binding, asset integrity, or leakage policy failed. |
| Release policy | Signing, release-set, source completeness, or publication policy failed. |

The exact numeric values are emitted by the CLI and should be read from the installed version's `--help` and automation output. Scripts should not convert a failing production command into success.

## PowerShell

```powershell
venom release-check dist
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

## Shell

```bash
venom release-check dist
```

With `set -e`, the shell exits immediately on failure. Preserve the command logs as release evidence.
