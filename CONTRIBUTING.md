# Contributing to Venom

Thank you for helping improve **Venom - Secure Web Runtime**.

## Before opening a change

1. Open an issue describing the problem, intended behavior, and security or
   compatibility implications.
2. Keep changes focused. Avoid mixing formatting, generated artifacts, and
   functional changes in one pull request.
3. Do not commit build directories, generated distributions, Emscripten SDK
   files, private keys, release signatures, or fuzz crash artifacts.

## Development workflow

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

For a strict release-oriented check:

```bash
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
```

Run the repository consistency and Python syntax checks before submitting:

```bash
python3 tests/package/repository-consistency-smoke.py
python3 -m compileall -q tools tests
```

## Code expectations

- C code targets C11; C++ code targets C++17.
- Preserve fail-closed production behavior.
- Add regression coverage for every bug fix.
- Keep platform-specific behavior behind the existing CMake and script layers.
- Update the README or relevant document when changing CLI behavior, output
  layout, security policy, compatibility, or toolchain requirements.

## Pull requests

Include a concise summary, test evidence, platform/toolchain details, and any
security or compatibility tradeoffs. Generated release packages should not be
attached to normal source pull requests.
