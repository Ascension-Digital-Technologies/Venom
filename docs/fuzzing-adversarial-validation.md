# v0.94.0 fuzzing and adversarial validation

Venom v0.94.0 introduces a permanent hostile-input program for the package and browser-runtime boundaries.

## Native package parser boundary

`read_package_bytes` exposes the same bounded parser used by file-based inspection without requiring a filesystem round trip. The native parser now enforces:

- 64 MiB maximum encoded package size
- 4,096 maximum sections
- 1 MiB maximum section-name table
- 32 MiB maximum decoded section
- 128 MiB maximum aggregate decoded package data
- duplicate `(type, name)` rejection
- overlapping payload range rejection
- checked 64-bit ranges and decoded-size accumulation

## Fuzz targets

Configure with Clang:

```sh
cmake -S . -B build-fuzz \
  -DVENOM_BUILD_FUZZERS=ON \
  -DVENOM_ENABLE_SANITIZERS=ON \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build-fuzz --target venom_package_parser_fuzz
./build-fuzz/venom_package_parser_fuzz tests/fuzz/corpus/package_parser -max_total_time=300
```

Non-Clang builds still provide `venom_package_parser_replay` for deterministic corpus replay in CI.

## Regression corpus

`adversarial-package-corpus-smoke.py` builds a valid control package and derives malformed cases for truncation, bad hashes, integer/range abuse, unknown flags/types, excessive decoded sizes, duplicate section identities, and overlapping payloads. Every malformed case must be rejected within three seconds; the valid control package must remain accepted.

`worker-hostile-input-smoke.py` preserves the worker nonce, message ceilings, startup timeout, redirect denial, fetch limits, queue limits, abort ownership, and disposal-state gates.

Any fuzzer-discovered crash input should be copied into `tests/fuzz/corpus/package_parser` and retained permanently.
