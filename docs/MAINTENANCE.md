# Documentation maintenance

> **Applies to:** Venom 1.65.2

Documentation is part of the release contract.

## Canonical locations

- `README.md`: product overview and quick start
- `docs/README.md`: documentation index
- `docs/getting-started/`: installation and first-use workflows
- `docs/guides/`: integration guidance
- `docs/architecture/`: system design
- `docs/security/`: current threat model and hardening controls
- `docs/reference/`: command and API reference
- `docs/development/`: contributor and release workflows

## Release requirements

Before publishing a version:

- update the current version in CMake, README, changelog, and citation metadata;
- remove obsolete release-candidate reports from stable archives;
- run `python tools/documentation_gate.py`;
- run `python tools/final_release_gate.py --run-smoke-tests`;
- verify every local Markdown link resolves inside the repository;
- verify documented commands against a clean extracted source archive;
- avoid absolute security claims.

Historical deep-reference pages may remain, but current guides should link to the categorized canonical page first.
