# Build console

Venom's build output is organized into numbered phases with elapsed timing, configuration context, detailed diagnostics, and a final artifact/security summary.

## Output levels

```bash
venom build site --profile prod
venom build site --profile prod --verbose
venom build site --profile prod --quiet
```

- Default output shows phases, important counts, protection status, and final artifacts.
- `--verbose` adds planner, cache, module graph, runtime, package, and hardener details.
- `--quiet` suppresses progress while preserving errors and a final result.
- JSON output remains free of decorative console messages.

## Color control

Venom automatically enables ANSI colors for interactive terminals and supports Windows virtual-terminal consoles.

Colors are enabled automatically whenever Venom is writing to an interactive terminal. Redirected output and machine-readable JSON remain free of ANSI escape sequences. The standard `NO_COLOR` environment variable is respected for environments that require plain text.

## Interactive phase pacing

Interactive terminal builds pause for one second after each completed phase before displaying the next phase. This keeps very fast compiler stages readable and prevents several status blocks from flashing past at once. The presentation delay is disabled for JSON, quiet, redirected, and non-interactive output, and it is excluded from reported compiler phase and total build timings.
