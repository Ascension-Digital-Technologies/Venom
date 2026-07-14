#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
module = (root / 'src/generated/runtime/quickjs_engine_module.cpp').read_text(encoding='utf-8')
harness = (root / 'tests/runtime/browser-compat-harness.mjs').read_text(encoding='utf-8')
fixture = (root / 'tests/fixtures/sites/browser-compat-site/assets/compat.js').read_text(encoding='utf-8')
entry = (root / 'tests/fixtures/sites/browser-compat-site/assets/module-entry.js').read_text(encoding='utf-8')
branch = (root / 'tests/fixtures/sites/browser-compat-site/assets/module-branch.js').read_text(encoding='utf-8')
leaf = (root / 'tests/fixtures/sites/browser-compat-site/assets/module-leaf.js').read_text(encoding='utf-8')
doc = (root / 'docs/route-module-fetch-hardening.md').read_text(encoding='utf-8')

for needle in [
    'fetch-response',
    'fetch-headers',
    'host-deny',
    'route-hydration',
    'module-order',
    'module-leaf',
    'module-branch',
    "g.__venomCompatFetchStatus = '200:ok'",
    "g.__venomCompatDenyByDefault = 'blocked-secret.json:denied'",
    "g.__venomCompatModuleOrderText = 'leaf>branch>entry'",
    "g.__venomCompatRouteHydrated = 'about-ready'",
]:
    if needle not in module:
        raise SystemExit(f'missing v0.83 module/replay marker: {needle}')

for needle in [
    'response.status',
    "response.headers.get('x-venom-fixture')",
    "fetch('blocked-secret.json')",
    'venomCompatAsyncThrow',
    '__venomCompatRouteHydrated',
]:
    if needle not in fixture:
        raise SystemExit(f'missing v0.83 fixture marker: {needle}')

for needle in [
    "import { branchValue } from './module-branch.js'",
    "__venomCompatModuleOrderText",
    "__venomCompatModuleGraph = moduleValue + extraValue + branchValue",
]:
    if needle not in entry:
        raise SystemExit(f'missing v0.83 entry module marker: {needle}')
for name, text, needle in [('branch', branch, "import { leafValue } from './module-leaf.js'"), ('leaf', leaf, 'leafValue = 5')]:
    if needle not in text:
        raise SystemExit(f'missing v0.83 {name} module marker: {needle}')

for needle in [
    'fetch response metadata failed',
    'deny-by-default host call failed',
    'route click/hydration handler failed',
    'real-engine host bridge module graph/order replay failed',
    'effectReplayCount || 0) >= 26',
    "'fetch-response', 'fetch-headers', 'host-deny', 'async-errors'",
]:
    if needle not in harness:
        raise SystemExit(f'missing v0.83 harness assertion: {needle}')

for needle in ['v0.88.0', 'fetch status/header', 'deny-by-default', 'module order', 'route hydration', 'venom_qjs_engine_version=83']:
    if needle not in doc:
        raise SystemExit(f'missing v0.83 docs marker: {needle}')

print('route/module/fetch hardening smoke passed')
