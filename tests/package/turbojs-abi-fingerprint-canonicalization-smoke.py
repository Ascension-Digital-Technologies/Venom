#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
engine = (root / 'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
worker = (root / 'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')

required = [
    'memory',
    'venom_tjs_context_alloc',
    'venom_tjs_context_free',
    'venom_tjs_context_set_limits',
    'venom_tjs_context_set_execution_limits',
    'venom_tjs_script_buffer_alloc',
    'venom_tjs_script_buffer_capacity',
    'venom_tjs_script_buffer_set_expected_hash',
    'venom_tjs_script_buffer_free',
    'venom_tjs_bytecode_validate',
    'venom_tjs_execute_bytecode',
    'venom_tjs_compact_result_ptr',
    'venom_tjs_diversification_install',
    'venom_tjs_exception_message_ptr',
    'venom_tjs_exception_message_size',
    'venom_tjs_exception_clear',
    'venom_tjs_upstream_turbojs_ready',
    'venom_tjs_bridge_abi',
    'venom_tjs_bridge_input_alloc',
    'venom_tjs_bridge_input_capacity',
    'venom_tjs_bridge_invoke',
    'venom_tjs_bridge_result_ptr',
    'venom_tjs_bridge_result_size',
    'venom_tjs_bridge_release',
]
extras = [
    '__indirect_function_table',
    '_emscripten_stack_restore',
    'emscripten_stack_get_current',
]

prefix = 'VTJSABI12|VRDIV003|VRC3|'
text = prefix + '|'.join(sorted(required))
h = 0x811C9DC5
for ch in text:
    h ^= ord(ch) & 0xFF
    h = (h * 0x01000193) & 0xFFFFFFFF
assert h == 0x33141281, hex(h)

checks = {
    'canonical required export constant': 'RELEASE_ABI_REQUIRED_EXPORTS' in engine,
    'canonical export validation': 'validateReleaseAbiExports(module)' in engine,
    'runtime validates exact canonical exports': 'validateReleaseAbiExports(module);' in engine and 'releaseAbiHash(canonicalNames)' not in engine,
    'toolchain extras excluded from fingerprint': all(name in engine for name in extras),
    'unknown extras rejected': 'unapproved' in engine and 'RELEASE_ABI_APPROVED_TOOLCHAIN_EXPORTS' in engine,
    'export kinds validated': 'kindMismatches' in engine and 'RELEASE_ABI_REQUIRED_KINDS' in engine,
    'no post-instantiate exact count gate': 'release runtime export surface is not exact' not in engine,
    'package fingerprint derived from generated ABI': 'turbojs_wasm_abi::release_abi_fingerprint' in build,
    'canonical prefix owned by generator': '|VRDIV003|VRC3|' in (root / 'tools/generate_turbojs_wasm_abi.py').read_text(encoding='utf-8'),
    'package fingerprint has no stale constant': '0x3b9ace76u' not in build.lower(),
    'engine validates approved toolchain extras': all(name in engine for name in extras),
}

for name, ok in checks.items():
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
raise SystemExit(0 if all(checks.values()) else 1)
