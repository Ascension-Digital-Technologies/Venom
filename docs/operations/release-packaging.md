# Release Packaging — Venom 3.0.0

Venom binary releases are platform-specific, integrity-manifested packages. Each package must include the main compiler and its mandatory hardener worker.

## Required binary payload

```text
bin/
├── venom[.exe]
└── venom_hardener_worker[.exe]
```

The packager and verifier must fail when either executable is missing. The worker is signed, checksummed, installed, upgraded, and removed as part of the same release unit as `venom`.

## Canonical package names

```text
venom-3.0.0-windows-x86_64.zip
venom-3.0.0-linux-x86_64.tar.gz
```

Additional target packages should follow the same `<product>-<version>-<platform>-<architecture>` pattern.

## Package contents

```text
venom-3.0.0-<target>/
├── bin/
│   ├── venom[.exe]
│   └── venom_hardener_worker[.exe]
├── LICENSE
├── NOTICE.md
├── README.txt
├── RELEASE_NOTES.md
└── SHA256SUMS
```

`SHA256SUMS` records the digest of every shipped file except itself. Archive-level digests should be published separately alongside the downloads.

## Build and package from source

Linux:

```bash
bash scripts/linux/build.sh --config Release
python tools/package_release.py --build-dir build --archive tar.gz --target-triplet linux-x86_64
```

Windows PowerShell:

```powershell
.\scripts\windows\build.ps1 -Config Release
python tools/package_release.py --build-dir build\Release --archive zip --target-triplet windows-x86_64
```

Use the repository release scripts when producing official artifacts; they perform the project-specific manifest, provenance, signing, and verification steps around the lower-level packager.

## Verification

A release is incomplete unless all of the following pass:

- both executables are present and executable;
- `venom --version` reports the expected version;
- the hardener-worker stress test passes;
- every file matches `SHA256SUMS`;
- the archive extracts without traversal, links, devices, or duplicate portable paths;
- release signing and provenance checks pass when required; and
- the extracted package passes the installation smoke test.

Never weaken verification to permit a main-binary-only package.

## Signing boundary

Sign both executables before generating the final file manifest. Sign the release manifest or archive according to the release policy, then verify the fully assembled archive from a clean extraction directory. See [Release signing](../security/release-signing.md).
