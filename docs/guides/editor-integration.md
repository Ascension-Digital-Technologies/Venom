# Editor and language-server integration

Venom 2.0.0 includes a dependency-free Language Server Protocol foundation:

```bash
python tools/venom_lsp.py
```

It supports document synchronization, diagnostics, runtime hover information, and quick fixes for missing runtime directives. Editors should launch it over standard input/output.

The graph query tool supports CI and interactive investigation:

```bash
python tools/venom_query.py examples/aegis-operations health --json
python tools/venom_query.py examples/aegis-operations dependencies browser/app.ts
python tools/venom_query.py examples/aegis-operations boundaries --json
```

Diagnostics are backed by `contracts/diagnostics.json`, so editor messages use the same stable codes as command-line reports.
