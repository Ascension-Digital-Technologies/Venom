# Fuzzing

- `targets/` contains the native fuzz and deterministic replay translation units built by CMake.
- `corpus/` contains checked-in seed inputs used by replay and CI gates.

Fuzzing is test infrastructure, so both targets and corpora are colocated under `tests/fuzz/` rather than split across the repository root.
