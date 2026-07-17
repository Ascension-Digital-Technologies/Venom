# Browser module linking

Production packages redact browser module source paths. Each browser ES module now carries a compiler-generated, hex-encoded import map that binds its original relative specifiers to the opaque source IDs stored in the VBC package. The runtime consumes this map before its legacy path-based fallback.

This keeps production source-path redaction intact while allowing extensionless JavaScript and TypeScript imports, including generated protected-module facades, to resolve from blob-backed browser modules.
