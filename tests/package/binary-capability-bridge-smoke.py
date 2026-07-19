from pathlib import Path

root = Path(__file__).resolve().parents[2]
worker = ((root / "src/generated/runtime/worker_runtime_js.cpp").read_text(encoding="utf-8") + (root / "include/venom/generated/runtime/worker_runtime_template.hpp").read_text(encoding="utf-8"))
loader = (root / "src/pipeline/js.cpp").read_text(encoding="utf-8")
planning = (root / "src/pipeline/js_planning.cpp").read_text(encoding="utf-8")

for marker in [
    "BRIDGE_CAPABILITIES", "BRIDGE_MAGIC", "BRIDGE_HEADER_BYTES",
    "keyedFrameTag", "encodeBridgeFrame", "decodeBridgeFrame",
    "bridgeGeneration", "bridgeKey", "MessageChannel", "event.ports[0]",
    "stale or replayed bridge request", "unknown-capability",
]:
    assert marker in worker or marker in loader, marker

for marker in [
    "binary-capability-v3-leased", "encodeFrame", "decodeFrame",
    "bridgePort.postMessage(frame,[frame])", "leaseCapabilityForSlot",
    "worker.postMessage({ protocol: 1, type: 'prepare'", "[bridgeChannel.port2]",
]:
    assert marker in loader, marker

assert 'schema_version\\": 2' in planning
assert 'binary-capability-v3-leased' in planning
assert "worker.postMessage([1,__venomInvokeOp" not in loader
assert "postMessage([WORKER_PROTOCOL, BRIDGE_RESULT_OP" not in worker
print("binary capability bridge smoke: PASS")
