#!/usr/bin/env python3
from pathlib import Path
import sys
root = Path(sys.argv[1]).resolve()
source = (root / 'src/compiler/runtime_js.cpp').read_text(encoding='utf-8')
required = [
    'let activeGeneration = 0;',
    "assertGeneration(requestGeneration, 'fetch response')",
    "stale route generation rejected for event dispatch",
    "stale route generation rejected for QuickJS execution",
    'activateRouteGeneration(generation)',
    'crypto.getRandomValues',
    'routeGeneration: boundGeneration',
]
missing = [item for item in required if item not in source]
if missing:
    raise SystemExit('missing lifecycle hardening markers: ' + ', '.join(missing))
if "record-and-fallback" in source[source.find('function createAsyncHostQueue'):source.find('function createEventQueue')]:
    raise SystemExit('async lifecycle queue contains fallback policy')
print('[venom] route generation lifecycle smoke: PASS')
