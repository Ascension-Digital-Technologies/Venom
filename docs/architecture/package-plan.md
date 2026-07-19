# Canonical Package Plan

Venom constructs every VBC artifact through `pipeline/planning/package_plan`.

The package plan owns the ordered section set, polymorphic section shuffling, writer policy, artifact emission, and immediate read-back verification. Development and production builds supply different policy values to the same plan rather than configuring separate writers.

## Pipeline

1. Compiler stages add typed sections to a `package_plan::Plan`.
2. Integrity, feature, layout, and lazy-section metadata are derived from the plan's exact section set.
3. Optional polymorphic ordering is applied once by the plan.
4. A single `WriterOptions` record controls compression, encryption, name redaction, layout padding, and the crypto provider.
5. `write_verified` writes the VBC file and immediately reopens it with the canonical package reader.

The build pipeline never transfers sections into `package::Writer` directly. This prevents development and production serialization paths from drifting.
