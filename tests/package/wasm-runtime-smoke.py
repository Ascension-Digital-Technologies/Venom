#!/usr/bin/env python3
import shutil
import subprocess
import sys
from pathlib import Path

venom = Path(sys.argv[1])
site = Path(sys.argv[2])
out = Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'protect', '--runtime', 'wasm', '--hashed'], check=True)
assets = out / 'assets'
wasm_files = sorted(assets.glob('runtime*.wasm'))
bridge_files = sorted(assets.glob('runtime-js-bridge*.js'))
loader_files = sorted(assets.glob('loader*.js'))
package_files = sorted(assets.glob('app*.vbc'))
quickjs_engine_files = sorted(assets.glob('quickjs-engine*.js'))
if len(quickjs_engine_files) != 1: raise SystemExit('expected exactly one quickjs-engine asset')
if len(wasm_files) != 1: raise SystemExit('expected exactly one runtime wasm asset')
if len(bridge_files) != 1: raise SystemExit('expected exactly one runtime-js-bridge asset')
if len(loader_files) != 1 or len(package_files) != 1: raise SystemExit('expected exactly one loader and app package')
wasm = wasm_files[0].read_bytes()
if not wasm.startswith(b'\x00asm\x01\x00\x00\x00'): raise SystemExit('runtime wasm asset has invalid magic/version')
if b'venom_wasm_analyze' not in wasm: raise SystemExit('runtime wasm asset is missing execution export')
if b'venom_wasm_domops_size' not in wasm: raise SystemExit('runtime wasm asset is missing binary DOM-op export')
bridge = bridge_files[0].read_text(encoding='utf-8')
if wasm_files[0].name not in bridge: raise SystemExit('WASM bridge does not reference generated wasm filename')
if 'runVenomWasmExecution' not in bridge or 'venom_wasm_analyze' not in bridge: raise SystemExit('WASM bridge does not call execution path')
if 'decodeWasmDomOpStream' not in bridge or 'VDOP0020' not in bridge or 'venom_wasm_domops_size' not in bridge: raise SystemExit('WASM bridge is missing binary DOM-op decoder support')
engine_module = quickjs_engine_files[0].read_text(encoding='utf-8')
if 'createVenomQuickJsEngineModule' not in engine_module or 'executeChunk' not in engine_module or 'executionSnapshot' not in engine_module or 'fallbackPolicy' not in engine_module: raise SystemExit('QuickJS engine module is missing execution exports')
if 'bytecodeManifest' not in engine_module or 'moduleGraph' not in engine_module or 'moduleCacheSnapshot' not in engine_module or 'resolverAudit' not in engine_module or 'interopFallback' not in engine_module or 'moduleResolver' not in engine_module or 'exceptionAbi' not in engine_module or 'hostTrapPolicy' not in engine_module or 'executionLifecycle' not in engine_module or 'scriptBufferPolicy' not in engine_module or 'contextLimitPolicy' not in engine_module or 'hostCallDispatch' not in engine_module or 'parityContract' not in engine_module or 'releaseFailClosed' not in engine_module or 'scriptRecord' not in engine_module or 'evalResult' not in engine_module or 'consoleCapture' not in engine_module or 'failureReport' not in engine_module or 'snapshotPolicy' not in engine_module or 'snapshotRecord' not in engine_module or 'replayValidation' not in engine_module or 'determinismLedger' not in engine_module or 'auditSeal' not in engine_module or 'executionCommit' not in engine_module or 'rollbackPolicy' not in engine_module or 'hostCallReceipts' not in engine_module or 'releaseAcceptance' not in engine_module or 'commitAudit' not in engine_module or 'capabilityPolicy' not in engine_module or 'hostIoPolicy' not in engine_module or 'permissionSeal' not in engine_module or 'policyReceipts' not in engine_module or 'releaseGate' not in engine_module or 'hostIoDecision' not in engine_module or 'hostIoDenyTrace' not in engine_module or 'capabilityLedger' not in engine_module or 'policySealAudit' not in engine_module or 'runtimeDenylist' not in engine_module: raise SystemExit('QuickJS engine module is missing v0.37 host I/O enforcement exports')
if 'parseQuickJsEngineModuleMetadata' not in bridge or 'quickJsEngineUrl' not in bridge: raise SystemExit('runtime bridge is missing QuickJS engine module loader support')
if 'bindEvent' not in bridge or 'parseHostBridgeMetadata' not in bridge or 'parseFetchBridgeMetadata' not in bridge or 'parseTimerBridgeMetadata' not in bridge or 'parseEventQueueMetadata' not in bridge or 'parseQuickJsBridgeMetadata' not in bridge or 'parseScriptIsolationMetadata' not in bridge or 'parseScriptPolicyMetadata' not in bridge or 'parseQuickJsChunkMetadata' not in bridge or 'createAsyncHostQueue' not in bridge: raise SystemExit('WASM bridge is missing host-call/fetch/async/script bridge support')
loader = loader_files[0].read_text(encoding='utf-8')
if bridge_files[0].name not in loader: raise SystemExit('loader does not import generated runtime-js-bridge filename')
if quickjs_engine_files[0].name not in loader: raise SystemExit('loader does not pass generated QuickJS engine filename')
inspect = subprocess.run([str(venom), 'inspect', str(package_files[0])], check=True, text=True, stdout=subprocess.PIPE)
text = inspect.stdout
required = [
    'version: 40',
    'wasm-runtime',
    'host-bridge',
    'binary-dom-ops',
    'fetch-bridge',
    'async-host-queue',
    'timer-bridge',
    'event-queue',
    'quickjs-bridge',
    'script-isolation',
    'script-policy',
    'quickjs-chunks',
    'quickjs-engine-module',
    'quickjs-context-lifecycle',
    'host-capabilities',
    'quickjs-adapter-diagnostics',
    'host_bridge name="s.',
    'fetch_bridge name="s.',
    'timer_bridge name="s.',
    'event_queue name="s.',
    'quickjs_bridge name="s.',
    'script_isolation name="s.',
    'script_policy name="s.',
    'quickjs_chunks name="s.',
    'quickjs_engine_module name="s.',
    'quickjs_context_lifecycle name="s.',
    'host_capabilities name="s.',
    'quickjs_adapter_diagnostics name="s.',
    'async_host_queue name="s.',
    'quickjs_execution_records name="s.',
    'quickjs_result_bridge name="s.',
    'quickjs_fallback_policy name="s.',
    'quickjs_engine_backend name="s.',
    'quickjs_runtime_denylist name="s.',
]
for forbidden in (
    'runtime-policy.vhrd', 'host-calls.vhcb', 'fetch-bridge.vfch', 'timer-bridge.vtmr',
    'event-queue.vevq', 'quickjs-bridge.vqjs', 'script-isolation.vsis', 'script-policy.vscp',
    'quickjs-chunks.vqbc', 'quickjs-engine-module.vqem', 'quickjs-runtime-denylist.vqrd'
):
    if forbidden in text:
        raise SystemExit('protect wasm inspect leaked canonical internal name: ' + forbidden)
missing = [item for item in required if item not in text]
if missing: raise SystemExit('missing expected wasm inspect output: ' + ', '.join(missing))
print('wasm runtime smoke passed')
