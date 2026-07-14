# Runtime integrity seals

Venom 1.62.0 adds distributed, build-bound integrity checks around critical runtime state.

The protected worker embeds a compiler-generated seal covering the capability identifiers, protected candidate registry, registry bytecode, and bridge opcodes. The worker recomputes this seal at startup and before every protected invocation. A mismatch closes the bridge and fails closed.

The browser runtime also seals the active release-diversification record, QuickJS ABI fingerprint, and route opcode map after package validation. It rechecks that state before route scripts execute, so casual mutation of these tables does not silently continue.

These checks increase tamper-detection coverage and force an analyst to patch both state and verification logic. They do not create a trusted boundary against someone who controls the browser engine and can modify all code involved.
