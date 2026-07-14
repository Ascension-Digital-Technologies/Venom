# Per-build bytecode permutation

> **Applies to:** Venom 1.65.2
> **Security goal:** make stored QuickJS records build-specific without forking the QuickJS bytecode ABI.

Venom 1.61 adds a validated, per-build lane permutation to the `VQJSE006` protected bytecode envelope. The canonical QuickJS record remains compatible with the pinned upstream engine, but its stored representation is no longer a simple stream-XOR transform.

## Record lifecycle

```text
canonical VQJSBC03 / VQJSMB04 record
→ derive envelope seed from build salt, route, source, and order
→ generate a 16-lane Fisher–Yates permutation
→ permute each record block
→ apply the build-specific stream transform
→ store VQJSE006 with map fingerprint and inner-record hash
→ validate and reverse only at the runtime boundary
→ erase the stored encoded copy
→ execute the reconstructed canonical record
```

The final partial block uses a filtered permutation, preserving a one-to-one mapping for every payload length.

## Why this design

Changing actual QuickJS opcode numbers would require a matching custom compiler and interpreter fork for every generated mapping. That would make upstream QuickJS upgrades, bytecode verification, debugging, and compatibility materially riskier. Venom instead diversifies the complete stored bytecode record while preserving the pinned engine ABI after validated reconstruction.

## Validation

The envelope binds and validates:

- envelope version and magic;
- exact QuickJS bytecode ABI fingerprint;
- payload size and offset;
- lane width;
- generated lane-map fingerprint;
- inner canonical-record hash.

A record decoded with a different build seed does not reconstruct the original bytecode and fails integrity validation.

## Security effect

This raises the cost of:

- fixed-offset bytecode carving;
- signatures based on canonical QuickJS record structure;
- reuse of one build's decoder against another build;
- generic extraction scripts that only reverse the previous stream transform.

It does not prevent a browser owner from instrumenting the runtime after reconstruction. The protection is build-specific representation diversification and validation, not permanent client-side secrecy.
