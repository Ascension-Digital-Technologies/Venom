from pathlib import Path

root = Path(__file__).resolve().parents[2]
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
bytecode = (root / 'src/quickjs/bytecode.cpp').read_text(encoding='utf-8')
quickjs_h = (root / 'third_party/quickjs/quickjs.h').read_text(encoding='utf-8')
quickjs_c = (root / 'third_party/quickjs/quickjs.c').read_text(encoding='utf-8')

required_build = [
    'redact_metadata ? 0u : static_cast<std::uint32_t>(chunk.code.size())',
    'redact_metadata ? 0u : venom::package::fnv1a64(source_bytes)',
    'protected_source_label(profile, chunk)',
    'protected_source_size(profile, chunk)',
    'protected_source_hash(profile, chunk)',
]
for token in required_build:
    if token not in build:
        raise SystemExit(f'missing release metadata redaction gate: {token}')

required_bytecode = [
    'append_u64(out, bytecode_hash)',
    'append_u64(out, source_hash)',
    'append_u32(out, static_cast<std::uint32_t>(source.size()))',
    'JS_RewriteModuleRequestNames(context, object',
    'JS_RewriteModuleRequestNames(ctx, object',
]
for token in required_bytecode:
    if token not in bytecode:
        raise SystemExit(f'missing native bytecode hardening behavior: {token}')

for text, token in ((quickjs_h, 'JS_RewriteModuleRequestNames'),
                    (quickjs_c, 'int JS_RewriteModuleRequestNames(')):
    if token not in text:
        raise SystemExit(f'missing QuickJS module request rewrite API: {token}')

print('release bytecode and module metadata redaction smoke: PASS')
