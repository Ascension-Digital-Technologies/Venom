# Exit codes and automation

Venom commands return `0` on success and a nonzero value when validation, compilation, runtime verification, release policy, or input processing fails.

## Automation guidance

1. Treat any nonzero exit code as a failed step.
2. Capture both standard output and standard error.
3. Prefer structured JSON output when a command offers `--format json` or `--json-out`.
4. Do not infer success from the presence of an output directory; production builds are fail-closed and may remove or leave incomplete staging output after a rejected build.
5. Run `venom release-check` or the repository release closure before publication.

Venom does not currently promise a permanent public mapping between individual numeric nonzero values and error categories. Automation should rely on zero versus nonzero status plus structured diagnostics, rather than hard-coding undocumented numeric values.
