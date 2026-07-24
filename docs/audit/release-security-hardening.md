# Release Security Hardening

This pass hardens two trust boundaries used by Venom 2.0 release tooling and VBC package parsing.

## Archive extraction

`tools/verify_release.py` now rejects archive entries that can redirect extraction outside the temporary verification root:

- ZIP symbolic links
- TAR symbolic links and hard links
- TAR device and FIFO entries
- absolute paths
- drive-qualified paths
- parent-directory traversal

Release archives are expected to contain ordinary directories and regular files only.

## Release manifest paths

Manifest rows now reject:

- Windows-style backslash paths
- drive-qualified paths
- control characters
- empty, current-directory, and parent-directory components
- negative file sizes
- duplicate file entries

This keeps release manifests portable and prevents platform-dependent path interpretation.

## VBC section names

Package section names now pass through the same validation in both the writer and reader. Names must be non-empty, at most 1024 bytes, and contain no ASCII control characters. This prevents ambiguous integrity keys, multiline diagnostics, and oversized individual name entries.

## Regression evidence

The focused security corpus is in:

- `tests/package/release-archive-safety-smoke.py`

It constructs malicious ZIP and TAR link entries and invalid manifest paths, and verifies fail-closed behavior. The native `venom_core` target and ABI lock are also checked after these changes.
