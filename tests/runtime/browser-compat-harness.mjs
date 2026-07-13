import { readFile, readdir } from 'node:fs/promises';
import { webcrypto } from 'node:crypto';
import { pathToFileURL } from 'node:url';
import { resolve, join, dirname, relative } from 'node:path';

const distDir = resolve(process.argv[2] || 'dist');
let activeDocumentShim = null;
const mode = process.argv[3] || 'compat';
const strictNoSourceEval = process.argv.includes('--strict-no-source-eval');
const denyFetch = process.argv.includes('--deny-fetch');
const viaLoader = process.argv.includes('--via-loader');
const requireRemoteScript = process.argv.includes('--require-remote-script');
const assertHtmlFidelity = process.argv.includes('--assert-html-fidelity');

function escapeHtml(value) {
  return String(value ?? '').replace(/[&<>"]/g, (ch) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[ch]);
}

class ClassList {
  constructor(owner) { this.owner = owner; this.items = new Set(); }
  add(...names) { for (const name of names) if (name) this.items.add(String(name)); this._sync(); }
  remove(...names) { for (const name of names) this.items.delete(String(name)); this._sync(); }
  contains(name) { return this.items.has(String(name)); }
  toggle(name, force) {
    const key = String(name);
    const shouldAdd = force === undefined ? !this.items.has(key) : !!force;
    if (shouldAdd) this.items.add(key); else this.items.delete(key);
    this._sync();
    return shouldAdd;
  }
  setFromString(value) { this.items = new Set(String(value || '').split(/\s+/).filter(Boolean)); }
  toString() { return Array.from(this.items).join(' '); }
  _sync() { this.owner.attributes.set('class', this.toString()); }
}

class StyleDeclaration {
  constructor(owner) { this.owner = owner; }
  setProperty(name, value) { this[String(name)] = String(value); this._sync(); }
  getPropertyValue(name) { return this[String(name)] || ''; }
  removeProperty(name) { const prev = this[String(name)] || ''; delete this[String(name)]; this._sync(); return prev; }
  _sync() {
    const entries = Object.entries(this).filter(([key, value]) => key !== 'owner' && typeof value !== 'function' && value !== undefined && value !== '');
    if (entries.length) this.owner.attributes.set('style', entries.map(([key, value]) => `${key}: ${value}`).join('; '));
    else this.owner.attributes.delete('style');
  }
}

class ElementNode {
  constructor(tagName) {
    this.tagName = String(tagName || '').toLowerCase();
    this.nodeType = 1;
    this.children = [];
    this.parentNode = null;
    this.attributes = new Map();
    this.dataset = new Proxy({}, {
      get: (_target, prop) => this.getAttribute('data-' + String(prop).replace(/[A-Z]/g, (ch) => '-' + ch.toLowerCase())) || undefined,
      set: (_target, prop, value) => { this.setAttribute('data-' + String(prop).replace(/[A-Z]/g, (ch) => '-' + ch.toLowerCase()), String(value)); return true; },
      deleteProperty: (_target, prop) => { this.removeAttribute('data-' + String(prop).replace(/[A-Z]/g, (ch) => '-' + ch.toLowerCase())); return true; },
    });
    this.listeners = new Map();
    this.style = new StyleDeclaration(this);
    this.classList = new ClassList(this);
    this.value = '';
    this.checked = false;
  }
  appendChild(node) {
    if (node) {
      if (node.parentNode && node.parentNode.removeChild) node.parentNode.removeChild(node);
      node.parentNode = this;
      this.children.push(node);
      notifyAttachedScript(node);
    }
    return node;
  }
  insertBefore(node, referenceNode) {
    if (!node) return node;
    if (node.parentNode && node.parentNode.removeChild) node.parentNode.removeChild(node);
    const index = this.children.indexOf(referenceNode);
    node.parentNode = this;
    if (index < 0) this.children.push(node); else this.children.splice(index, 0, node);
    notifyAttachedScript(node);
    return node;
  }
  removeChild(node) { const idx = this.children.indexOf(node); if (idx >= 0) { this.children.splice(idx, 1); node.parentNode = null; } return node; }
  replaceChildren(...nodes) { for (const child of this.children) child.parentNode = null; this.children = []; for (const node of nodes) this.appendChild(node); }
  setAttribute(name, value) {
    const key = String(name);
    const val = String(value);
    this.attributes.set(key, val);
    if (key === 'class') this.classList.setFromString(val);
    if (key === 'style') {
      for (const part of val.split(';')) {
        const [prop, ...rest] = part.split(':');
        if (prop && rest.length) this.style[String(prop).trim()] = rest.join(':').trim();
      }
    }
    if (key === 'value') this.value = val;
    if (key === 'checked') this.checked = true;
  }
  getAttribute(name) { return this.attributes.has(String(name)) ? this.attributes.get(String(name)) : null; }
  hasAttribute(name) { return this.attributes.has(String(name)); }
  removeAttribute(name) { const key = String(name); this.attributes.delete(key); if (key === 'class') this.classList.setFromString(''); if (key === 'checked') this.checked = false; }
  addEventListener(name, handler) { const key = String(name); const list = this.listeners.get(key) || []; list.push(handler); this.listeners.set(key, list); }
  removeEventListener(name, handler) { const key = String(name); const list = this.listeners.get(key) || []; this.listeners.set(key, list.filter((item) => item !== handler)); }
  dispatchEvent(event) {
    const evt = event || { type: '' };
    if (!evt.target) evt.target = this;
    evt.currentTarget = this;
    for (const handler of this.listeners.get(evt.type || '') || []) handler.call(this, evt);
    if (evt.bubbles && this.parentNode && typeof this.parentNode.dispatchEvent === 'function') this.parentNode.dispatchEvent(evt);
    else if (evt.bubbles && activeDocumentShim && typeof activeDocumentShim.dispatchEvent === 'function') activeDocumentShim.dispatchEvent(evt);
    return !evt.defaultPrevented;
  }
  matches(selector) { return matchesSelector(this, selector); }
  closest(selector) { let cur = this; while (cur) { if (cur.matches && cur.matches(selector)) return cur; cur = cur.parentNode; } return null; }
  querySelector(selector) { return findFirst(this, (node) => node instanceof ElementNode && matchesSelector(node, selector)); }
  querySelectorAll(selector) { const out = []; walk(this, (node) => { if (node instanceof ElementNode && matchesSelector(node, selector)) out.push(node); }); return out; }
  getElementById(id) { return findFirst(this, (node) => node instanceof ElementNode && node.getAttribute('id') === String(id)); }
  get textContent() { return this.children.map((child) => child.textContent ?? child.toText()).join(''); }
  set textContent(value) { this.replaceChildren(new TextNode(String(value))); }
  get innerHTML() { return this.children.map((child) => child instanceof TextNode ? escapeHtml(child.textContent) : child.outerHTML).join(''); }
  set innerHTML(value) { this.replaceChildren(...parseHtmlFragment(String(value))); }
  get outerHTML() {
    const attrs = Array.from(this.attributes.entries()).map(([key, value]) => ` ${key}="${escapeHtml(value)}"`).join('');
    return `<${this.tagName}${attrs}>${this.innerHTML}</${this.tagName}>`;
  }
  get id() { return this.getAttribute('id') || ''; }
  set id(value) { this.setAttribute('id', value); }
  toggleAttribute(name, force) { const exists = this.hasAttribute(name); const shouldSet = force === undefined ? !exists : !!force; if (shouldSet) this.setAttribute(name, ''); else this.removeAttribute(name); return shouldSet; }
  submit() { this.dispatchEvent(new globalThis.Event('submit', { bubbles: true, cancelable: true })); }
  click() { this.dispatchEvent(new globalThis.Event('click', { bubbles: true, cancelable: true })); }
  toText() { return this.textContent; }
}

function notifyAttachedScript(node) {
  if (!(node instanceof ElementNode) || node.tagName !== 'script' || !node.src) return;
  const remoteScripts = globalThis.__venomCompatRemoteScripts || (globalThis.__venomCompatRemoteScripts = []);
  remoteScripts.push(String(node.src));
  queueMicrotask(() => {
    if (typeof node.onload === 'function') node.onload(new globalThis.Event('load'));
    node.dispatchEvent(new globalThis.Event('load'));
  });
}

class TextNode {
  constructor(text) { this.text = String(text || ''); this.parentNode = null; this.nodeType = 3; }
  get textContent() { return this.text; }
  set textContent(value) { this.text = String(value); }
  toText() { return this.text; }
}

function parseHtmlFragment(html) {
  const root = new ElementNode('fragment');
  const stack = [root];
  const tokenRe = /<\/?[a-zA-Z][^>]*>|[^<]+/g;
  for (const token of String(html).match(tokenRe) || []) {
    if (token.startsWith('</')) { if (stack.length > 1) stack.pop(); continue; }
    if (token.startsWith('<')) {
      const match = token.match(/^<\s*([a-zA-Z0-9_-]+)([^>]*)>/);
      if (!match) continue;
      const node = new ElementNode(match[1]);
      const attrRe = /([a-zA-Z0-9_:-]+)(?:\s*=\s*['"]([^'"]*)['"])?/g;
      let attr;
      while ((attr = attrRe.exec(match[2] || ''))) node.setAttribute(attr[1], attr[2] ?? '');
      stack[stack.length - 1].appendChild(node);
      if (!/\/>$/.test(token) && !['br', 'img', 'input', 'meta', 'link'].includes(node.tagName)) stack.push(node);
    } else {
      stack[stack.length - 1].appendChild(new TextNode(token));
    }
  }
  return root.children;
}

function parseSimpleSelector(selector) {
  const result = { tag: '', id: '', classes: [], attrs: [] };
  let raw = String(selector || '').trim();
  const tag = raw.match(/^[a-zA-Z][a-zA-Z0-9_-]*/);
  if (tag) { result.tag = tag[0].toLowerCase(); raw = raw.slice(tag[0].length); }
  const re = /([#.][a-zA-Z0-9_-]+)|\[([^\]=]+)(?:=['"]?([^'"\]]+)['"]?)?\]/g;
  let m;
  while ((m = re.exec(raw))) {
    if (m[1] && m[1].startsWith('#')) result.id = m[1].slice(1);
    else if (m[1] && m[1].startsWith('.')) result.classes.push(m[1].slice(1));
    else if (m[2]) result.attrs.push({ name: m[2], value: m[3] });
  }
  return result;
}

function matchesSimpleSelector(node, selector) {
  if (!(node instanceof ElementNode)) return false;
  const parsed = parseSimpleSelector(selector);
  if (parsed.tag && node.tagName !== parsed.tag) return false;
  if (parsed.id && node.getAttribute('id') !== parsed.id) return false;
  for (const cls of parsed.classes) if (!node.classList.contains(cls)) return false;
  for (const attr of parsed.attrs) {
    if (!node.hasAttribute(attr.name)) return false;
    if (attr.value !== undefined && node.getAttribute(attr.name) !== attr.value) return false;
  }
  return !!(parsed.tag || parsed.id || parsed.classes.length || parsed.attrs.length);
}

function matchesSelector(node, selector) {
  const groups = String(selector || '').split(',').map((part) => part.trim()).filter(Boolean);
  for (const group of groups) {
    const parts = group.split(/\s+/).filter(Boolean);
    let cur = node;
    let ok = true;
    for (let i = parts.length - 1; i >= 0; --i) {
      while (cur && !matchesSimpleSelector(cur, parts[i])) cur = cur.parentNode;
      if (!cur) { ok = false; break; }
      cur = cur.parentNode;
    }
    if (ok) return true;
  }
  return false;
}

function walk(node, visitor) {
  visitor(node);
  for (const child of node.children || []) walk(child, visitor);
}

function findFirst(node, predicate) {
  if (predicate(node)) return node;
  for (const child of node.children || []) {
    const found = findFirst(child, predicate);
    if (found) return found;
  }
  return null;
}

const documentElement = new ElementNode('html');
const head = new ElementNode('head');
const body = new ElementNode('body');
const root = new ElementNode('div');
root.setAttribute('id', 'venom-root');
documentElement.appendChild(head);
documentElement.appendChild(body);
body.appendChild(root);
const documentListeners = new Map();
function addDocumentListener(type, handler) { const key = String(type); const list = documentListeners.get(key) || []; list.push(handler); documentListeners.set(key, list); }
function dispatchDocumentEvent(event) { const evt = event || { type: '' }; if (!evt.target) evt.target = root; evt.currentTarget = documentShim; for (const handler of documentListeners.get(evt.type || '') || []) handler.call(documentShim, evt); return !evt.defaultPrevented; }
const documentShim = {
  createElement: (tag) => new ElementNode(tag),
  createTextNode: (text) => new TextNode(text),
  addEventListener: addDocumentListener,
  removeEventListener: (type, handler) => { const key = String(type); const list = documentListeners.get(key) || []; documentListeners.set(key, list.filter((item) => item !== handler)); },
  dispatchEvent: dispatchDocumentEvent,
  querySelector: (selector) => documentElement.querySelector(selector),
  querySelectorAll: (selector) => documentElement.querySelectorAll(selector),
  getElementById: (id) => documentElement.getElementById(id),
  getElementsByTagName: (tag) => {
    const expected = String(tag || '*').toLowerCase();
    const out = [];
    walk(documentElement, (node) => {
      if (node instanceof ElementNode && (expected === '*' || node.tagName === expected)) out.push(node);
    });
    return out;
  },
  documentElement,
  head,
  body,
  baseURI: pathToFileURL(join(distDir, 'index.html')).href,
};
activeDocumentShim = documentShim;

const windowListeners = new Map();
function addWindowListener(type, handler) { const list = windowListeners.get(String(type)) || []; list.push(handler); windowListeners.set(String(type), list); }
function dispatchWindowEvent(event) { for (const handler of windowListeners.get(event.type || '') || []) handler.call(globalThis.window, event); return !event.defaultPrevented; }

globalThis.Element = ElementNode;
globalThis.HTMLElement = ElementNode;
globalThis.HTMLFormElement = ElementNode;
globalThis.Event = class Event {
  constructor(type, init = {}) { this.type = String(type || ''); this.defaultPrevented = false; this.bubbles = !!init.bubbles; this.cancelable = !!init.cancelable; Object.assign(this, init); }
  preventDefault() { if (this.cancelable !== false) this.defaultPrevented = true; }
};
globalThis.SubmitEvent = globalThis.Event;
globalThis.document = documentShim;
globalThis.window = { addEventListener: addWindowListener, dispatchEvent: dispatchWindowEvent, document: documentShim };
globalThis.self = globalThis.window;
globalThis.location = { pathname: '/', origin: 'http://localhost', href: 'http://localhost/' };
globalThis.history = {
  pushes: [],
  pushState: (_state, _title, url) => {
    const next = new URL(String(url || '/'), globalThis.location.href);
    globalThis.location.pathname = next.pathname;
    globalThis.location.href = next.href;
    globalThis.history.pushes.push(next.pathname);
  },
};
Object.defineProperty(globalThis, 'navigator', { configurable: true, value: { userAgent: 'VenomCompatHarness/1.0' } });

if (!globalThis.crypto) {
  Object.defineProperty(globalThis, 'crypto', { configurable: true, value: webcrypto });
}

function fnvPackageHash(buffer) {
  const bytes = new Uint8Array(buffer);
  let hash = 1469598103934665603n;
  const prime = 0x100000001b3n;
  for (let i = 0; i < bytes.length; ++i) {
    hash ^= BigInt(i >= 72 && i < 80 ? 0 : bytes[i]);
    hash = BigInt.asUintN(64, hash * prime);
  }
  return '0x' + hash.toString(16).padStart(16, '0');
}

async function sha256Hex(buffer) {
  const digest = new Uint8Array(await webcrypto.subtle.digest('SHA-256', buffer));
  return Array.from(digest, (byte) => byte.toString(16).padStart(2, '0')).join('');
}

class HarnessWorker {
  constructor(url, options = {}) {
    this.url = String(url && url.href ? url.href : url);
    this.options = options;
    this.onmessage = null;
    this.onerror = null;
    globalThis.__venomLoaderWorkerUrl = this.url;
  }
  postMessage(message) {
    queueMicrotask(async () => {
      try {
        const nonce = message && message.nonce;
        if (!message || message.protocol !== 1 || message.type !== 'prepare') throw new Error('unexpected worker message');
        const packageResponse = await globalThis.fetch(message.packageUrl);
        const packageBytes = await packageResponse.arrayBuffer();
        if (message.maxPackageBytes && packageBytes.byteLength > message.maxPackageBytes) throw new Error('package exceeds worker limit');
        const wasmResponse = await globalThis.fetch(message.quickJsWasmUrl);
        const wasmBytes = await wasmResponse.arrayBuffer();
        if (message.maxWasmBytes && wasmBytes.byteLength > message.maxWasmBytes) throw new Error('QuickJS WASM exceeds worker limit');
        const actualPackageSha256 = await sha256Hex(packageBytes);
        if (actualPackageSha256 !== String(message.expectedPackageSha256 || '').toLowerCase()) throw new Error('package SHA-256 attestation mismatch');
        const actualPackageHash = fnvPackageHash(packageBytes);
        if (actualPackageHash !== String(message.expectedPackageHash || '').toLowerCase()) throw new Error('package hash attestation mismatch');
        const actualQuickJsWasmSha256 = await sha256Hex(wasmBytes);
        if (actualQuickJsWasmSha256 !== String(message.expectedQuickJsWasmSha256 || '').toLowerCase()) throw new Error('QuickJS WASM digest attestation mismatch');
        const quickJsModule = await WebAssembly.compile(wasmBytes);
        globalThis.__venomLoaderWorkerPrepared = {
          packageBytes: packageBytes.byteLength,
          quickJsWasmBytes: wasmBytes.byteLength,
          packageUrl: String(message.packageUrl || ''),
          quickJsWasmUrl: String(message.quickJsWasmUrl || ''),
          workerUrl: this.url,
        };
        if (typeof this.onmessage === 'function') {
          this.onmessage({ data: { protocol: 1, type: 'ready', nonce, packageHash: actualPackageHash, packageSha256: actualPackageSha256, quickJsWasmSha256: actualQuickJsWasmSha256, quickJsRuntimeReady: true, packageBytes, quickJsModule } });
        }
      } catch (error) {
        const messageText = String(error && error.message ? error.message : error);
        if (typeof this.onmessage === 'function') this.onmessage({ data: { protocol: 1, type: 'error', nonce: message && message.nonce, message: messageText } });
        if (typeof this.onerror === 'function') this.onerror({ message: messageText });
      }
    });
  }
  terminate() {}
}

globalThis.Worker = HarnessWorker;

let assetNameMap = null;
let runtimeAssetDirectory = join(distDir, 'assets');
async function resolveAssetFileName(rawInput) {
  if (!assetNameMap) {
    assetNameMap = new Map();
    try {
      const manifest = await readFile(join(distDir, 'assets', 'asset-manifest.txt'), 'utf8');
      for (const line of manifest.split(/\r?\n/)) {
        if (!line || line.startsWith('VASM') || line.startsWith('version') || line.startsWith('count') || line.startsWith('source')) continue;
        const parts = line.split('\t');
        if (parts.length >= 3) {
          assetNameMap.set(parts[0], parts[1]);
          assetNameMap.set(parts[0].replace(/^assets\//, ''), parts[1]);
          assetNameMap.set(parts[2], parts[1]);
          assetNameMap.set(parts[2].replace(/^assets\//, ''), parts[1]);
        }
      }
    } catch {
      assetNameMap = new Map();
    }
  }
  const key = String(rawInput || '').replace(/^\.\//, '').replace(/^assets\//, '');
  return assetNameMap.get(key) || assetNameMap.get('assets/' + key) || key;
}

globalThis.fetch = async (url) => {
  const raw = String(url);
  if ((denyFetch && /compat-data\.json|assets_compat-data/i.test(raw)) || /blocked-secret\.json/i.test(raw)) throw new Error('fetch denied by compatibility harness');
  let normalized;
  if (raw.startsWith('file://')) normalized = new URL(raw);
  else if (/^(?:\.\.\/|\.\/).+\.(?:vbc|wasm)$/i.test(raw)) normalized = resolve(runtimeAssetDirectory, raw);
  else normalized = join(distDir, 'assets', await resolveAssetFileName(raw));
  const bytes = await readFile(normalized);
  const text = Buffer.from(bytes).toString('utf8');
  return {
    ok: true,
    status: 200,
    statusText: 'OK',
    url: raw,
    headers: { get: (name) => String(name || '').toLowerCase() === 'x-venom-fixture' ? 'host-api-v83' : null, forEach: (cb) => { if (typeof cb === 'function') cb('host-api-v83', 'x-venom-fixture'); } },
    text: async () => text,
    json: async () => JSON.parse(text),
    arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
  };
};

if (strictNoSourceEval) {
  globalThis.__venomCompatFunctionBlocked = 0;
  globalThis.Function = function blockedFunctionConstructor() { globalThis.__venomCompatFunctionBlocked++; throw new Error('source eval disabled by compatibility harness'); };
  globalThis.eval = function blockedEval() { throw new Error('eval disabled by compatibility harness'); };
}

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
const runtimeFile = assetFiles.find((name) => /(?:^|\/)runtime(?:\.[a-f0-9]+)?\.js$/.test(name) || /(?:^|\/)r(?:\.[a-f0-9]+)?\.js$/.test(name));
const packageFile = assetFiles.find((name) => /(?:^|\/)app(?:\.[a-f0-9]+)?\.vbc$/.test(name));
const quickJsEngineFile = assetFiles.find((name) => /(?:^|\/)(?:quickjs-engine|engine)(?:\.[a-f0-9]+)?\.js$/.test(name));
if (!loaderFile || !runtimeFile || !packageFile || !quickJsEngineFile) throw new Error(`missing loader/runtime/package/QuickJS engine assets: ${assetFiles.join(', ')}`);

const runtimePath = join(distDir, 'assets', ...runtimeFile.split('/'));
runtimeAssetDirectory = dirname(runtimePath);
const packageRelative = relative(dirname(runtimeFile), packageFile).replace(/\\/g, '/');
const packageUrl = packageRelative.startsWith('.') ? packageRelative : './' + packageRelative;
const loaderText = await readFile(join(distDir, 'assets', ...loaderFile.split('/')), 'utf8');
const bindingToken = (loaderText.match(/bindingToken:\s*'([^']+)'/) || [])[1] || '';
const enginePath = join(distDir, 'assets', ...quickJsEngineFile.split('/'));
let bootErrors = [];
const originalConsoleError = console.error.bind(console);
console.error = (...args) => {
  if (String(args[0] || '').includes('[venom] boot failed')) bootErrors.push(args.map((arg) => String(arg && arg.message ? arg.message : arg)).join(' '));
  originalConsoleError(...args);
};
let result;
if (viaLoader) {
  await import(pathToFileURL(join(distDir, 'assets', ...loaderFile.split('/'))));
  const deadline = Date.now() + 2500;
  while (Date.now() < deadline) {
    await Promise.resolve();
    await new Promise((resolveTimer) => setTimeout(resolveTimer, 10));
    const failure = root.querySelector('[data-venom-failure]');
    if (failure) throw new Error(failure.textContent || 'loader rendered failure panel');
    if (globalThis.__venomRuntime && typeof globalThis.__venomRuntime.quickJsExecutionSnapshot === 'function') {
      const snapshot = globalThis.__venomRuntime.quickJsExecutionSnapshot();
      if (snapshot && snapshot.count >= 1) break;
    }
  }
  if (bootErrors.length) throw new Error('loader boot failed: ' + bootErrors.join(' | '));
  if (!globalThis.__venomRuntime || typeof globalThis.__venomRuntime.quickJsExecutionSnapshot !== 'function') throw new Error('loader did not install Venom runtime bridge');
  if (!globalThis.__venomLoaderWorkerPrepared || !String(globalThis.__venomLoaderWorkerPrepared.packageUrl || '').includes('/assets/app/app.')) throw new Error('loader did not prepare grouped app package through worker');
  if (!String(globalThis.__venomLoaderWorkerPrepared.quickJsWasmUrl || '').includes('/assets/runtime/runtime.')) throw new Error('loader did not prepare grouped QuickJS WASM through worker');
  if (!String(globalThis.__venomLoaderWorkerPrepared.workerUrl || '').includes('/assets/workers/worker.')) throw new Error('loader did not resolve grouped worker URL');
  result = { route: '/' };
} else {
  const runtime = await import(pathToFileURL(runtimePath));
  const assetBaseUrl = pathToFileURL(join(distDir, 'assets') + '/').href;
  result = await runtime.bootVenom({ root, packageUrl, assetBaseUrl, quickJsEngineUrl: pathToFileURL(enginePath).href, bindingToken, hardened: true });
}
await Promise.resolve();
async function waitForCompatibilityWork(timeoutMs = 1500) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    await Promise.resolve();
    if (globalThis.__venomCompatTimer === 'timer-fired' &&
        (mode !== 'host-bridge' || globalThis.__venomCompatFetchText === 'text-ok')) return;
    await new Promise((resolveTimer) => setTimeout(resolveTimer, 10));
  }
}
await waitForCompatibilityWork();
await Promise.resolve();

if (result.route !== '/') throw new Error(`expected root route, got ${result.route}`);
if (!globalThis.__venomRuntime || globalThis.__venomRuntime.quickJsRuntimeAbi < 12) throw new Error('Venom runtime bridge was not installed');

if (mode === 'compat' || mode === 'host-bridge' || mode === 'capability-denied') {
  const output = root.getElementById('compat-output');
  const button = root.getElementById('compat-button');
  if (!output || !button) throw new Error('compat fixture DOM was not rendered');
  button.dispatchEvent(new globalThis.Event('click', { bubbles: true, cancelable: true }));
  if (globalThis.__venomCompatClosure !== 7) throw new Error(`closure semantics failed: ${globalThis.__venomCompatClosure}`);
  if (globalThis.__venomCompatIterator !== 20) throw new Error(`iterator/array semantics failed: ${globalThis.__venomCompatIterator}`);
  if (globalThis.__venomCompatException !== 'compat-exception') throw new Error(`exception semantics failed: ${globalThis.__venomCompatException}`);
  if (globalThis.__venomCompatPromise !== 42) throw new Error(`promise semantics failed: ${globalThis.__venomCompatPromise}`);
  if (globalThis.__venomCompatTimer !== 'timer-fired') throw new Error(`timer bridge failed: ${globalThis.__venomCompatTimer}`);
  if (globalThis.__venomCompatClicks !== 1 || output.getAttribute('data-clicked') !== 'yes') throw new Error('event listener dispatch failed');
  if (globalThis.__venomCompatModule !== 65) throw new Error(`module execution failed: ${globalThis.__venomCompatModule}`);
  if (globalThis.__venomCompatModuleDependency !== 23) throw new Error(`module dependency failed: ${globalThis.__venomCompatModuleDependency}`);
  if (globalThis.__venomCompatModuleExtra !== 17 || globalThis.__venomCompatModuleGraph !== 93) throw new Error(`module graph expansion failed: extra=${globalThis.__venomCompatModuleExtra} branch=${globalThis.__venomCompatModuleBranch} graph=${globalThis.__venomCompatModuleGraph} order=${globalThis.__venomCompatModuleOrderText}`);
  if (!output.textContent.includes('dom:7:20') || output.getAttribute('data-sync') !== 'done') throw new Error(`DOM write failed: ${output.textContent}`);
  if (output.dataset.bridge !== 'dataset-ready' || output.getAttribute('data-remove-me') !== null) throw new Error('dataset/removeAttribute bridge failed');
}

if (mode === 'host-bridge') {
  const output = root.getElementById('compat-output');
  const child = root.querySelector('#compat-root .bridge-child');
  const form = root.getElementById('compat-form');
  const input = root.querySelector('input[name="bridge-input"]');
  const link = root.querySelector('a[data-route="about"]');
  if (!child || child.textContent !== 'inner-ready') throw new Error('innerHTML parsing/mutation failed');
  if (!output.classList.contains('ready') || output.classList.contains('pending')) throw new Error('classList mutation failed');
  if (output.style.getPropertyValue('borderColor') !== 'green') throw new Error('style.setProperty bridge failed');
  if (output.style.backgroundColor !== 'blue') throw new Error('style property bridge failed');
  if (!form || !input || globalThis.__venomCompatForm !== 'posted:form-value') throw new Error(`form submit bridge failed: ${globalThis.__venomCompatForm}`);
  if (globalThis.__venomCompatFetch !== 'fixture-data:83') throw new Error(`fetch bridge failed: ${globalThis.__venomCompatFetch}`);
  if (globalThis.__venomCompatFetchStatus !== '200:ok' || globalThis.__venomCompatFetchHeader !== 'host-api-v83' || globalThis.__venomCompatFetchDetail !== 'text-ok') throw new Error(`fetch response metadata failed: ${globalThis.__venomCompatFetchStatus}:${globalThis.__venomCompatFetchHeader}:${globalThis.__venomCompatFetchDetail}`);
  if (globalThis.__venomCompatDenyByDefault !== 'blocked-secret.json:denied') throw new Error(`deny-by-default host call failed: ${globalThis.__venomCompatDenyByDefault}`);
  if (globalThis.__venomCompatAwait !== 'await-ok') throw new Error(`async/await bridge failed: ${globalThis.__venomCompatAwait}`);
  if (globalThis.__venomCompatAsyncError !== 'async-bridge-error') throw new Error(`async error propagation failed: ${globalThis.__venomCompatAsyncError}`);
  if (!link) throw new Error('route link missing');
  link.dispatchEvent(new globalThis.Event('click', { bubbles: true, cancelable: true }));
  await Promise.resolve();
  if (globalThis.__venomCompatRoute !== 'about.html' || globalThis.__venomCompatRouteHydrated !== 'about-ready') throw new Error(`route click/hydration handler failed: ${globalThis.__venomCompatRoute}:${globalThis.__venomCompatRouteHydrated}`);
  if (globalThis.location.pathname !== '/about.html' && globalThis.location.pathname !== '/about') throw new Error(`route push failed: ${globalThis.location.pathname}`);
  const about = root.getElementById('about-output');
  if (!about || about.textContent !== 'about-ready') throw new Error('multi-page route DOM load failed');
}

if (mode === 'capability-denied') {
  if (globalThis.__venomCompatFetchDenied !== 'fetch denied by compatibility harness') throw new Error(`capability-denied fetch was not trapped: ${globalThis.__venomCompatFetchDenied}`);
}

if (mode === 'strict-no-eval' || mode === 'real-engine-strict') {
  if (globalThis.__venomCompatFunctionBlocked !== 0) throw new Error('protected runtime attempted to use Function constructor fallback');
  const snapshot = globalThis.__venomRuntime.quickJsExecutionSnapshot();
  if (!snapshot || snapshot.count < 1) throw new Error('protected runtime did not record QuickJS execution');
  const records = Array.isArray(snapshot.records) ? snapshot.records : [];
  if (!records.length) throw new Error('protected runtime did not expose execution records');
  if (!records.some((entry) => entry && entry.kind === 'quickjs.executionRecord' && entry.wasmAccepted === true)) throw new Error('protected runtime did not accept the WASM execution boundary');
  const fallback = globalThis.__venomRuntime.quickJsFallbackPolicy();
  if (!fallback || fallback.mode !== 'explicit-policy-gated') throw new Error('protected fallback policy is not explicit-policy-gated');
  if (mode === 'real-engine-strict') {
    if (globalThis.__venomRuntime.quickJsRuntimeAbi !== 12) throw new Error(`expected QuickJS runtime ABI 12, got ${globalThis.__venomRuntime.quickJsRuntimeAbi}`);
    if (globalThis.__venomRuntime.quickJsWasmRuntimeMode !== 'quickjs-wasm-abi12-upstream-global-host-api-shims') throw new Error(`expected quickjs-wasm-abi12-upstream-global-host-api-shims mode, got ${globalThis.__venomRuntime.quickJsWasmRuntimeMode}`);
    if (!globalThis.__venomRuntime.quickJsWasmUrl || !/(?:quickjs-runtime|runtime(?:\.[a-f0-9]+)?\.wasm)/.test(globalThis.__venomRuntime.quickJsWasmUrl)) throw new Error('real-engine runtime did not expose QuickJS WASM URL');
    if (fallback.mode !== 'explicit-policy-gated') throw new Error('real-engine fallback policy is not explicit-policy-gated');
    if (!records.some((entry) => entry && entry.engine === 'quickjs-wasm-backend-candidate' && entry.wasmAccepted === true)) throw new Error('protected execution records did not identify the accepted QuickJS WASM backend');
    const bridgeRecords = records.filter((entry) => entry && entry.hostBridgeTelemetry && entry.hostBridgeTelemetry.boundary === 'upstream-quickjs-wasm-host-call-bridge');
    if (!bridgeRecords.length) throw new Error('real-engine protected execution did not expose host bridge telemetry');
    if (!bridgeRecords.every((entry) => entry.hostBridgeTelemetry.wasmAccepted === true && entry.hostBridgeTelemetry.upstreamReady === true && entry.hostBridgeTelemetry.fallbackRequired === false)) throw new Error('host bridge telemetry did not prove accepted upstream WASM execution');
    if (!bridgeRecords.some((entry) => entry.hostBridgeTelemetry.sourceKind === 'opaque-vqjsbc03-native-object')) throw new Error('host bridge telemetry did not preserve the native VQJSBC03 opacity boundary');
    const ops = new Set(bridgeRecords.flatMap((entry) => Array.isArray(entry.hostBridgeTelemetry.operations) ? entry.hostBridgeTelemetry.operations : []));
    const opaqueRecords = bridgeRecords.filter((entry) => entry.hostBridgeTelemetry.sourceKind === 'opaque-vqjsbc03-native-object');
    if (!opaqueRecords.every((entry) => entry.hostBridgeTelemetry.operationInference === 'disabled-for-opaque-bytecode')) throw new Error('opaque protected chunks attempted source-derived operation inference');
    if (opaqueRecords.some((entry) => Array.isArray(entry.hostBridgeTelemetry.operations) && entry.hostBridgeTelemetry.operations.length !== 0)) throw new Error(`opaque protected chunks leaked source-derived operation categories: ${Array.from(ops).sort().join(',')}`);
    if (!bridgeRecords.some((entry) => (entry.hostBridgeTelemetry.hostCallCount || 0) > 0)) throw new Error('host bridge telemetry did not observe WASM host calls');
    if (bridgeRecords.some((entry) => (entry.hostBridgeTelemetry.effectReplayCount || 0) !== 0)) throw new Error('strict protected runtime replayed source-derived browser effects');
    if (bridgeRecords.some((entry) => entry.hostBridgeTelemetry.replayEvalUsed || entry.hostBridgeTelemetry.replayFunctionConstructorUsed)) throw new Error('host bridge replay used eval/Function fallback');
    if (!bridgeRecords.every((entry) => entry.hostBridgeTelemetry.replayMode === 'native-wasm-host-calls-only')) throw new Error('strict protected runtime did not preserve the native host-call-only boundary');
  }
}

if (requireRemoteScript) {
  const remoteScripts = Array.isArray(globalThis.__venomCompatRemoteScripts) ? globalThis.__venomCompatRemoteScripts : [];
  if (!remoteScripts.length) throw new Error('production loader did not insert any remote script chunks');
  if (!remoteScripts.every((url) => /^https?:|^blob:|^file:/.test(String(url)))) throw new Error(`invalid remote script URL observed: ${remoteScripts.join(',')}`);
}
if (assertHtmlFidelity) {
  const spacing = root.getElementById('spacing');
  const heading = root.getElementById('heading');
  const pre = root.getElementById('preformatted');
  if (!spacing || spacing.textContent !== 'User Agent (Old)') throw new Error(`inline boundary whitespace was not preserved: ${spacing && spacing.textContent}`);
  if (!heading || heading.textContent !== 'Fingerprint Scanner tests') throw new Error(`heading boundary whitespace was not preserved: ${heading && heading.textContent}`);
  if (!pre || pre.textContent !== '{\n  "value": 1\n}') throw new Error(`preformatted whitespace was not preserved: ${JSON.stringify(pre && pre.textContent)}`);
}
console.log(`browser compatibility harness passed: ${mode}${viaLoader ? ':via-loader' : ''}${requireRemoteScript ? ':remote-script' : ''}${assertHtmlFidelity ? ':html-fidelity' : ''}`);
