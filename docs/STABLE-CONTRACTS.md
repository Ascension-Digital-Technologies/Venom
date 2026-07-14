# Stable product contracts

Venom publishes `CONTRACTS.json` in every binary release. It is the machine-readable compatibility boundary for package readers, the protected runtime, build configuration, and the public bridge.

## Stability rules

- Numeric ABI and schema values never decrease within a major release.
- Named wire contracts do not change within a major release.
- Existing keys are not removed within a major release.
- Incompatible changes require a new major version or an explicitly versioned parallel reader.

## Inspecting contracts

```powershell
venom contracts --format json
```

Release automation generates `CONTRACTS.json` from `src/contracts/product_contracts.hpp` and compares it with the prior published release using `tools/check_contract_upgrade.py`.
