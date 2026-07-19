from pathlib import Path
root = Path(__file__).resolve().parents[2]
build = (root/'src/pipeline/build.cpp').read_text() + (root/'src/pipeline/build_package_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_audit_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_module_metadata.cpp').read_text()
runtime = (root/'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
doc = (root/'docs/capability-secure-browser-bridge.md').read_text()
for needle in ['VENOM_HOST_BRIDGE_V14', 'capability_model=application-specialized-schema-v1', 'unknown_host_call=deny', 'application_specialized=true', 'fetch_enabled=', 'timers_enabled=', 'events_enabled=']:
    assert needle in build, needle
for needle in ['authorizeHostCapability', 'duplicate host capability', 'host capability denied', 'capabilitiesById', 'strictCapabilities', 'invalid DOM handle authentication tag', 'crypto.getRandomValues']:
    assert needle in runtime, needle
for needle in ['application-specialized', 'deny-by-default', 'authenticated DOM handles', 'per-call byte limits']:
    assert needle in doc, needle
print('capability-secure-browser-bridge smoke: ok')
