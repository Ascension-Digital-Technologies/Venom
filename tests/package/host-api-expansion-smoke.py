#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
module = (root / 'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')
harness = (root / 'tests/runtime/browser-compat-harness.mjs').read_text(encoding='utf-8')
fixture = (root / 'tests/fixtures/sites/browser-compat-site/assets/compat.js').read_text(encoding='utf-8')
doc = (root / 'docs/host-api-expansion.md').read_text(encoding='utf-8')

for needle in [
    'dom-dataset',
    'dom-remove-attribute',
    'dom-querySelectorAll',
    'dom-closest',
    'dom-matches',
    'dom-create-append',
    'fetch-text',
    'async-await',
    'module-graph',
    'route-pushstate',
    "g.__venomCompatModuleGraph = 93",
]:
    if needle not in module:
        raise SystemExit(f'missing v0.83 host bridge module marker: {needle}')

for needle in [
    'dataset.bridge',
    'removeAttribute',
    'querySelectorAll',
    'closest',
    'matches',
    'createElement',
    'appendChild',
    'response.text',
    'venomCompatAsyncAwait',
    'history.pushState',
    "import { extraValue } from './module-extra.js'",
]:
    if needle not in fixture and needle not in (root / 'tests/fixtures/sites/browser-compat-site/assets/module-entry.js').read_text(encoding='utf-8'):
        raise SystemExit(f'missing v0.83 fixture marker: {needle}')

for needle in [
    'real-engine host bridge dataset/removeAttribute replay failed',
    'real-engine host bridge createElement/appendChild replay failed',
    'real-engine host bridge module graph/order replay failed',
    'real-engine host bridge route hydration replay failed',
    'effectReplayCount || 0) >= 26',
]:
    if needle not in harness:
        raise SystemExit(f'missing v0.83 harness assertion: {needle}')

for needle in ['v0.88.0', 'dataset', 'querySelectorAll', 'history.pushState', 'venom_qjs_engine_version=83']:
    if needle not in doc:
        raise SystemExit(f'missing v0.83 docs marker: {needle}')

print('host API expansion smoke passed')
