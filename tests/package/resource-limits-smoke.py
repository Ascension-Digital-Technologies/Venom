#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
b=(root/'src/compiler/build.cpp').read_text()
r=(root/'src/compiler/runtime_js.cpp').read_text()
need=['VENOM_ASYNC_HOST_QUEUE_V10','max_pending=128','max_completed=128','max_host_calls_per_route=8192','max_concurrent_fetches=16','max_dom_handles=4096','max_fetch_response_bytes=1048576','teardown=cancel-timers|abort-fetches|reject-pending|destroy-contexts']
for x in need:
    assert x in b, x
for x in ['maxFetchResponseBytes','AbortController','fetch response size limit exceeded','function dispose(','async host queue is disposed']:
    assert x in r, x
print('resource limits smoke passed')
