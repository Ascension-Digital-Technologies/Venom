# v0.88.0 WASM-owned package decode boundary

Hardened WASM browser builds now materialize package sections through `venom_wasm_decode_section(package_size, section_index)`. The loader copies the package image into the bounded WASM package buffer once, requests sections by validated table index, and copies only the final plaintext bytes back to JavaScript.

The WASM runtime owns:

- package header and section-table revalidation
- stored-section hash verification
- legacy seal and `VAEAD001` opening
- `VCLZ0008` decompression
- raw-size and output-capacity enforcement

The readable JS decrypt/decompress implementation remains only as the non-WASM development runtime path. Release WASM builds install the WASM decoder before package metadata is materialized, so integrity, feature, route, script, and lazy route sections cross the same WASM-owned boundary.
