from pathlib import Path
root = Path(__file__).resolve().parents[2]
checks = {
    root/'src/cli/cli.hpp': ['lazy_loading_enabled', 'lazy_preload'],
    root/'src/core/config.cpp': ['runtime.lazy_loading.enabled', 'runtime.lazy_loading.preload'],
    root/'src/pipeline/js.cpp': ['candidate-chunk-activation', 'lazy_protected_manifest_json'],
    root/'src/pipeline/build.cpp': ['lazy-protected-manifest.vlpm', 'compiler-v12'],
    root/'src/pipeline/project_ir.hpp': ['kProjectIrVersion = 12'],
}
for path, needles in checks.items():
    text = path.read_text(encoding='utf-8')
    for needle in needles:
        assert needle in text, f'{needle} missing from {path}'
print('lazy protected manifest smoke: PASS')
