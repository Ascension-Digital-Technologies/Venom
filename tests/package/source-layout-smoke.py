#!/usr/bin/env python3
import sys
from pathlib import Path

NATIVE_SUFFIXES = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx'}


def line_count(path: Path) -> int:
    return len(path.read_text(encoding='utf-8').splitlines())


def main() -> int:
    if len(sys.argv) != 2:
        print('usage: source-layout-smoke.py <repo-root>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    failures: list[str] = []

    expected_limits = {
        'src/pipeline/build.cpp': 850,
        'src/pipeline/build_report.cpp': 180,
        'src/pipeline/build_package_metadata.cpp': 650,
        'src/pipeline/build_runtime_metadata.cpp': 700,
        'src/pipeline/build_runtime_audit_metadata.cpp': 700,
        'src/pipeline/build_runtime_module_metadata.cpp': 450,
        'src/pipeline/build_support.cpp': 600,
        'src/pipeline/security.cpp': 250,
        'src/pipeline/security_analysis.cpp': 800,
        'src/pipeline/security_artifact_inspection.cpp': 400,
        'src/pipeline/js.cpp': 500,
        'src/pipeline/js_bundle_encoding.cpp': 360,
        'src/pipeline/js_rewriting.cpp': 1000,
        'src/pipeline/js_planning.cpp': 450,
        'src/pipeline/js_discovery.cpp': 800,
        'src/generated/runtime/worker_runtime_js.cpp': 500,
        'src/generated/runtime/runtime_js.cpp': 4200,
        'src/generated/runtime/wasm_runtime_js.cpp': 700,
        'src/generated/runtime/quickjs_engine_module.cpp': 1400,
    }

    expected_src_roots = {'base', 'cli', 'core', 'frontends', 'generated', 'graph', 'package', 'pipeline', 'quickjs', 'remote', 'runtime', 'templates', 'vm'}
    actual_src_roots = {p.name for p in (root / 'src').iterdir() if p.is_dir()}
    if actual_src_roots != expected_src_roots:
        failures.append(f'src top-level ownership mismatch: expected {sorted(expected_src_roots)}, got {sorted(actual_src_roots)}')

    forbidden_paths = (
        'src/engine',
        'src/compiler',
        'src/compiler/commands',
        'src/compiler/frontends',
        'src/compiler/pipeline',
        'src/compiler/templates',
        'src/runtime/native',
        'src/runtime/templates',
        'src/runtime/js',
        'src/templates/runtime.js',
        'src/compiler/package',
        'integrations',
        'fuzz',
    )
    for rel in forbidden_paths:
        if (root / rel).exists():
            failures.append(f'legacy source layout path must not return: {rel}')

    allowed_non_native_prefixes = (
        'src/templates/',
        'src/generated/runtime/javascript/',
        'src/generated/runtime/metadata/',
    )
    for path in sorted((root / 'src').rglob('*')):
        if not path.is_file() or path.suffix.lower() in NATIVE_SUFFIXES or path.suffix.lower() == '.md':
            continue
        rel = path.relative_to(root).as_posix()
        if not rel.startswith(allowed_non_native_prefixes):
            failures.append(f'non-native source is outside an owned template/generated folder: {rel}')

    for path in sorted((root / 'src/runtime').iterdir()):
        if path.is_file() and path.suffix.lower() not in NATIVE_SUFFIXES:
            failures.append(f'runtime domain contains non-native artifact: {path.relative_to(root)}')

    template_root = root / 'src/templates'
    for path in template_root.rglob('*'):
        if path.is_file() and path.suffix.lower() not in {'.js'}:
            failures.append(f'authored template domain contains unexpected file type: {path.relative_to(root)}')

    quickjs_text = '\n'.join(
        path.read_text(encoding='utf-8')
        for path in sorted((root / 'src/quickjs').glob('*'))
        if path.suffix in NATIVE_SUFFIXES
    )
    package_text = '\n'.join(
        path.read_text(encoding='utf-8')
        for path in sorted((root / 'src/package').glob('*'))
        if path.suffix in NATIVE_SUFFIXES
    )
    for symbol in (
        'install_host_bridge_placeholder',
        'compile_placeholder_bytecode',
        'compile_byte_buffer_record',
        'using CryptoProvider =',
        'using NoCryptoProvider =',
    ):
        search_text = package_text if symbol.startswith('using ') else quickjs_text
        if symbol in search_text:
            failures.append(f'legacy placeholder API must not return: {symbol}')

    for path in sorted((root / 'src/runtime').glob('*.c')):
        text = path.read_text(encoding='utf-8')
        if 'void venom_' in text and '(void) {}' in text:
            failures.append(f'empty runtime initializer is not allowed: {path.relative_to(root)}')

    required_ownership_directories = (
        'src/frontends/javascript',
        'src/frontends/jsx',
        'src/frontends/typescript',
        'src/pipeline/planning',
        'src/generated/compiler',
        'src/generated/include/venom/generated/contracts',
        'src/generated/runtime',
        'src/base/include/venom/base',
        'src/core/include/venom/core',
        'src/frontends/include/venom/frontends',
        'src/graph/include/venom/graph',
        'src/package/include/venom/package',
        'src/pipeline/include/venom/pipeline',
        'src/quickjs/include/venom/quickjs',
        'src/remote/include/venom/remote',
        'src/runtime/include/venom/runtime',
        'src/vm/include/venom/vm',
    )
    for rel in required_ownership_directories:
        if not (root / rel).is_dir():
            failures.append(f'missing source ownership directory: {rel}')

    for rel, maximum in expected_limits.items():
        path = root / rel
        if not path.exists():
            failures.append(f'missing {rel}')
            continue
        lines = line_count(path)
        if lines > maximum:
            failures.append(f'{rel} has {lines} lines, expected <= {maximum}')

    cmake = '\n'.join(path.read_text(encoding='utf-8') for path in [root / 'CMakeLists.txt', *sorted((root / 'cmake').glob('*.cmake'))])
    for rel in expected_limits:
        if rel == 'src/generated/runtime/quickjs_engine_module.cpp':
            if 'VENOM_QUICKJS_ENGINE_CPP' not in cmake or 'tools/generators/runtime/generate_quickjs_engine_module.py' not in cmake:
                failures.append('CMake does not generate the QuickJS engine module from authored templates')
            continue
        if rel not in cmake:
            failures.append(f'CMakeLists.txt does not include {rel}')

    required_cmake_markers = (
        'src/templates/browser/*.js',
        'src/templates/quickjs-engine/*.js',
        'src/quickjs/bytecode.cpp',
        'src/vm/encoder.cpp',
        'src/runtime/package_runtime.c',
        'tests/fuzz/targets/package_parser_fuzz.cpp',
        'venom_add_source_domain(venom_base',
        'venom_add_source_domain(venom_pipeline',
        'venom_add_source_domain(venom_frontends',
        'venom_add_source_domain(venom_cli_commands',
    )
    for marker in required_cmake_markers:
        if marker not in cmake:
            failures.append(f'CMakeLists.txt missing structural path: {marker}')

    if failures:
        for failure in failures:
            print('failure:', failure)
        return 1
    print('source layout smoke passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
