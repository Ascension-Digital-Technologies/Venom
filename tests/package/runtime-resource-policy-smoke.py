#!/usr/bin/env python3
import json
from pathlib import Path
root = Path(__file__).resolve().parents[2]
policy = json.loads((root / 'contracts/runtime-resource-policy.json').read_text())
assert policy['schema_version'] == 1
limits = policy['limits']
expected = {
    'max_pending_host_calls': 128,
    'max_completed_host_calls': 128,
    'max_host_calls_per_route': 8192,
    'max_concurrent_fetches': 16,
    'max_active_timers': 128,
    'max_timers_scheduled_per_route': 512,
    'max_event_records': 256,
    'max_event_dispatches_per_route': 1024,
    'max_dom_handles': 4096,
    'max_route_lifetime_ms': 86400000,
}
for key, value in expected.items():
    assert limits[key] == value, (key, limits.get(key))

build = (root / 'src/compiler/pipeline/build.cpp').read_text()
runtime = (root / 'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
for marker in [
    'max_pending=128', 'max_completed=128', 'max_host_calls_per_route=8192',
    'max_concurrent_fetches=16', 'max_dom_handles=4096', 'max_active=128',
    'max_scheduled_per_route=512', 'max_records=256', 'max_dispatches_per_route=1024',
]:
    assert marker in build, marker
for marker in [
    'Venom per-route host-call budget exceeded',
    'Venom concurrent fetch limit exceeded',
    'Venom per-route timer schedule budget exceeded',
    'Venom per-route event dispatch budget exceeded',
    'Venom route lifetime limit exceeded',
    'maxDomHandles',
    'routeStartedAt',
    'hostCallsThisRoute',
]:
    assert marker in runtime, marker
print('runtime resource policy smoke passed')
