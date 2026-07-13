# WASM-owned package parser

v0.88.0 makes the WASM runtime authoritative for package header and section-table parsing. JavaScript fetches the package and copies it into WASM memory. `venom_wasm_parse_package` validates the header, version/ABI, package hash, release flags, section types and flags, name and payload ranges, stored hashes, duplicate section identities, and overlapping payloads.

Only a compact validated metadata projection is returned to the loader. Section decode continues through the WASM-owned indexed decoder and reuses the retained authoritative package state. Protected boot fails closed when parsing or metadata projection fails.
