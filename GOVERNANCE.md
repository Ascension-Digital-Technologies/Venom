# Governance

Venom is maintained through technical review, reproducible release gates, and documented security boundaries.

## Decision principles

1. Production safety and fail-closed behavior take priority over convenience.
2. Browser compatibility changes require executable evidence.
3. Security claims must match implemented and tested behavior.
4. Public interfaces remain stable unless a versioned migration is documented.
5. Generated artifacts must be reproducible from canonical source inputs.

## Changes

Substantial architecture, package-format, bridge-protocol, runtime, or release-policy changes should include:

- a design explanation;
- tests or compatibility evidence;
- migration notes when public behavior changes;
- updated documentation;
- successful release closure.

## Releases

Stable releases are produced only by the canonical release workflow. Release sets are signed, provenance is verified, and production packages must pass the repository release gates.
