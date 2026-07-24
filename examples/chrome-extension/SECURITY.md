# Security Policy

## Supported version

Security fixes are applied to the latest release on the default branch.

## Reporting a vulnerability

Please open a private GitHub security advisory rather than a public issue when reporting:

- allowlist bypasses;
- unexpected access to non-allowed pages;
- data exfiltration or network activity;
- extension-context escape;
- unsafe page-to-extension trust assumptions;
- dependency or packaging compromise.

Include reproduction steps, affected files, impact, and a suggested fix when available.

## Security model

Velocity Chess Browser Engine is designed to:

- run locally without a remote engine;
- request only `activeTab` and `scripting` permissions;
- reject global allowlist patterns;
- avoid background workers and extension messaging;
- avoid telemetry and gameplay uploads;
- interact only after explicit user activation.

The project does not claim to provide anti-tamper protection against a user who controls their own browser profile or unpacked extension files.
