# Security policy

## Supported versions

Security fixes are applied to the latest release line. Older development
snapshots and generated distributions should be upgraded before reporting a
runtime issue.

## Reporting a vulnerability

Do not open a public GitHub issue for a suspected vulnerability. Send a private
report to **mario.appledev@gmail.com** with:

- the affected version or commit;
- the operating system, compiler, browser, and build profile;
- reproduction steps and a minimal test case;
- expected and observed behavior;
- impact assessment and any proposed mitigation.

Please avoid including production secrets, signing keys, private site source,
or customer data. Acknowledgement and remediation timing depend on severity and
reproducibility.

## Security boundaries

Venom raises the cost of extracting and tampering with shipped application
logic, but it cannot make browser-delivered code or data absolutely secret. A
hostile client controls its browser, storage, network stack, and debugging
environment. See [docs/threat-model.md](docs/threat-model.md) and
[docs/security-model.md](docs/security-model.md) before deploying.
