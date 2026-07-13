#!/usr/bin/env python3
import shutil
import subprocess
import sys
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
site = Path(sys.argv[2]).resolve()
out = Path(sys.argv[3]).resolve()
node = sys.argv[4] if len(sys.argv) > 4 else "node"

if out.exists():
    shutil.rmtree(out)

dist = out / "production"
subprocess.run([str(venom), "build", str(site), "--out", str(dist)], check=True)
subprocess.run([str(venom), "verify-runtime", str(dist), "--target", "browser", "--require-real-engine"], check=True)

script = r'''
import { readFile, readdir } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { resolve, join, dirname } from 'node:path';

const distDir = resolve(process.argv[1]);
class ElementNode {
  constructor(tagName) { this.tagName = tagName; this.children = []; this.attributes = new Map(); this.listeners = new Map(); this.parentNode = null; }
  appendChild(node) { if (node) { node.parentNode = this; this.children.push(node); } return node; }
  removeChild(node) { const index = this.children.indexOf(node); if (index >= 0) this.children.splice(index, 1); if (node) node.parentNode = null; return node; }
  replaceChildren(...nodes) { this.children = []; for (const n of nodes) this.appendChild(n); }
  setAttribute(name, value) { this.attributes.set(String(name), String(value)); }
  getAttribute(name) { return this.attributes.has(String(name)) ? this.attributes.get(String(name)) : null; }
  removeAttribute(name) { this.attributes.delete(String(name)); }
  addEventListener(name, handler) { const list = this.listeners.get(name) || []; list.push(handler); this.listeners.set(name, list); }
  dispatchEvent(event) { for (const handler of this.listeners.get(event && event.type ? event.type : '') || []) handler.call(this, event); return true; }
  closest() { return null; }
  toText() { return this.children.map((child) => child.toText()).join(''); }
}
class TextNode { constructor(text) { this.text = text; } toText() { return this.text; } }

globalThis.Element = ElementNode;
const documentElement = new ElementNode('html');
const head = new ElementNode('head');
const body = new ElementNode('body');
documentElement.appendChild(head);
documentElement.appendChild(body);
globalThis.document = {
  documentElement, head, body, baseURI: 'file:///',
  createElement: (tag) => new ElementNode(tag),
  createTextNode: (text) => new TextNode(text),
  addEventListener: () => {},
};
Object.defineProperty(globalThis, 'navigator', { configurable: true, value: {
  sendBeacon() { throw new TypeError("native sendBeacon should not receive zero args"); },
  getUserMedia() { throw new TypeError("native getUserMedia should not receive zero args"); },
  webkitGetUserMedia() { throw new TypeError("native webkitGetUserMedia should not receive zero args"); },
  mozGetUserMedia() { throw new TypeError("native mozGetUserMedia should not receive zero args"); },
  msGetUserMedia() { throw new TypeError("native msGetUserMedia should not receive zero args"); },
  mediaDevices: { getUserMedia() { throw new TypeError("native mediaDevices.getUserMedia should not receive zero args"); } }
} });
globalThis.window = globalThis;
globalThis.window.addEventListener = () => {};
globalThis.window.removeEventListener = () => {};
globalThis.self = globalThis;
globalThis.history = { pushState: () => {} };
globalThis.location = { pathname: '/' };

async function listAssetFiles(relative = '') {
  const base = join(distDir, 'assets', relative);
  const entries = await readdir(base, { withFileTypes: true });
  const files = [];
  for (const entry of entries) {
    const child = relative ? `${relative}/${entry.name}` : entry.name;
    if (entry.isDirectory()) files.push(...await listAssetFiles(child));
    else files.push(child);
  }
  return files;
}
const assetFiles = await listAssetFiles();
const loaderFile = assetFiles.find((name) => /(?:^|\/)loader(?:\.[a-f0-9]+)?\.js$/.test(name));
const runtimeFile = assetFiles.find((name) => /(?:^|\/)r(?:\.[a-f0-9]+)?\.js$/.test(name));
const packageFile = assetFiles.find((name) => /(?:^|\/)app(?:\.[a-f0-9]+)?\.vbc$/.test(name));
const quickJsEngineFile = assetFiles.find((name) => /(?:^|\/)engine(?:\.[a-f0-9]+)?\.js$/.test(name));
const quickJsWasmFile = assetFiles.find((name) => /(?:^|\/)runtime(?:\.[a-f0-9]+)?\.wasm$/.test(name));
if (!loaderFile || !runtimeFile || !packageFile || !quickJsEngineFile || !quickJsWasmFile) throw new Error(`missing production assets: ${assetFiles.join(', ')}`);
const loaderText = await readFile(join(distDir, 'assets', ...loaderFile.split('/')), 'utf8');
const bindingToken = (loaderText.match(/bindingToken:\s*'([^']+)'/) || [])[1] || '';
const runtimeAssetDirectory = join(distDir, 'assets', dirname(runtimeFile));
globalThis.fetch = async (url) => {
  const raw = String(url);
  let filePath;
  if (raw.startsWith('file://')) filePath = new URL(raw);
  else if (/^(?:\.\.\/|\.\/)/.test(raw)) filePath = resolve(runtimeAssetDirectory, raw);
  else filePath = join(distDir, 'assets', raw.replace(/^assets\//, '').replace(/^\.\//, ''));
  const bytes = await readFile(filePath);
  return { ok: true, status: 200, arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength), text: async () => Buffer.from(bytes).toString('utf8') };
};

const root = new ElementNode('div');
const runtime = await import(pathToFileURL(join(distDir, 'assets', ...runtimeFile.split('/'))));
const assetBaseUrl = pathToFileURL(join(distDir, 'assets') + '/').href;
const result = await runtime.bootVenom({
  root,
  packageUrl: '../' + packageFile,
  assetBaseUrl,
  quickJsEngineUrl: pathToFileURL(join(distDir, 'assets', ...quickJsEngineFile.split('/'))).href,
  quickJsWasmUrl: pathToFileURL(join(distDir, 'assets', ...quickJsWasmFile.split('/'))).href,
  hardened: true,
  bindingToken,
});

if (result.route !== '/') throw new Error(`expected root route, got ${result.route}`);
// Production execution must boot through the QuickJS/WASM boundary without host source-eval fallback.
// Browser API effect replay is covered by the browser-compat fixture; this smoke keeps the media/beacon fixture in the production package gate.
if (!globalThis.__venomRuntime || globalThis.__venomRuntime.packageVersion !== 40) throw new Error('Venom host bridge missing after browser API shim smoke');
const execution = globalThis.__venomRuntime.quickJsExecutionSnapshot();
if (!execution) throw new Error('QuickJS execution snapshot missing after browser API shim smoke');
'''
subprocess.run([node, "--input-type=module", "-e", script, str(dist)], check=True)

package = next((dist / "assets" / "app").glob("app.*.vbc"))
raw = package.read_bytes()
for needle in [b"browser host api shim script loaded", b"native sendBeacon should not receive zero args", b"native getUserMedia should not receive zero args", b"native mediaDevices.getUserMedia should not receive zero args", b"navigator.sendBeacon.arity", b"navigator.getUserMedia.arity", b"navigator.mediaDevices.getUserMedia.arity", b"global.fpCollect.missing", b"__venomMissingGlobalShim"]:
    if needle in raw:
        raise SystemExit(f"raw protected package leaked browser API shim marker: {needle!r}")
print("browser API shim smoke passed")
