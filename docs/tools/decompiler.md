# Venom Decompiler

The Venom decompiler performs static recovery and inspection of Venom distributions, package files, browser JavaScript, Route VM programs, and protected QuickJS records. It never executes recovered application code.

## Commands

```powershell
venom decompile <dist|package|javascript> --out recovered --force
venom-decompiler <dist|package|javascript> --out recovered --force
```

Use `--key-file <file>` for packages built with an external key. Use `--no-js` to skip browser JavaScript normalization and `--no-qjs-disasm` to skip QuickJS object dumps.

## Protected QuickJS output

For each decoded `VQJSBC03` record, the decompiler writes:

- `quickjs-object.bin` — exact native QuickJS object bytes.
- `quickjs-read-object.txt` — QuickJS structural object dump when full QuickJS support is enabled.
- `record.json` — ABI, hashes, sizes, and module status.
- `static-analysis.json` — entropy and categorized recovery counts.
- `printable-strings.txt` — printable constants retained in the bytecode object.
- `probable-identifiers.txt` — strings shaped like JavaScript identifiers.
- `urls.txt` — probable HTTP, WebSocket, and data URLs.
- `module-paths.txt` — probable import, asset, and module paths.

The exact original protected source cannot be reconstructed because comments, formatting, source maps, and many source-level names are removed before packaging. The output is intended for structural analysis and equivalent-logic reconstruction.

## Safety and integrity

The decompiler validates envelope and bytecode hashes before producing analysis. Corrupted protected records are rejected. All processing is static; decoded QuickJS objects are parsed for structure but not evaluated.
