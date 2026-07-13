from pathlib import Path
root = Path(__file__).resolve().parents[2]
c = (root / "src/runtime/wasm_runtime.c").read_text()
bridge = (root / "src/compiler/wasm_runtime_js.cpp").read_text()
runtime = (root / "src/compiler/runtime_js.cpp").read_text()
for token in ["venom_wasm_materialize_section", "venom_wasm_materialized_section_ptr", "venom_wasm_materialized_section_size"]:
    if token not in c:
        raise SystemExit("missing private WASM-owned materialization implementation: " + token)
    if token in bridge:
        raise SystemExit("descriptive package WASM ABI leaked into browser bridge: " + token)
for token in ["v12_p", "v12_n", "v12_x"]:
    if token not in c or token not in bridge:
        raise SystemExit("missing compact v1.0.12 package ABI: " + token)
if "installVenomWasmPackageDecoder(wasmRuntime)" not in bridge:
    raise SystemExit("WASM package decoder is not installed before package loading")
if "venomPackageSectionDecoder(pkg, sectionIndex" not in runtime:
    raise SystemExit("lazy package sections do not route through the WASM decoder")
if "decodeSectionData(section.storedData" not in runtime or ", index, pkg)" not in runtime:
    raise SystemExit("section index/package ownership is not carried into lazy decode")
print("wasm-owned package decode smoke: ok")
