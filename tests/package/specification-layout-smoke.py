from pathlib import Path
root = Path(__file__).resolve().parents[2]
contracts = root / 'contracts'
required = {
    'browser-fixture.schema.json', 'browser-performance.json', 'host-api.json',
    'product-contracts.json', 'runtime-benchmark.json', 'runtime-performance.json',
    'runtime-resource-policy.json'
}
assert contracts.is_dir()
assert required.issubset({p.name for p in contracts.iterdir() if p.is_file()})
assert (root / 'src' / 'generated' / 'include' / 'venom' / 'generated' / 'contracts' / 'product_contracts.hpp').is_file()
print('contracts layout: PASS')
