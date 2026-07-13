#!/usr/bin/env python3
from pathlib import Path
import argparse

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("wasm", type=Path)
    a=ap.parse_args()
    b=a.wasm.read_bytes()
    required=[b"v12_p",b"v12_n",b"v12_x"]
    forbidden=[b"venom_wasm_materialize_section",b"venom_wasm_materialized_section_ptr",b"venom_wasm_package_meta_ptr",b"venom_wasm_route_record_ptr",b"venom_wasm_domops_ptr"]
    missing=[x.decode() for x in required if x not in b]
    leaked=[x.decode() for x in forbidden if x in b]
    if missing or leaked:
        if missing: print("missing compact exports:", ", ".join(missing))
        if leaked: print("descriptive ABI leak:", ", ".join(leaked))
        return 2
    print("[venom] opaque package WASM ABI: PASS (3 compact exports)")
    return 0
if __name__=="__main__": raise SystemExit(main())
