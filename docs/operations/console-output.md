# Console output

Venom uses one concise terminal contract across native commands and Python launchers.

- Normal mode prints meaningful progress and a compact result.
- `--verbose` prints complete technical evidence.
- `--quiet` prints only failures.
- `--format json` emits machine-readable output without ANSI formatting.
- Colors are automatic for interactive terminals and disabled for redirected output or `NO_COLOR`.

Verification commands intentionally suppress their historical field-by-field reports unless `--verbose` is supplied.
