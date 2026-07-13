# Compatibility evidence bundles

Venom can package browser-validation reports, fixture manifests, and the aggregate compatibility matrix into one deterministic archive. The bundle is intended for release pages, audit records, and support-claim review.

The evidence manifest records SHA-256 and size for every included file, the Venom version, source revision, browser policy, minimum checks, and whether support promotion passed. Optional Ed25519 signing uses the same offline signing primitives as release packaging.

A valid bundle proves that the included records have not changed since packaging. It does not independently prove that the browsers were honestly executed; that assurance comes from the trusted CI identity and signed release process.
