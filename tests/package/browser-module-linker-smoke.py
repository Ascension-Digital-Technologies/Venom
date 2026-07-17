#!/usr/bin/env python3
from __future__ import annotations
import subprocess
import tempfile
from pathlib import Path

repo = Path(__file__).resolve().parents[2]
runtime = repo / "src" / "runtime" / "templates" / "runtime.js"
source = runtime.read_text(encoding="utf-8")
start = source.find("function normalizeBrowserModulePath")
end = source.find("async function executeBrowserScriptChunk", start)
if start < 0 or end < 0:
    raise SystemExit("browser module linker implementation is missing")
linker = source[start:end]

program = f"""
'use strict';
const SCRIPT_FLAG = {{ MODULE: 1 << 1, DEPENDENCY: 1 << 8, BROWSER: 1 << 11 }};
function safeSourceUrl(value) {{ return String(value || '').replace(/[^a-zA-Z0-9_./#:-]/g, '_'); }}
const textDecoder = new TextDecoder();
const emitted = new Map();
let sequence = 0;
class TestBlob {{ constructor(parts) {{ this.source = parts.map(String).join(''); }} }}
const Blob = TestBlob;
const URL = {{
  createObjectURL(blob) {{ const url = 'blob:venom-test/' + (++sequence); emitted.set(url, blob.source); return url; }},
  revokeObjectURL(url) {{ emitted.delete(url); }}
}};
{linker}
const flags = SCRIPT_FLAG.BROWSER | SCRIPT_FLAG.MODULE;
const chunks = [
  {{ route: '/', source: 'browser/app.ts', flags, code: 'import {{ calculateQuote }} from "../protected/pricing";\\nimport formatMoney from "./format";\\nglobalThis.answer=[calculateQuote,formatMoney];' }},
  {{ route: '/protected-facade', source: 'protected/pricing.ts', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: 'export async function calculateQuote(input){{return input;}}' }},
  {{ route: '/', source: 'browser/format.ts', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: 'export default function formatMoney(value){{return String(value);}}' }}
];
const graph = createBrowserModuleLinker(chunks, '/');
const entryUrl = graph.urlFor(chunks[0]);
const entry = emitted.get(entryUrl) || '';
if (entry.includes('../protected/pricing') || entry.includes('./format')) throw new Error('relative browser module specifier survived linking');
if (!/blob:venom-test\\/\\d+/.test(entry)) throw new Error('linked browser module URLs were not emitted');
if (emitted.size !== 3) throw new Error('expected three linked browser modules, got ' + emitted.size);
graph.dispose();
if (emitted.size !== 0) throw new Error('linked browser module URLs were not revoked');
const opaqueChunks = [
  {{ route: '/', source: 's_entry', flags, code: `/*@venom-module-map-v1\n2e2e2f70726f7465637465642f70726963696e67\t735f70726963696e67\n2e2f666f726d6174\t735f666f726d6174\n*/\nimport {{ calculateQuote }} from "../protected/pricing";\nimport formatMoney from "./format";` }},
  {{ route: '/protected-facade', source: 's_pricing', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: 'export async function calculateQuote(input){{return input;}}' }},
  {{ route: '/', source: 's_format', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: 'export default function formatMoney(value){{return String(value);}}' }}
];
const opaqueGraph = createBrowserModuleLinker(opaqueChunks, '/');
const opaqueEntry = emitted.get(opaqueGraph.urlFor(opaqueChunks[0])) || '';
if (opaqueEntry.includes('../protected/pricing') || opaqueEntry.includes('./format')) throw new Error('opaque production module specifier survived linking');
opaqueGraph.dispose();
if (emitted.size !== 0) throw new Error('opaque linked browser module URLs were not revoked');
const generatedFacadeChunks = [
  {{ route: '/', source: 's_generated_entry', flags, code: `/*@venom-module-map-v1
2e2e2f70726f7465637465642f70726963696e67\t735f67656e6572617465645f70726963696e67
*/
import {{ calculateQuote }} from "../protected/pricing";` }},
  {{ route: '/protected-facade', source: 's_generated_pricing', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: 'export async function calculateQuote(input){{return input;}}' }}
];
const generatedFacadeGraph = createBrowserModuleLinker(generatedFacadeChunks, '/');
const generatedFacadeEntry = emitted.get(generatedFacadeGraph.urlFor(generatedFacadeChunks[0])) || '';
if (generatedFacadeEntry.includes('../protected/pricing')) throw new Error('generated protected facade did not link');
generatedFacadeGraph.dispose();
const remappedChunks = [
  {{ route: '/', source: 's_entry_runtime_alias', flags, code: `/* generated prefix */\nimport {{ calculateQuote }} from "../protected/pricing";` }},
  {{ route: '/', source: 's_map_owner', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: `/* prefix before map */\n/*@venom-module-map-v1\n2e2e2f70726f7465637465642f70726963696e67\t735f70726963696e67\n*/\nexport {{}};` }},
  {{ route: '/', source: 's_pricing', flags: flags | SCRIPT_FLAG.DEPENDENCY, code: 'export async function calculateQuote(input){{return input;}}' }}
];
const remappedGraph = createBrowserModuleLinker(remappedChunks, '/');
const remappedEntry = emitted.get(remappedGraph.urlFor(remappedChunks[0])) || '';
if (remappedEntry.includes('../protected/pricing')) throw new Error('unique opaque map fallback did not link dependency');
remappedGraph.dispose();
if (emitted.size !== 0) throw new Error('remapped linked browser module URLs were not revoked');

const identityFlags = 2 | 256 | 2048;
const identityChunks = [
  {{ route: '/', source: 's_entry_identity', flags: identityFlags, code: 'import {{ calculateQuote }} from "../protected/pricing";' }},
  {{ route: '/generated', source: 's_final_facade_identity', flags: identityFlags, code: '/*@venom-module-source-v1\\n70726f7465637465642f70726963696e672e7473\\n*/\\nexport async function calculateQuote(input){{ return input; }}' }}
];
const identityLinker = createBrowserModuleLinker(identityChunks, '/');
const identityUrl = identityLinker.urlFor(identityChunks[0]);
const identityEntry = emitted.get(identityUrl) || '';
if (identityEntry.includes('../protected/pricing')) throw new Error('authored module identity fallback did not link dependency');
identityLinker.dispose();
if (emitted.size !== 0) throw new Error('identity linked browser module URLs were not revoked');

console.log('browser module linker smoke: PASS');
"""
with tempfile.TemporaryDirectory() as tmp:
    script = Path(tmp) / "test.js"
    script.write_text(program, encoding="utf-8")
    run = subprocess.run(["node", str(script)], text=True, capture_output=True)
    if run.returncode != 0:
        raise SystemExit(run.stdout + run.stderr)
    print(run.stdout.strip())
