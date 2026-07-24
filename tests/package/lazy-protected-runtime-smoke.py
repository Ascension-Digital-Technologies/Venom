from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
worker=(root/'src/generated/runtime/worker_runtime_template.hpp').read_text(encoding='utf-8')
build=(root/'src/pipeline/build.cpp').read_text(encoding='utf-8')
checks=[
 ('candidate activation','ensureLazyRegistry(candidate)' in worker),
 ('sha256 attestation','protected registry chunk digest mismatch' in worker),
 ('hashed external assets','named_output("", ".vqc", chunk.bytecode, options.hashed_assets)' in build),
 ('package exclusion','external_lazy_registry ? std::vector<unsigned char>{} : js_bridge.bridge_registry_bytecode' in build),
 ('selective preload','preloadLazyRegistries' in worker and 'filter((chunk) => chunk.preload)' in worker),
 ('ir v13','compiler-v13' in build),
 ('tjs runtime compatibility','prepareTurboJsRecordForExecution' in worker),
]
failed=[name for name,ok in checks if not ok]
if failed: raise SystemExit('lazy protected runtime smoke failed: '+', '.join(failed))
print('lazy protected runtime smoke: PASS')
