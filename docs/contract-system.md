# Product contract system

Venom's public binary and configuration contracts are defined in one source of truth:

```text
contracts/product-contracts.json
```

Run:

```bash
python tools/generate_product_contracts.py
python tools/generate_product_contracts.py --check
```

Generated bindings are consumed by the C++ package/compiler implementation and Python release tooling. Contract names are explicit so the package runtime ABI, Route VM contract, QuickJS runtime ABI, and host bridge ABI cannot be confused.
