#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
cpp = (root / 'src/pipeline/chrome_extension.cpp').read_text(encoding='utf-8')
hpp = (root / 'src/pipeline/chrome_extension.hpp').read_text(encoding='utf-8')
required_cpp = [
    'venom-extension-rpc.js',
    'venom-extension-host.js',
    'venom-extension-broker.js',
    'venom-background.js',
    'MAX_INFLIGHT',
    'max_payload_bytes',
    'HOST_CALL',
    'Protected runtime host did not initialize',
    'importScripts',
    "import './venom-extension-broker.js'",
]
for token in required_cpp:
    assert token in cpp, token
for token in ('venom-extension-rpc-v1', 'RpcPolicy', 'max_payload_bytes', 'max_inflight', 'default_timeout_ms', 'max_timeout_ms'):
    assert token in hpp, token
print('Chrome extension RPC generation smoke: PASS')
