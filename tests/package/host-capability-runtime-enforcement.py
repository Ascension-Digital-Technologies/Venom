from pathlib import Path
root = Path(__file__).resolve().parents[2]
runtime = (root/'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
required = [
    'function capabilitySetForChunk(chunk)',
    'function makeScopedRuntimeBridge(bridge, allowed, chunk)',
    "VNM-CAP-1001: undeclared host capability",
    "if (permits('document')) bindings.document",
    "if (permits('fetch')) bindings.fetch",
    "if (permits('timers')) {",
    "if (permits('events')) for (const name of ['Event', 'CustomEvent'])",
    "bindings.__venomRuntime = makeScopedRuntimeBridge(bridge, allowed, chunk)",
    'const capabilityNames = allowed ? Array.from(allowed)',
]
for needle in required:
    assert needle in runtime, needle
for forbidden in [
    'bindings.__venomRuntime = bridge;',
    'const capabilityNames = hostCapabilitiesPlan.capabilities && hostCapabilitiesPlan.capabilities.length',
]:
    assert forbidden not in runtime, forbidden
print('host capability runtime enforcement: ok')
