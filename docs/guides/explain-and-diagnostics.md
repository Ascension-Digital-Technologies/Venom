# Explain and diagnostics

Venom 1.7.0 introduces a contract-backed project explanation report. It inspects authored JavaScript, TypeScript, JSX, and TSX before transformation so runtime directives and extensionless dependencies are reported exactly as written by the developer.

```bash
python tools/venom_explain.py examples/aegis-operations \
  --json-out build/aegis-explain.json \
  --markdown-out build/aegis-explain.md \
  --dot-out build/aegis-modules.dot
```

The report includes module ownership, relative dependency resolution, browser-to-protected boundaries, unresolved imports, source hashes, build evidence, cache summaries, and stable diagnostic codes. Use `--fail-on warning` for strict CI or `--fail-on never` for exploratory reports.

The governing code catalog is `contracts/diagnostics.json`. Tools and documentation must evolve with that versioned contract.

The optional Graphviz output colors browser modules blue, protected modules red, unspecified modules gray, and conflicting declarations gold.
