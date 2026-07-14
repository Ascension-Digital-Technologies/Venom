# Versioning policy

> **Applies to:** Venom 1.x  
> **Standard:** Semantic Versioning 2.0.0

Venom uses one public version number across the executable, source archive, release metadata, README, documentation, and Git tag.

Public versions represent meaningful product releases. Documentation polish, repository cleanup, CI maintenance, comments, formatting, and other release-preparation work do not receive their own version numbers or individual changelog entries.

## Version format

```text
MAJOR.MINOR.PATCH
```

- **MAJOR** changes indicate intentionally incompatible product or public-interface changes.
- **MINOR** changes add meaningful backward-compatible features or capabilities.
- **PATCH** changes fix meaningful user-visible defects without adding a new feature set.

The first public Venom release is **1.0.0**.

## What warrants a release

Create a new public version for changes such as:

- a new protection mode or major compiler capability;
- a new public CLI command or configuration contract;
- meaningful framework or browser compatibility expansion;
- substantial runtime, security, or performance improvements;
- a user-visible defect fix important enough to distribute independently; or
- an intentional breaking change.

Do not create a new public version solely for:

- documentation wording or formatting;
- repository organization and cleanup;
- GitHub workflow presentation;
- comments, names, or internal refactoring with no public behavioral effect;
- release packaging polish; or
- routine CI maintenance.

Small maintenance changes should remain part of the current release branch and may be summarized collectively in the next meaningful release when relevant.

## Internal development identifiers

Commits, pull requests, CI run numbers, and artifact hashes identify work between public releases. Development artifacts may append optional Semantic Versioning build metadata without creating a new release, for example:

```text
1.0.0+git.a1b2c3d
1.0.0+ci.184
```

## Source of truth

`CMakeLists.txt` defines the product version. The generated C++ version header ensures `venom --version` and generated release metadata use the same value.

Before publishing a release, verify that:

- `venom --version` prints the intended version;
- the Git tag is `v<version>`, such as `v1.0.0`;
- the release archive begins with `venom-<version>`;
- the README and documentation index name the same version; and
- the newest changelog heading matches the release.
