# Contributing to Venom

Thank you for helping improve Venom. Contributions should preserve the project's security boundaries, fail-closed behavior, portability, and documentation quality.

## Before opening a change

1. Search existing issues and pull requests to avoid duplicate work.
2. For substantial architecture, package-format, runtime, or security changes, open a discussion or proposal first.
3. Never include private application source, customer data, signing keys, generated distributions, SDK downloads, or build output.

## Development workflow

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release
```

Use the platform-specific presets and scripts documented in [Build from source](docs/getting-started/build-from-source.md). Run the narrowest relevant tests while developing, then run the repository and documentation gates before submitting.

## Pull request expectations

A focused pull request should include:

- a clear problem statement and rationale;
- implementation notes for security- or compatibility-sensitive behavior;
- tests that cover the change and important failure cases;
- updated documentation when commands, configuration, output, or guarantees change; and
- confirmation that no generated or confidential artifacts are included.

Keep unrelated refactors separate. Preserve existing command behavior unless the change explicitly introduces a documented compatibility break.

## Coding and documentation standards

- Follow `.clang-format` and `.editorconfig`.
- Prefer explicit error handling and fail-closed behavior at trust boundaries.
- Keep platform-specific behavior isolated and tested.
- Write documentation for users first: state prerequisites, commands, expected output, limitations, and recovery steps.
- Do not make security claims broader than the implemented and tested behavior.
- Keep local Markdown links valid and version references current.

## Security reports

Do not disclose suspected vulnerabilities in a public issue or pull request. Follow [SECURITY.md](SECURITY.md).

## Community conduct

Participation is governed by [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
