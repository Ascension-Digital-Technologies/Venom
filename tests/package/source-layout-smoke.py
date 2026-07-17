#!/usr/bin/env python3
import sys
from pathlib import Path


def line_count(path: Path) -> int:
    return len(path.read_text(encoding='utf-8').splitlines())


def main() -> int:
    if len(sys.argv) != 2:
        print('usage: source-layout-smoke.py <repo-root>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    expected = {
        'src/compiler/pipeline/build.cpp': 850,
        'src/compiler/pipeline/build_report.cpp': 180,
        'src/compiler/pipeline/build_package_metadata.cpp': 650,
        'src/compiler/pipeline/build_runtime_metadata.cpp': 700,
        'src/compiler/pipeline/build_runtime_audit_metadata.cpp': 700,
        'src/compiler/pipeline/build_runtime_module_metadata.cpp': 450,
        'src/compiler/pipeline/build_support.cpp': 600,
        'src/compiler/pipeline/security.cpp': 250,
        'src/compiler/pipeline/security_analysis.cpp': 800,
        'src/compiler/pipeline/security_artifact_inspection.cpp': 400,
        'src/compiler/pipeline/js.cpp': 500,
        'src/compiler/pipeline/js_bundle_encoding.cpp': 360,
        'src/compiler/pipeline/js_rewriting.cpp': 1000,
        'src/compiler/pipeline/js_planning.cpp': 450,
        'src/compiler/pipeline/js_discovery.cpp': 800,
        'src/generated/runtime/worker_runtime_js.cpp': 500,
        'src/generated/runtime/runtime_js.cpp': 4200,
        'src/generated/runtime/wasm_runtime_js.cpp': 700,
        'src/generated/runtime/quickjs_engine_module.cpp': 1400,
    }
    failures = []
    forbidden_runtime_stubs = (
        'src/runtime/loader.c',
        'src/runtime/loader.h',
        'src/runtime/host_dom.c',
        'src/runtime/host_dom.h',
        'src/runtime/host_fetch.c',
        'src/runtime/host_fetch.h',
        'src/runtime/host_events.c',
        'src/runtime/host_events.h',
        'src/runtime/runtime.c',
        'src/runtime/runtime.h',
        'src/vm/interpreter.cpp',
        'src/vm/interpreter.hpp',
    )
    for rel in forbidden_runtime_stubs:
        if (root / rel).exists():
            failures.append(f'legacy placeholder/dead scaffold file must not return: {rel}')

    forbidden_placeholder_apis = (
        'install_host_bridge_placeholder',
        'compile_placeholder_bytecode',
        'compile_byte_buffer_record',
        'using CryptoProvider =',
        'using NoCryptoProvider =',
    )
    quickjs_text = '\n'.join(
        path.read_text(encoding='utf-8')
        for path in sorted((root / 'src/quickjs').glob('*'))
        if path.suffix in {'.c', '.cpp', '.h', '.hpp'}
    )
    package_text = '\n'.join(
        path.read_text(encoding='utf-8')
        for path in sorted((root / 'src/package').glob('*'))
        if path.suffix in {'.c', '.cpp', '.h', '.hpp'}
    )
    for symbol in forbidden_placeholder_apis:
        search_text = package_text if symbol.startswith('using ') else quickjs_text
        if symbol in search_text:
            failures.append(f'legacy placeholder API must not return: {symbol}')

    runtime_c_sources = sorted((root / 'src/runtime').glob('*.c'))
    for path in runtime_c_sources:
        text = path.read_text(encoding='utf-8')
        if 'void venom_' in text and '(void) {}' in text:
            failures.append(f'empty runtime initializer is not allowed: {path.relative_to(root)}')

    source_templates = sorted((root / 'src').rglob('*.in'))
    if source_templates:
        failures.append('build templates must live outside src/: ' + ', '.join(str(path.relative_to(root)) for path in source_templates))

    compiler_root_headers = sorted((root / 'src/compiler').glob('*.hpp'))
    compiler_root_headers += sorted((root / 'src/compiler').glob('*.h'))
    if compiler_root_headers:
        failures.append('headers must be grouped below src/compiler: ' + ', '.join(path.name for path in compiler_root_headers))

    required_header_dirs = ('commands', 'core', 'pipeline', 'services')
    for directory in required_header_dirs:
        header_dir = root / 'src/compiler' / directory
        if not header_dir.is_dir() or not any(header_dir.glob('*.hpp')):
            failures.append(f'missing organized compiler header directory: {directory}')
    for rel, maximum in expected.items():
        path = root / rel
        if not path.exists():
            failures.append(f'missing {rel}')
            continue
        lines = line_count(path)
        if lines > maximum:
            failures.append(f'{rel} has {lines} lines, expected <= {maximum}')

    cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    for rel in expected:
        if rel == 'src/generated/runtime/quickjs_engine_module.cpp':
            if 'VENOM_QUICKJS_ENGINE_CPP' not in cmake or 'generate_quickjs_engine_module.py' not in cmake:
                failures.append('CMakeLists.txt does not generate the QuickJS engine module from authored runtime modules')
            continue
        if rel not in cmake:
            failures.append(f'CMakeLists.txt does not include {rel}')
    if 'src/runtime/js/browser/*.js' not in cmake or 'src/runtime/js/quickjs-engine/*.js' not in cmake:
        failures.append('CMakeLists.txt does not register authored JavaScript runtime modules')

    # Production source packages intentionally omit contributor-only source-layout documentation.

    if failures:
        for failure in failures:
            print('failure:', failure)
        return 1
    print('source layout smoke passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
