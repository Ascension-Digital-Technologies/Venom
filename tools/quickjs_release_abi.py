#!/usr/bin/env python3
"""Emit the intentionally small QuickJS browser release ABI.

Debug/test builds keep the full VENOM_QJS_PUBLIC surface. Protected browser
artifacts use only this execution-oriented allowlist.
"""
import json, sys
RELEASE_ABI = [
    'venom_qjs_context_alloc',
    'venom_qjs_context_free',
    'venom_qjs_context_set_limits',
    'venom_qjs_context_set_execution_limits',
    'venom_qjs_script_buffer_alloc',
    'venom_qjs_script_buffer_capacity',
    'venom_qjs_script_buffer_set_expected_hash',
    'venom_qjs_script_buffer_free',
    'venom_qjs_bytecode_validate',
    'venom_qjs_execute_bytecode',
    'venom_qjs_compact_result_ptr',
    'venom_qjs_diversification_install',
    'venom_qjs_exception_message_ptr',
    'venom_qjs_exception_message_size',
    'venom_qjs_exception_clear',
    'venom_qjs_upstream_quickjs_ready',
    'venom_qjs_bridge_abi',
    'venom_qjs_bridge_input_alloc',
    'venom_qjs_bridge_input_capacity',
    'venom_qjs_bridge_invoke',
    'venom_qjs_bridge_result_ptr',
    'venom_qjs_bridge_result_size',
    'venom_qjs_bridge_release',
]

if len(sys.argv) != 2:
    raise SystemExit('usage: quickjs_release_abi.py OUT.json')
with open(sys.argv[1], 'w', encoding='utf-8') as f:
    json.dump(['_' + n for n in RELEASE_ABI], f, indent=2)
print(f'[venom] wrote {len(RELEASE_ABI)} release ABI exports to {sys.argv[1]}')
