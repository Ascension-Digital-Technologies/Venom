function runtimeIntegrityText(value) {
  if (value === null || value === undefined) return '';
  if (value instanceof Map) return Array.from(value.entries()).sort((a, b) => String(a[0]).localeCompare(String(b[0]))).map(([k, v]) => String(k) + '=' + String(v)).join('|');
  if (ArrayBuffer.isView(value)) return Array.from(value).join(',');
  if (typeof value === 'object') return Object.keys(value).sort().map((key) => key + '=' + runtimeIntegrityText(value[key])).join('|');
  return String(value);
}
function computeRuntimeIntegritySeal(opcodeMap = null) {
  const text = runtimeIntegrityText(activeReleaseDiversification) + '\n' + runtimeIntegrityText(activeTurboJsAbiFingerprint) + '\n' + runtimeIntegrityText(opcodeMap);
  return fnv1a32(textEncoder.encode(text)) >>> 0;
}
function assertRuntimeIntegrity(opcodeMap = null) {
  if (!activeRuntimeIntegritySeal || computeRuntimeIntegritySeal(opcodeMap) !== activeRuntimeIntegritySeal) throw new Error('runtime integrity seal mismatch');
}

function parseTurboJsAbiFingerprint(section, required) {
  if (!section) {
    if (required) throw new Error('missing TurboJS ABI fingerprint');
    return null;
  }
  const data = section.data;
  if (data.byteLength !== 24 || textDecoder.decode(data.subarray(0, 8)) !== 'VQAF0001') throw new Error('invalid TurboJS ABI fingerprint');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  if (readU32(view, 8) !== 1 || readU32(view, 16) !== 12 || readU32(view, 20) !== 24) throw new Error('unsupported TurboJS ABI fingerprint');
  return Object.freeze({ hash: readU32(view, 12) >>> 0, runtimeAbi: 12, functionExports: 24 });
}


function parseReleaseDiversification(section, required) {
  if (!section) {
    if (required) throw new Error('missing release diversification contract');
    return null;
  }
  const data = section.data;
  const expectedSize = 8 + 16 + 89 * 8 + 8 * 8;
  if (data.byteLength !== expectedSize || textDecoder.decode(data.subarray(0, 8)) !== 'VRDIV003') throw new Error('invalid release diversification contract');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  if (readU32(view, 8) !== 3 || readU32(view, 16) !== 89 || readU32(view, 20) !== 8) throw new Error('unsupported release diversification contract');
  const hostWireToLogical = new Uint32Array(90);
  const resultLogicalToWire = new Uint32Array(8);
  let offset = 24;
  for (let i = 0; i < 89; i++, offset += 8) {
    const logical = readU32(view, offset);
    const wire = readU32(view, offset + 4);
    if (!logical || logical > 89 || !wire || wire > 89 || hostWireToLogical[wire]) throw new Error('invalid host-call diversification permutation');
    hostWireToLogical[wire] = logical;
  }
  const seenFields = new Uint8Array(8);
  for (let i = 0; i < 8; i++, offset += 8) {
    const logical = readU32(view, offset);
    const wire = readU32(view, offset + 4);
    if (logical > 7 || wire > 7 || seenFields[wire]) throw new Error('invalid result diversification permutation');
    seenFields[wire] = 1;
    resultLogicalToWire[logical] = wire;
  }
  return Object.freeze({ bytes: data, hostWireToLogical, resultLogicalToWire, seedTag: readU32(view, 12) >>> 0 });
}

function parseRuntimePolicy(section) {
  const policy = new Map();
  if (!section) return policy;
  const data = section.data;
  if (data.byteLength >= 24 && textDecoder.decode(data.subarray(0, 8)) === 'VRPOL002') {
    const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
    const version = readU32(view, 8);
    if (version !== 2) throw new Error('unsupported binary runtime policy version: ' + version);
    const flags = readU32(view, 12);
    policy.set('fail_closed', (flags & (1 << 0)) !== 0 ? 'true' : 'false');
    policy.set('runtime_hardened', (flags & (1 << 1)) !== 0 ? 'true' : 'false');
    policy.set('strip_debug_metadata', (flags & (1 << 2)) !== 0 ? 'true' : 'false');
    policy.set('freeze_host_bridge', (flags & (1 << 3)) !== 0 ? 'true' : 'false');
    policy.set('require_integrity', (flags & (1 << 4)) !== 0 ? 'true' : 'false');
    policy.set('wasm_runtime', (flags & (1 << 5)) !== 0 ? 'true' : 'false');
    return policy;
  }
  const lines = textDecoder.decode(data).split(/\r?\n/).filter((line) => line.length > 0);
  if (lines[0] !== 'VENOM_RUNTIME_POLICY_V1') throw new Error('invalid runtime policy metadata header');
  for (const line of lines.slice(1)) {
    const idx = line.indexOf('=');
    if (idx <= 0) continue;
    policy.set(line.slice(0, idx), line.slice(idx + 1));
  }
  return policy;
}

function enforceRuntimePolicy(pkg, policy, options) {
  const hardenedFlag = (pkg.flags & PACKAGE_FLAG.RUNTIME_HARDENED) !== 0;
  const releaseFlag = (pkg.flags & PACKAGE_FLAG.RELEASE) !== 0;
  if (releaseFlag && !hardenedFlag) throw new Error('release package is missing runtime hardening');
  if (releaseFlag && policy.get('fail_closed') !== 'true') throw new Error('release package policy is not fail-closed');
  if (hardenedFlag && policy.get('runtime_hardened') !== 'true') throw new Error('runtime hardening policy mismatch');
  if (options && options.hardened && !hardenedFlag) throw new Error('loader expected a hardened package');
  return Object.freeze({
    failClosed: policy.get('fail_closed') === 'true',
    runtimeHardened: hardenedFlag,
    freezeHostBridge: policy.get('freeze_host_bridge') === 'true',
    stripDebugMetadata: policy.get('strip_debug_metadata') === 'true',
    releaseProfile: releaseFlag,
  });
}

function findSection(pkg, type, name = null) {
  const section = pkg.sections.find((candidate) => candidate.type === type && (name === null || sectionNameMatches(type, candidate.name, name)));
  if (!section) throw new Error('missing required VBC section: ' + (name || type));
  return section;
}

function findOptionalSection(pkg, type, name = null) {
  return pkg.sections.find((candidate) => candidate.type === type && (name === null || sectionNameMatches(type, candidate.name, name))) || null;
}

function parseStringTable(section) {
  const data = section.data;
  requireAscii(data, 'VSTR0011', 'protected string table');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const version = readU32(view, 8);
  const count = readU32(view, 12);
  const tableSeed = readU32(view, 16);
  if (version !== 2) throw new Error('unsupported protected string table version: ' + version);
  const entryBase = 20;
  const dataBase = entryBase + count * 12;
  if (dataBase > data.byteLength) throw new Error('protected string table directory out of range');
  const cache = new Array(count);
  const decode = (id) => {
    if (!Number.isInteger(id) || id < 0 || id >= count) return '';
    if (cache[id] !== undefined) return cache[id];
    const entry = entryBase + id * 12;
    const off = readU32(view, entry);
    const size = readU32(view, entry + 4);
    const nonce = readU32(view, entry + 8);
    if (dataBase + off + size > data.byteLength) throw new Error('protected string record out of range');
    let state = (tableSeed ^ nonce ^ Math.imul(size, 0x27d4eb2d)) >>> 0;
    const plain = new Uint8Array(size);
    for (let i = 0; i < size; ++i) {
      state ^= state << 13; state >>>= 0;
      state ^= state >>> 17; state >>>= 0;
      state ^= state << 5; state >>>= 0;
      plain[i] = data[dataBase + off + i] ^ (state & 0xff);
    }
    const value = textDecoder.decode(plain);
    plain.fill(0);
    cache[id] = value;
    return value;
  };
  return new Proxy(Object.create(null), {
    get(_target, property) {
      if (property === 'length') return count;
      if (property === 'get') return decode;
      const id = typeof property === 'string' && /^\d+$/.test(property) ? Number(property) : -1;
      return decode(id);
    },
  });
}

function parseRouteTable(section, strings) {
  const data = section.data;
  requireAscii(data, 'VRTE0003', 'route table');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const version = readU32(view, 8);
  const count = readU32(view, 12);
  if (version !== 1) throw new Error('unsupported route table version: ' + version);
  const routes = [];
  for (let i = 0; i < count; ++i) {
    const base = 16 + i * 24;
    const routeId = readU32(view, base + 0);
    const sourceId = readU32(view, base + 4);
    routes.push({
      route: strings[routeId] || '/',
      sourcePath: strings[sourceId] || '',
      bytecodeOffset: readU32(view, base + 8),
      bytecodeSize: readU32(view, base + 12),
      instructionCount: readU32(view, base + 16),
      flags: readU32(view, base + 20),
    });
  }
  return routes;
}

function parseVmBytecode(section) {
  const data = section.data;
  requireAscii(data, 'VBCODE03', 'VM bytecode');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const version = readU32(view, 8);
  const instructionSize = readU32(view, 12);
  const instructionCount = readU32(view, 16);
  const flags = readU32(view, 20);
  if (version !== 1) throw new Error('unsupported VM bytecode version: ' + version);
  if (instructionSize !== 16) throw new Error('unsupported VM instruction size: ' + instructionSize);
  return { data, view, dataOffset: 24, instructionSize, instructionCount, flags };
}


function parseAssetManifest(section) {
  const map = new Map();
  if (!section) return map;
  const source = textDecoder.decode(section.data);
  const lines = source.split(/\r?\n/).filter(Boolean);
  if (lines[0] !== 'VASM0005') throw new Error('invalid asset manifest magic');
  for (const line of lines.slice(4)) {
    const parts = line.split('\t');
    if (parts.length < 6) continue;
    const [sourcePath, outputName, publicPath, mime, size, hash] = parts;
    map.set(normalizeAssetSource(sourcePath), { sourcePath, outputName, publicPath, mime, size: Number(size), hash });
  }
  return map;
}


const SCRIPT_FLAG = Object.freeze({
  INLINE: 1,
  MODULE: 2,
  DEFER: 4,
  ASYNC: 8,
  EXTERNAL_FILE: 16,
  REMOTE_URL: 32,
  BYTECODE_ENCODED: 64,
  DEPENDENCY: 256,
  DYNAMIC_IMPORT: 512,
  MODULE_PRELOAD: 1024,
  BROWSER: 2048,
});

function turboJsEnvelopeLaneMap(seed) {
  const map = new Uint8Array(16);
  for (let i = 0; i < map.length; ++i) map[i] = i;
  let state = (seed ^ 0xC6EF3720) >>> 0;
  for (let i = map.length - 1; i > 0; --i) {
    state ^= (state << 13) >>> 0;
    state ^= state >>> 17;
    state ^= (state << 5) >>> 0;
    state >>>= 0;
    const j = state % (i + 1);
    const tmp = map[i]; map[i] = map[j]; map[j] = tmp;
  }
  return map;
}

function turboJsEnvelopeLaneFingerprint(map) {
  let h = 2166136261 >>> 0;
  for (const value of map) { h ^= value; h = Math.imul(h, 16777619) >>> 0; }
  return h >>> 0;
}

function decodeTurboJsEnvelope(bytes, sourceName) {
  if (!bytes || bytes.length < 48) throw new Error('TurboJS bytecode envelope is truncated for ' + (sourceName || '<script>'));
  if (asciiOf(bytes.slice(0, 8)) !== 'VTJSE006') return bytes;
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const version = readU32(view, 8);
  const payloadSize = readU32(view, 12);
  const seed = readU32(view, 16) >>> 0;
  const bytecodeAbi = readU32(view, 20);
  const expectedHash = readHash64(view, 24);
  const payloadOffset = readU32(view, 32);
  const laneWidth = readU32(view, 40);
  const expectedLaneFingerprint = readU32(view, 44);
  if (version !== 2 || bytecodeAbi !== 0x01000300 || payloadOffset !== 48 || laneWidth !== 16 || payloadOffset + payloadSize !== bytes.length) {
    throw new Error('TurboJS bytecode envelope contract mismatch for ' + (sourceName || '<script>'));
  }
  const laneMap = turboJsEnvelopeLaneMap(seed);
  if (turboJsEnvelopeLaneFingerprint(laneMap) !== expectedLaneFingerprint) {
    throw new Error('TurboJS bytecode envelope lane map mismatch for ' + (sourceName || '<script>'));
  }
  const decoded = new Uint8Array(payloadSize);
  let stream = (seed ^ 0x9E3779B9) >>> 0;
  for (let block = 0; block < payloadSize; block += laneWidth) {
    const blockSize = Math.min(laneWidth, payloadSize - block);
    const blockMap = Array.from(laneMap).filter(lane => lane < blockSize);
    for (let storedLane = 0; storedLane < blockSize; ++storedLane) {
      if (((block + storedLane) & 3) === 0) {
        stream ^= (stream << 13) >>> 0;
        stream ^= stream >>> 17;
        stream ^= (stream << 5) >>> 0;
        stream >>>= 0;
      }
      const value = bytes[payloadOffset + block + storedLane] ^ ((stream >>> (((block + storedLane) & 3) * 8)) & 0xff);
      decoded[block + blockMap[storedLane]] = value;
    }
  }
  laneMap.fill(0);
  if (fnv1a64(decoded) !== expectedHash) {
    decoded.fill(0);
    throw new Error('TurboJS bytecode envelope integrity mismatch for ' + (sourceName || '<script>'));
  }
  return decoded;
}

function validateTurboJsBytecodeRecord(bytes, sourceName) {
  if (!bytes || bytes.length < 48) throw new Error('TurboJS bytecode record is truncated for ' + (sourceName || '<script>'));
  const magic = asciiOf(bytes.slice(0, 8));
  if (magic === 'VTJSBC01' || magic === 'VTJSBC02') throw new Error('source-preserving TurboJS bytecode record rejected for ' + (sourceName || '<script>'));
  if (magic !== 'VTJSBC03') throw new Error('unknown TurboJS bytecode record for ' + (sourceName || '<script>'));
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const version = readU32(view, 8);
  const bytecodeSize = readU32(view, 16);
  const payloadOffset = readU32(view, 44);
  if (version !== 3 || payloadOffset !== 48 || payloadOffset + bytecodeSize !== bytes.length) {
    throw new Error('TurboJS native bytecode record ranges are invalid for ' + (sourceName || '<script>'));
  }
  return true;
}

function parseJsBundle(section) {
  if (!section) return { chunks: [] };
  const data = section.data;
  requireAscii(data, 'VJSB0006', 'JS bundle');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const version = readU32(view, 8);
  const count = readU32(view, 12);
  const textSize = readU32(view, 16);
  if (version !== 1) throw new Error('unsupported JS bundle version: ' + version);

  const entrySize = 40;
  const entryBase = 24;
  const textBase = entryBase + count * entrySize;
  const codeBase = textBase + textSize;
  if (textBase > data.length || codeBase > data.length) throw new Error('JS bundle ranges are invalid');

  const chunks = [];
  for (let i = 0; i < count; ++i) {
    const base = entryBase + i * entrySize;
    const routeOffset = readU32(view, base + 0);
    const routeSize = readU32(view, base + 4);
    const sourceOffset = readU32(view, base + 8);
    const sourceSize = readU32(view, base + 12);
    const codeOffset = readU32(view, base + 16);
    const codeSize = readU32(view, base + 20);
    const order = readU32(view, base + 24);
    const flags = readU32(view, base + 28);
    const kind = readU32(view, base + 32);

    const routeEnd = textBase + routeOffset + routeSize;
    const sourceEnd = textBase + sourceOffset + sourceSize;
    const codeEnd = codeBase + codeOffset + codeSize;
    if (routeEnd > codeBase || sourceEnd > codeBase || codeEnd > data.length) {
      throw new Error('JS chunk range is invalid');
    }

    const route = textDecoder.decode(data.slice(textBase + routeOffset, routeEnd));
    const source = textDecoder.decode(data.slice(textBase + sourceOffset, sourceEnd));
    const storedCodeBytes = data.slice(codeBase + codeOffset, codeEnd);
    const storedMagic = asciiOf(storedCodeBytes.slice(0, 8));
    const bytecodeEncoded = (flags & SCRIPT_FLAG.BYTECODE_ENCODED) !== 0 || storedMagic === 'VTJSE006' || storedMagic === 'VTJSBC03' || storedMagic === 'VTJSMB04';
    const codeBytes = bytecodeEncoded ? decodeTurboJsEnvelope(storedCodeBytes, source) : storedCodeBytes;
    if (storedMagic === 'VTJSE006') storedCodeBytes.fill(0);
    const bytecodeMagic = asciiOf(codeBytes.slice(0, 8));
    const browserSide = (flags & SCRIPT_FLAG.BROWSER) !== 0;
    if (browserSide && bytecodeEncoded) {
      throw new Error('browser script chunk cannot contain TurboJS bytecode: ' + (source || '<script>'));
    }
    if (bytecodeEncoded) {
      if (bytecodeMagic === 'VTJSBC03') validateTurboJsBytecodeRecord(codeBytes, source);
      else if (bytecodeMagic !== 'VTJSMB04') throw new Error('unknown protected TurboJS payload for ' + (source || '<script>'));
    }
    const bytecodeTrustHandoff = bytecodeEncoded ? (() => {
      const byteHash = fnv1a32(codeBytes) >>> 0;
      const bindingText = String(route || '') + '\n' + String(source || '') + '\n' + String(order >>> 0) + '\n' + String(codeBytes.length >>> 0) + '\n' + String(byteHash);
      return Object.freeze({
        version: 1,
        producer: 'package-decoder-wasm',
        consumer: 'turbojs-execution-wasm',
        byteLength: codeBytes.length >>> 0,
        byteHash,
        bindingHash: fnv1a32(textEncoder.encode(bindingText)) >>> 0,
      });
    })() : null;
    chunks.push({
      route,
      source,
      code: bytecodeEncoded ? '' : textDecoder.decode(codeBytes),
      bytecodeBytes: bytecodeEncoded ? codeBytes : null,
      bytecodeTrustHandoff,
      bytecode: bytecodeEncoded,
      bytecodeSize: codeBytes.length,
      bytecodeOpaque: bytecodeEncoded,
      order,
      flags,
      kind,
    });
  }

  chunks.sort((a, b) => a.order - b.order);
  return { chunks };
}



function parseLazySectionsMetadata(section) {
  const empty = Object.freeze({ enabled: false, routes: new Map(), scripts: new Map(), routeCount: 0, scriptRouteCount: 0 });
  if (!section) return empty;
  const lines = textDecoder.decode(section.data).split(/\r?\n/).filter((line) => line.length > 0);
  if (lines[0] !== 'VENOM_LAZY_SECTIONS_V1') throw new Error('invalid lazy section metadata header');
  let enabled = false;
  let routeCount = 0;
  let scriptRouteCount = 0;
  const routes = new Map();
  const scripts = new Map();
  for (const line of lines.slice(1)) {
    if (line.startsWith('enabled=')) { enabled = line.slice('enabled='.length) === 'true'; continue; }
    if (line.startsWith('route_count=')) { routeCount = Number.parseInt(line.slice('route_count='.length), 10) >>> 0; continue; }
    if (line.startsWith('script_route_count=')) { scriptRouteCount = Number.parseInt(line.slice('script_route_count='.length), 10) >>> 0; continue; }
    if (line.startsWith('route\t')) {
      const parts = line.split('\t');
      if (parts.length !== 5) throw new Error('invalid lazy route entry');
      routes.set(parts[1], Object.freeze({ route: parts[1], section: parts[2], instructionCount: Number.parseInt(parts[3], 10) >>> 0, bytecodeSize: Number.parseInt(parts[4], 10) >>> 0 }));
      continue;
    }
    if (line.startsWith('script\t')) {
      const parts = line.split('\t');
      if (parts.length !== 4) throw new Error('invalid lazy script entry');
      scripts.set(parts[1], Object.freeze({ route: parts[1], section: parts[2], chunkCount: Number.parseInt(parts[3], 10) >>> 0 }));
    }
  }
  if (enabled && routes.size !== routeCount) throw new Error('lazy route metadata count mismatch');
  if (enabled && scripts.size !== scriptRouteCount) throw new Error('lazy script metadata count mismatch');
  return Object.freeze({ enabled, routes, scripts, routeCount, scriptRouteCount });
}

function routeWithLazyBytecode(route, vm) {
  return Object.freeze({
    route: route.route,
    sourcePath: route.sourcePath,
    bytecodeOffset: 0,
    bytecodeSize: Math.max(0, vm.data.length - vm.dataOffset),
    instructionCount: vm.instructionCount,
    flags: route.flags,
  });
}

function secureWipeView(view) {
  if (!view || typeof view.fill !== 'function') return;
  try { view.fill(0); } catch (_) {}
}

function cloneSectionForRoute(section) {
  if (!section) return null;
  const owned = new Uint8Array(section.data.length);
  owned.set(section.data);
  return Object.freeze({ ...section, data: owned, __venomOwnedData: owned });
}

function wipeRouteRuntime(runtime) {
  if (!runtime || runtime.disposed) return false;
  const buffers = runtime.ownedBuffers || [];
  for (const buffer of buffers) secureWipeView(buffer);
  if (runtime.jsBundle && Array.isArray(runtime.jsBundle.chunks)) {
    for (const chunk of runtime.jsBundle.chunks) {
      if (chunk && chunk.bytecodeBytes) secureWipeView(chunk.bytecodeBytes);
    }
  }
  runtime.disposed = true;
  return true;
}

function makeRouteRuntimeLoader(pkg, lazySectionsPlan, strictIsolation = false) {
  let activeRuntime = null;
  let routeGeneration = 0;
  if (strictIsolation && (!lazySectionsPlan || !lazySectionsPlan.enabled)) {
    throw new Error('protected release requires route-lazy sections');
  }
  function disposeActive() {
    if (!activeRuntime) return false;
    const disposed = wipeRouteRuntime(activeRuntime);
    activeRuntime = null;
    return disposed;
  }
  function loadRouteRuntime(route) {
    if (!route) throw new Error('no route available');
    if (activeRuntime && activeRuntime.route && activeRuntime.route.route === route.route && !activeRuntime.disposed) return activeRuntime;
    disposeActive();
    routeGeneration = (routeGeneration + 1) >>> 0;
    let runtime;
    if (lazySectionsPlan && lazySectionsPlan.enabled && lazySectionsPlan.routes.has(route.route)) {
      const lazyRoute = lazySectionsPlan.routes.get(route.route);
      const vmSection = cloneSectionForRoute(findSection(pkg, SECTION.VM_BYTECODE, lazyRoute.section));
      const vm = parseVmBytecode(vmSection);
      const scriptInfo = lazySectionsPlan.scripts.get(route.route);
      const jsSection = scriptInfo ? cloneSectionForRoute(findSection(pkg, SECTION.JAVASCRIPT, scriptInfo.section)) : null;
      const parsedBundle = jsSection ? parseJsBundle(jsSection) : { chunks: [] };
      const jsBundle = Object.freeze({ chunks: parsedBundle.chunks.map((chunk) => Object.freeze({ ...chunk, routeGeneration })) });
      runtime = { route: routeWithLazyBytecode(route, vm), vm, jsBundle, lazy: true, routeGeneration, ownedBuffers: [vmSection.__venomOwnedData].concat(jsSection ? [jsSection.__venomOwnedData] : []), disposed: false };
    } else {
      const vm = parseVmBytecode(findSection(pkg, SECTION.VM_BYTECODE, 'route-bytecode.vmbc'));
      const jsBundle = parseJsBundle(findOptionalSection(pkg, SECTION.JAVASCRIPT, 'scripts.vjsb'));
      runtime = { route, vm, jsBundle, lazy: false, routeGeneration, ownedBuffers: [], disposed: false };
    }
    activeRuntime = runtime;
    return runtime;
  }
  loadRouteRuntime.dispose = disposeActive;
  loadRouteRuntime.generation = () => routeGeneration;
  return Object.freeze(loadRouteRuntime);
}

function parseHostBridgeMetadata(section) {
  const plan = {
    version: 0,
    hostCalls: new Map(),
    eventAttributePrefix: 'on',
    inlineEventBindings: false,
    wasmDomOp: 'bindEvent',
    fetchBridge: 'planned',
    diagnostics: new Map(),
    capabilitiesById: new Map(),
    capabilitiesByName: new Map(),
  };
  if (!section) return Object.freeze(plan);
  const lines = textDecoder.decode(section.data).split(/\r?\n/).filter((line) => line.length > 0);
  if (!['VENOM_HOST_BRIDGE_V14', 'VENOM_HOST_BRIDGE_V13', 'VENOM_HOST_BRIDGE_V12', 'VENOM_HOST_BRIDGE_V11', 'VENOM_HOST_BRIDGE_V10', 'VENOM_HOST_BRIDGE_V9', 'VENOM_HOST_BRIDGE_V8', 'VENOM_HOST_BRIDGE_V7', 'VENOM_HOST_BRIDGE_V6', 'VENOM_HOST_BRIDGE_V5', 'VENOM_HOST_BRIDGE_V4', 'VENOM_HOST_BRIDGE_V3', 'VENOM_HOST_BRIDGE_V2', 'VENOM_HOST_BRIDGE_V1'].includes(lines[0])) throw new Error('invalid host bridge metadata header');
  for (const line of lines.slice(1)) {
    if (line.startsWith('host_call\t')) {
      const parts = line.split('\t');
      if (parts.length >= 4) {
        const id = Number.parseInt(parts[1], 10) >>> 0;
        plan.hostCalls.set(parts[2], Object.freeze({ id, name: parts[2], surface: parts[3], mode: parts[4] || 'runtime' }));
      }
      continue;
    }
    if (line.startsWith('capability\t')) {
      const parts = line.split('\t');
      if (parts.length !== 7) throw new Error('invalid host capability record');
      const id = Number.parseInt(parts[1], 10);
      const name = parts[2];
      const maxRequestBytes = Number.parseInt(parts[5], 10);
      const maxResponseBytes = Number.parseInt(parts[6], 10);
      if (!Number.isSafeInteger(id) || id <= 0 || id > 65535 || !name || !Number.isSafeInteger(maxRequestBytes) || maxRequestBytes < 0 || !Number.isSafeInteger(maxResponseBytes) || maxResponseBytes < 0) throw new Error('invalid host capability bounds');
      if (plan.capabilitiesById.has(id) || plan.capabilitiesByName.has(name)) throw new Error('duplicate host capability');
      const capability = Object.freeze({ id, name, requestSchema: parts[3], responseSchema: parts[4], maxRequestBytes, maxResponseBytes });
      plan.capabilitiesById.set(id, capability);
      plan.capabilitiesByName.set(name, capability);
      continue;
    }
    const idx = line.indexOf('=');
    if (idx <= 0) continue;
    const key = line.slice(0, idx);
    const value = line.slice(idx + 1);
    if (key === 'version') plan.version = Number.parseInt(value, 10) >>> 0;
    else if (key === 'event_attribute_prefix') plan.eventAttributePrefix = value || 'on';
    else if (key === 'inline_event_bindings') plan.inlineEventBindings = value === 'true';
    else if (key === 'wasm_dom_op') plan.wasmDomOp = value || 'bindEvent';
    else if (key === 'fetch_bridge') plan.fetchBridge = value || 'planned';
    else if (key === 'async_host_queue') plan.asyncHostQueue = value || 'planned';
    else if (key === 'timer_bridge') plan.timerBridge = value || 'planned';
    else if (key === 'event_queue') plan.eventQueue = value || 'planned';
    else if (key === 'turbojs_bridge') plan.turboJsBridge = value || 'planned';
    else plan.diagnostics.set(key, value);
  }
  return Object.freeze({
    version: plan.version,
    hostCalls: plan.hostCalls,
    eventAttributePrefix: plan.eventAttributePrefix,
    inlineEventBindings: plan.inlineEventBindings,
    wasmDomOp: plan.wasmDomOp,
    fetchBridge: plan.fetchBridge,
    asyncHostQueue: plan.asyncHostQueue || 'planned',
    timerBridge: plan.timerBridge || 'planned',
    eventQueue: plan.eventQueue || 'planned',
    turboJsBridge: plan.turboJsBridge || 'planned',
    diagnostics: plan.diagnostics,
    capabilitiesById: plan.capabilitiesById,
    capabilitiesByName: plan.capabilitiesByName,
    strictCapabilities: plan.diagnostics.get('capability_model') === 'numeric-fixed-schema-v1',
    unknownHostCall: plan.diagnostics.get('unknown_host_call') || 'deny',
    maxRequestBytes: Number.parseInt(plan.diagnostics.get('max_request_bytes') || '65536', 10) >>> 0,
    maxResponseBytes: Number.parseInt(plan.diagnostics.get('max_response_bytes') || '1048576', 10) >>> 0,
  });
}



function normalizeHostUrl(input, baseUrl = (globalThis.location ? globalThis.location.href : 'http://localhost/')) {
  const raw = String(input == null ? '' : input).trim();
  if (!raw || raw.length > 8192) throw new Error('host URL rejected');
  let url;
  try { url = new URL(raw, baseUrl); } catch (_) { throw new Error('host URL rejected'); }
  const protocol = String(url.protocol || '').toLowerCase();
  if (protocol !== 'http:' && protocol !== 'https:') throw new Error('host URL scheme denied');
  url.username = '';
  url.password = '';
  return url;
}

function authorizeHostUrl(input, options = {}) {
  const url = normalizeHostUrl(input, options.baseUrl);
  const sameOriginOnly = options.sameOriginOnly !== false;
  const currentOrigin = globalThis.location && globalThis.location.origin ? String(globalThis.location.origin) : url.origin;
  if (sameOriginOnly && url.origin !== currentOrigin) throw new Error('cross-origin host request denied');
  return url;
}

function isUnsafeDomAttribute(name, value) {
  const attr = String(name || '').trim().toLowerCase();
  const text = String(value == null ? '' : value).trim();
  if (!attr || attr.length > 128 || text.length > 1048576) return true;
  if (attr.startsWith('on')) return true;
  if (attr === 'srcdoc') return true;
  if (attr === 'style' && /(?:url\s*\(|expression\s*\(|-moz-binding)/i.test(text)) return true;
  if (attr === 'href' || attr === 'src' || attr === 'action' || attr === 'formaction' || attr === 'poster' || attr === 'xlink:href') {
    const compact = text.replace(/[\u0000-\u0020]+/g, '').toLowerCase();
    if (compact.startsWith('javascript:') || compact.startsWith('vbscript:') || compact.startsWith('data:text/html')) return true;
  }
  return false;
}

function createDomHandleRegistry(root, maxHandles = 8192) {
  let generation = 1;
  let nextSlot = 1;
  const entries = new Map();
  const secret = (() => {
    const words = new Uint32Array(1);
    if (!globalThis.crypto || typeof globalThis.crypto.getRandomValues !== 'function') throw new Error('secure DOM handle entropy unavailable');
    globalThis.crypto.getRandomValues(words);
    return (words[0] || 0x9e3779b9) >>> 0;
  })();
  function tag(payload) {
    let x = (payload ^ secret) >>> 0;
    x ^= x >>> 16; x = Math.imul(x, 0x7feb352d) >>> 0;
    x ^= x >>> 15; x = Math.imul(x, 0x846ca68b) >>> 0;
    x ^= x >>> 16;
    return x & 0x0f;
  }
  function encode(slot) {
    const payload = ((((generation & 0x0fff) << 16) | (slot & 0x0000ffff)) >>> 0);
    return (((payload << 4) | tag(payload)) >>> 0);
  }
  function decode(handle) {
    const value = handle >>> 0;
    const payload = value >>> 4;
    if ((value & 0x0f) !== tag(payload)) throw new Error('invalid DOM handle authentication tag');
    return Object.freeze({ generation: (payload >>> 16) & 0x0fff, slot: payload & 0x0000ffff });
  }
  function register(node) {
    if (!node || typeof node !== 'object') throw new Error('invalid DOM handle target');
    if (entries.size >= maxHandles) throw new Error('DOM handle capacity exceeded');
    let slot = nextSlot++ & 0x0000ffff;
    if (slot === 0) slot = nextSlot++ & 0x0000ffff;
    const handle = encode(slot);
    entries.set(slot, Object.freeze({ node, generation }));
    return handle;
  }
  function resolve(handle) {
    if (!Number.isInteger(handle) || handle <= 0) throw new Error('invalid DOM handle');
    const decoded = decode(handle >>> 0);
    if (decoded.generation !== generation) throw new Error('stale DOM handle');
    const entry = entries.get(decoded.slot);
    if (!entry || entry.generation !== generation) throw new Error('unknown DOM handle');
    if (root && entry.node !== root && typeof root.contains === 'function' && !root.contains(entry.node)) throw new Error('DOM handle ownership denied');
    return entry.node;
  }
  function revoke(handle) {
    const decoded = decode(handle >>> 0);
    if (decoded.generation !== generation) return false;
    return entries.delete(decoded.slot);
  }
  function nextGeneration() {
    entries.clear();
    generation = (generation + 1) & 0x0fff;
    if (generation === 0) generation = 1;
    nextSlot = 1;
    return generation;
  }
  let currentRootHandle = register(root);
  function advanceGeneration() {
    nextGeneration();
    currentRootHandle = register(root);
    return generation;
  }
  return Object.freeze({ register, resolve, revoke, nextGeneration: advanceGeneration, rootHandle: () => currentRootHandle, snapshot: () => Object.freeze({ generation, size: entries.size, maxHandles, rootHandle: currentRootHandle }) });
}

function authorizeHostCapability(hostBridgePlan, id, requestBytes, responseBytes = 0) {
  if (!hostBridgePlan || !hostBridgePlan.strictCapabilities) return true;
  if (!Number.isInteger(id) || id <= 0) throw new Error('invalid host capability id');
  const capability = hostBridgePlan.capabilitiesById.get(id);
  if (!capability) throw new Error('host capability denied');
  const requestSize = Number(requestBytes) >>> 0;
  const responseSize = Number(responseBytes) >>> 0;
  if (requestSize > capability.maxRequestBytes || requestSize > hostBridgePlan.maxRequestBytes) throw new Error('host capability request limit exceeded');
  if (responseSize > capability.maxResponseBytes || responseSize > hostBridgePlan.maxResponseBytes) throw new Error('host capability response limit exceeded');
  return true;
}

function parseKeyValueMetadata(section, expectedMagic) {
  const plan = { version: 0, enabled: false, values: new Map(), lines: [] };
  if (!section) return Object.freeze(plan);
  const lines = textDecoder.decode(section.data).split(/\r?\n/).filter((line) => line.length > 0);
  const allowedMagic = Array.isArray(expectedMagic) ? expectedMagic : [expectedMagic];
  if (!allowedMagic.includes(lines[0])) throw new Error('invalid metadata header for ' + allowedMagic.join('|'));
  for (const line of lines.slice(1)) {
    const idx = line.indexOf('=');
    if (idx <= 0) {
      plan.lines.push(line);
      continue;
    }
    const key = line.slice(0, idx);
    const value = line.slice(idx + 1);
    plan.values.set(key, value);
    if (key === 'version') plan.version = Number.parseInt(value, 10) >>> 0;
    if (key === 'enabled') plan.enabled = value === 'true';
  }
  return Object.freeze({ version: plan.version, enabled: plan.enabled, values: plan.values, lines: Object.freeze(plan.lines) });
}

function parseFetchBridgeMetadata(section) {
  const plan = parseKeyValueMetadata(section, 'VENOM_FETCH_BRIDGE_V1');
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    requestHostCall: plan.values.get('request_host_call') || 'fetch.request',
    responseHostCall: plan.values.get('response_host_call') || 'fetch.response',
    errorHostCall: plan.values.get('error_host_call') || 'fetch.error',
    queue: plan.values.get('queue') || 'async-host-queue.vahq',
    cachePolicy: plan.values.get('cache_policy') || 'browser-default',
    credentialsPolicy: plan.values.get('credentials_policy') || 'same-origin',
    bodyBridge: plan.values.get('body_bridge') || 'uint8array',
    textResponse: plan.values.get('text_response') !== 'false',
    jsonResponse: plan.values.get('json_response') !== 'false',
  });
}

function parseTimerBridgeMetadata(section) {
  const plan = parseKeyValueMetadata(section, 'VENOM_TIMER_BRIDGE_V1');
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    scheduleHostCall: plan.values.get('schedule_host_call') || 'timer.schedule',
    cancelHostCall: plan.values.get('cancel_host_call') || 'timer.cancel',
    queue: plan.values.get('queue') || 'async-host-queue.vahq',
    clock: plan.values.get('clock') || 'browser-event-loop',
    timerIdBits: Number.parseInt(plan.values.get('timer_id_bits') || '32', 10) >>> 0,
    maxActive: Number.parseInt(plan.values.get('max_active') || '128', 10) >>> 0,
    maxScheduledPerRoute: Number.parseInt(plan.values.get('max_scheduled_per_route') || '512', 10) >>> 0,
    minimumDelayMs: Number.parseInt(plan.values.get('minimum_delay_ms') || '0', 10) >>> 0,
    wasmResponseBoundary: plan.values.get('wasm_response_boundary') || 'planned',
    turbojsTimerBoundary: plan.values.get('turbojs_timer_boundary') || 'planned',
  });
}

function parseEventQueueMetadata(section) {
  const plan = parseKeyValueMetadata(section, 'VENOM_EVENT_QUEUE_V1');
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    queueHostCall: plan.values.get('queue_host_call') || 'event.queue',
    dispatchHostCall: plan.values.get('dispatch_host_call') || 'event.dispatch',
    inlineEventBindings: plan.values.get('inline_event_bindings') === 'true',
    maxRecords: Number.parseInt(plan.values.get('max_records') || '256', 10) >>> 0,
    maxDispatchesPerRoute: Number.parseInt(plan.values.get('max_dispatches_per_route') || '1024', 10) >>> 0,
    recordPayload: plan.values.get('record_payload') || 'eventName|attrName|route|targetTag',
    wasmEventBoundary: plan.values.get('wasm_event_boundary') || 'planned',
    turbojsEventBoundary: plan.values.get('turbojs_event_boundary') || 'planned',
  });
}

function parseTurboJsBridgeMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_BRIDGE_V9', 'VENOM_TURBOJS_BRIDGE_V8', 'VENOM_TURBOJS_BRIDGE_V7', 'VENOM_TURBOJS_BRIDGE_V6', 'VENOM_TURBOJS_BRIDGE_V5', 'VENOM_TURBOJS_BRIDGE_V4', 'VENOM_TURBOJS_BRIDGE_V3', 'VENOM_TURBOJS_BRIDGE_V2', 'VENOM_TURBOJS_BRIDGE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    mode: plan.values.get('mode') || 'planned-boundary',
    callHostCall: plan.values.get('call_host_call') || 'turbojs.call',
    resultHostCall: plan.values.get('result_host_call') || 'turbojs.result',
    queue: plan.values.get('queue') || 'async-host-queue.vahq',
    scriptIsolation: plan.values.get('script_isolation') || 'route-scoped',
    bytecodeInput: plan.values.get('bytecode_input') || 'planned',
    wasmEngine: plan.values.get('wasm_engine') || 'planned',
    nativeEngine: plan.values.get('native_engine') || 'compile-time-probe',
    chunkMetadata: plan.values.get('chunk_metadata') || '',
    capabilityPolicy: plan.values.get('capability_policy') || '',
    requestRecord: plan.values.get('request_record') || 'turbojs.call',
    resultRecord: plan.values.get('result_record') || 'turbojs.result',
    engineMetadata: plan.values.get('engine_metadata') || '',
    scriptEnginePolicy: plan.values.get('script_engine_policy') || '',
    fallbackPolicy: plan.values.get('fallback_policy') || 'host-js-isolated-wrapper',
    contextLifecycle: plan.values.get('context_lifecycle') || '',
    hostCapabilities: plan.values.get('host_capabilities') || '',
    adapterDiagnostics: plan.values.get('adapter_diagnostics') || '',
  });
}

function parseScriptIsolationMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_SCRIPT_ISOLATION_V4', 'VENOM_SCRIPT_ISOLATION_V3', 'VENOM_SCRIPT_ISOLATION_V2', 'VENOM_SCRIPT_ISOLATION_V1']);
  return Object.freeze({
    version: plan.version,
    mode: plan.values.get('mode') || 'none',
    scopeKey: plan.values.get('scope_key') || 'route|source|order',
    globalPolicy: plan.values.get('global_policy') || 'shared-browser-global-with-route-wrapper',
    documentAccess: plan.values.get('document_access') || 'bridge-parameter',
    windowAccess: plan.values.get('window_access') || 'bridge-parameter',
    turboJsBoundary: plan.values.get('turbojs_boundary') || 'planned',
    engineContext: plan.values.get('engine_context') || 'planned',
    engineFallback: plan.values.get('engine_fallback') || 'none',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseScriptPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_SCRIPT_POLICY_V4', 'VENOM_SCRIPT_POLICY_V3', 'VENOM_SCRIPT_POLICY_V2', 'VENOM_SCRIPT_POLICY_V1']);
  return Object.freeze({
    version: plan.version,
    defaultCapabilities: plan.values.get('default_capabilities') || '',
    remoteScripts: plan.values.get('remote_scripts') || 'metadata-only',
    moduleScripts: plan.values.get('module_scripts') || 'blob-loader-until-turbojs-wasm',
    inlineScripts: plan.values.get('inline_scripts') || 'route-wrapper',
    policyCheckHostCall: plan.values.get('policy_check_host_call') || 'script.policyCheck',
    chunkStartHostCall: plan.values.get('chunk_start_host_call') || 'script.chunkStart',
    chunkFinishHostCall: plan.values.get('chunk_finish_host_call') || 'script.chunkFinish',
    engineFallback: plan.values.get('engine_fallback') || 'none',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseTurboJsChunkMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_CHUNKS_V7', 'VENOM_TURBOJS_CHUNKS_V6', 'VENOM_TURBOJS_CHUNKS_V5', 'VENOM_TURBOJS_CHUNKS_V4', 'VENOM_TURBOJS_CHUNKS_V3', 'VENOM_TURBOJS_CHUNKS_V2', 'VENOM_TURBOJS_CHUNKS_V1']);
  return Object.freeze({
    version: plan.version,
    mode: plan.values.get('mode') || 'none',
    bytecodeProvider: plan.values.get('bytecode_provider') || 'planned',
    sourceChunkSection: plan.values.get('source_chunk_section') || 'scripts.vjsb',
    requestHostCall: plan.values.get('request_host_call') || 'turbojs.chunk',
    bytecodeBoundaryHostCall: plan.values.get('bytecode_boundary_host_call') || 'turbojs.bytecodeBoundary',
    engineExecuteHostCall: plan.values.get('engine_execute_host_call') || 'turbojs.executeChunk',
    engineFallback: plan.values.get('engine_fallback') || 'none',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}


function parseTurboJsEngineMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_ENGINE_V7', 'VENOM_TURBOJS_ENGINE_V6', 'VENOM_TURBOJS_ENGINE_V5', 'VENOM_TURBOJS_ENGINE_V4', 'VENOM_TURBOJS_ENGINE_V3', 'VENOM_TURBOJS_ENGINE_V2', 'VENOM_TURBOJS_ENGINE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    mode: plan.values.get('mode') || 'none',
    engine: plan.values.get('engine') || 'none',
    browserEngine: plan.values.get('browser_engine') || 'none',
    nativeEngine: plan.values.get('native_engine') || 'none',
    contextModel: plan.values.get('context_model') || 'none',
    sourceChunks: plan.values.get('source_chunks') || '',
    capabilityPolicy: plan.values.get('capability_policy') || '',
    contextCreateHostCall: plan.values.get('context_create_host_call') || 'turbojs.contextCreate',
    executeHostCall: plan.values.get('execute_host_call') || 'turbojs.executeChunk',
    resultHostCall: plan.values.get('result_host_call') || 'turbojs.executionResult',
    fallbackHostCall: plan.values.get('fallback_host_call') || 'script.engineFallback',
    wasmRuntimeAsset: plan.values.get('wasm_runtime_asset') || '',
    sourceTransfer: plan.values.get('source_transfer') || '',
    consoleBridge: plan.values.get('console_bridge') || '',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseScriptEnginePolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_SCRIPT_ENGINE_POLICY_V4', 'VENOM_SCRIPT_ENGINE_POLICY_V3', 'VENOM_SCRIPT_ENGINE_POLICY_V2', 'VENOM_SCRIPT_ENGINE_POLICY_V1']);
  const bool = (key, fallback = false) => plan.values.has(key) ? plan.values.get(key) === 'true' : fallback;
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    fallback: plan.values.get('fallback') || 'host-js-isolated-wrapper',
    allowEval: bool('allow_eval', false),
    allowFunctionConstructor: bool('allow_function_constructor', true),
    capabilities: plan.values.get('capabilities') || '',
    bindConsole: bool('bind_console', true),
    bindDocument: bool('bind_document', true),
    bindFetch: bool('bind_fetch', true),
    bindTimers: bool('bind_timers', true),
    bindEvents: bool('bind_events', true),
    routeScope: bool('route_scope', true),
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseTurboJsEngineModuleMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_ENGINE_MODULE_V7', 'VENOM_TURBOJS_ENGINE_MODULE_V6', 'VENOM_TURBOJS_ENGINE_MODULE_V5', 'VENOM_TURBOJS_ENGINE_MODULE_V4', 'VENOM_TURBOJS_ENGINE_MODULE_V3', 'VENOM_TURBOJS_ENGINE_MODULE_V2', 'VENOM_TURBOJS_ENGINE_MODULE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    assetName: plan.values.get('asset_name') || '',
    moduleFormat: plan.values.get('module_format') || 'esm',
    loader: plan.values.get('loader') || 'dynamic-import',
    exportName: plan.values.get('export') || 'createVenomTurboJsEngineModule',
    executionMode: plan.values.get('execution_mode') || 'route-scoped-module',
    fallback: plan.values.get('fallback') || 'host-js-isolated-wrapper',
    contextModel: plan.values.get('context_model') || 'route-scoped',
    contextApi: plan.values.get('context_api') || 'createContext|destroyContext|contextSnapshot',
    hostCapabilities: plan.values.get('host_capabilities') || 'injectable-bindings',
    wasmAssetName: plan.values.get('wasm_asset_name') || '',
    wasmLoader: plan.values.get('wasm_loader') || 'fetch-instantiate',
    sourceTransferApi: plan.values.get('source_transfer_api') || '',
    consoleBridge: plan.values.get('console_bridge') || '',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseTurboJsContextLifecycleMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_CONTEXT_LIFECYCLE_V3', 'VENOM_TURBOJS_CONTEXT_LIFECYCLE_V2', 'VENOM_TURBOJS_CONTEXT_LIFECYCLE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    model: plan.values.get('model') || 'route-scoped-reusable',
    key: plan.values.get('key') || 'route|source|order',
    createHostCall: plan.values.get('create_host_call') || 'turbojs.contextCreate',
    reuseHostCall: plan.values.get('reuse_host_call') || 'turbojs.contextReuse',
    destroyHostCall: plan.values.get('destroy_host_call') || 'turbojs.contextDestroy',
    moduleCreateHostCall: plan.values.get('module_create_host_call') || 'turbojs.moduleContextCreate',
    moduleDestroyHostCall: plan.values.get('module_destroy_host_call') || 'turbojs.moduleContextDestroy',
    snapshot: plan.values.get('snapshot') !== 'false',
    maxContexts: Number.parseInt(plan.values.get('max_contexts') || '4096', 10) >>> 0,
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseHostCapabilitiesMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_HOST_CAPABILITIES_V3', 'VENOM_HOST_CAPABILITIES_V2', 'VENOM_HOST_CAPABILITIES_V1']);
  const capabilities = [];
  const chunkCapabilities = [];
  if (section) {
    for (const line of textDecoder.decode(section.data).split(/\r?\n/)) {
      if (line.startsWith('capability\t')) {
        const parts = line.split('\t');
        if (parts.length >= 3) capabilities.push(Object.freeze({ name: parts[1], mode: parts[2] }));
      } else if (line.startsWith('chunk_capabilities\t')) {
        const parts = line.split('\t');
        if (parts.length >= 5) {
          chunkCapabilities.push(Object.freeze({
            order: Number.parseInt(parts[1] || '0', 10) >>> 0,
            route: parts[2] || '',
            source: parts[3] || '',
            capabilities: Object.freeze((parts[4] || '').split(',').filter(Boolean)),
          }));
        }
      }
    }
  }
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    policy: plan.values.get('policy') || 'legacy-global',
    undeclaredCapability: plan.values.get('undeclared_capability') || 'allow',
    injectHostCall: plan.values.get('inject_host_call') || 'turbojs.hostCapabilityInject',
    defaultCapabilityCount: Number.parseInt(plan.values.get('default_capability_count') || String(capabilities.length), 10) >>> 0,
    defaultCapabilities: Object.freeze((plan.values.get('default_capabilities') || '').split(',').filter(Boolean)),
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
    capabilities: Object.freeze(capabilities),
    chunkCapabilities: Object.freeze(chunkCapabilities),
  });
}

function parseTurboJsAdapterDiagnosticsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_ADAPTER_DIAGNOSTICS_V3', 'VENOM_TURBOJS_ADAPTER_DIAGNOSTICS_V2', 'VENOM_TURBOJS_ADAPTER_DIAGNOSTICS_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    engineAsset: plan.values.get('engine_asset') || '',
    adapterContract: plan.values.get('adapter_contract') || '',
    fallback: plan.values.get('fallback') || 'host-js-isolated-wrapper',
    records: plan.values.get('records') || '',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseTurboJsWasmRuntimeMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_WASM_RUNTIME_V10', 'VENOM_TURBOJS_WASM_RUNTIME_V9', 'VENOM_TURBOJS_WASM_RUNTIME_V8', 'VENOM_TURBOJS_WASM_RUNTIME_V7', 'VENOM_TURBOJS_WASM_RUNTIME_V6', 'VENOM_TURBOJS_WASM_RUNTIME_V5', 'VENOM_TURBOJS_WASM_RUNTIME_V4', 'VENOM_TURBOJS_WASM_RUNTIME_V3', 'VENOM_TURBOJS_WASM_RUNTIME_V2', 'VENOM_TURBOJS_WASM_RUNTIME_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    assetName: plan.values.get('asset_name') || '',
    abi: Number.parseInt(plan.values.get('abi') || '12', 10) >>> 0,
    packageVersion: Number.parseInt(plan.values.get('package_version') || '50', 10) >>> 0,
    exports: plan.values.get('exports') || '',
    executionMode: plan.values.get('execution_mode') || 'turbojs-wasm-abi12-upstream-global-host-api-shims',
    abiContract: plan.values.get('abi_contract') || '',
    statusExports: plan.values.get('status_exports') || '',
    limitExports: plan.values.get('limit_exports') || '',
    bytecodeValidationExports: plan.values.get('bytecode_validation_exports') || '',
  });
}

function parseTurboJsSourceTransferMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_SOURCE_TRANSFER_V7', 'VENOM_TURBOJS_SOURCE_TRANSFER_V6', 'VENOM_TURBOJS_SOURCE_TRANSFER_V5', 'VENOM_TURBOJS_SOURCE_TRANSFER_V4', 'VENOM_TURBOJS_SOURCE_TRANSFER_V3', 'VENOM_TURBOJS_SOURCE_TRANSFER_V2', 'VENOM_TURBOJS_SOURCE_TRANSFER_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    sourcePtr: plan.values.get('source_ptr') || 'venom_tjs_source_ptr',
    sourceCapacity: plan.values.get('source_capacity') || 'venom_tjs_source_capacity',
    executeSource: plan.values.get('execute_source') || 'venom_tjs_execute_source',
    executeBytecode: plan.values.get('execute_bytecode') || 'venom_tjs_execute_bytecode',
    validateBytecode: plan.values.get('validate_bytecode') || 'venom_tjs_bytecode_validate',
    bytecodeStatus: plan.values.get('bytecode_status') || 'venom_tjs_status_code',
    bytecodeRecordHash: plan.values.get('bytecode_record_hash') || 'venom_tjs_bytecode_record_hash32',
    bytecodePayloadSize: plan.values.get('bytecode_payload_size') || 'venom_tjs_bytecode_payload_size',
    transferMode: plan.values.get('transfer_mode') || 'utf8-source',
    resultPtr: plan.values.get('result_ptr') || 'venom_tjs_result_ptr',
    resultSize: plan.values.get('result_size') || 'venom_tjs_result_size',
  });
}

function parseTurboJsConsoleBridgeMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_CONSOLE_BRIDGE_V4', 'VENOM_TURBOJS_CONSOLE_BRIDGE_V3', 'VENOM_TURBOJS_CONSOLE_BRIDGE_V2', 'VENOM_TURBOJS_CONSOLE_BRIDGE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    mode: plan.values.get('mode') || 'host-console-forward',
    hostCall: plan.values.get('host_call') || 'turbojs.console',
  });
}


function parseTurboJsExecutionRecordsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_EXECUTION_RECORDS_V3', 'VENOM_TURBOJS_EXECUTION_RECORDS_V2', 'VENOM_TURBOJS_EXECUTION_RECORDS_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    recordHostCall: plan.values.get('record_host_call') || 'turbojs.executionRecord',
    snapshotHostCall: plan.values.get('snapshot_host_call') || 'turbojs.executionSnapshot',
    recordFields: plan.values.get('record_fields') || '',
    retention: plan.values.get('retention') || 'runtime-session',
    chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0,
  });
}

function parseTurboJsResultBridgeMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_RESULT_BRIDGE_V3', 'VENOM_TURBOJS_RESULT_BRIDGE_V2', 'VENOM_TURBOJS_RESULT_BRIDGE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    resultHostCall: plan.values.get('result_host_call') || 'turbojs.resultBridge',
    wasmDecodeHostCall: plan.values.get('wasm_decode_host_call') || 'turbojs.wasmResultDecode',
    consoleEventHostCall: plan.values.get('console_event_host_call') || 'turbojs.consoleEvent',
    consoleFlushHostCall: plan.values.get('console_flush_host_call') || 'turbojs.consoleFlush',
    format: plan.values.get('format') || 'json-record-v1',
    fields: plan.values.get('fields') || '',
  });
}

function parseTurboJsFallbackPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TURBOJS_FALLBACK_POLICY_V3', 'VENOM_TURBOJS_FALLBACK_POLICY_V2', 'VENOM_TURBOJS_FALLBACK_POLICY_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    mode: plan.values.get('mode') || 'explicit-policy-gated',
    decisionHostCall: plan.values.get('decision_host_call') || 'script.fallbackDecision',
    policyCheckHostCall: plan.values.get('policy_check_host_call') || 'turbojs.fallbackPolicyCheck',
    allowWhen: plan.values.get('allow_when') || '',
    denyWhen: plan.values.get('deny_when') || '',
    currentReleasePolicy: plan.values.get('current_release_policy') || 'allow-compatible-fallback-with-record',
    strictRelease: plan.values.get('strict_release') === 'true' || (plan.values.get('current_release_policy') || '').startsWith('deny-host-fallback'),
    auditRecords: plan.values.get('audit_records') || '',
  });
}

function parseTurboJsRuntimeAbiMetadata(section) {
  const plan = parseKeyValueMetadata(section, [['VENOM','TJS','RUNTIME','ABI','V12'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V11'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V10'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V7'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V5'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V4'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V3'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V2'].join('_'), ['VENOM','TJS','RUNTIME','ABI','V1'].join('_')]);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, abi: Number.parseInt(plan.values.get('abi') || '12', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, entryCount: Number.parseInt(plan.values.get('entry_count') || '0', 10) >>> 0, tableHash: plan.values.get('table_hash') || plan.values.get('abi_hash') || '', table: plan.values.get('table') || '', exports: plan.values.get('exports') || '' });
}

function parseTurboJsHostImportsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_IMPORT_TABLE_V10', 'VENOM_TJS_HOST_IMPORT_TABLE_V9', 'VENOM_TJS_HOST_IMPORT_TABLE_V8', 'VENOM_TJS_HOST_IMPORT_TABLE_V5', 'VENOM_TJS_HOST_IMPORT_TABLE_V4', 'VENOM_TJS_HOST_IMPORT_TABLE_V3', 'VENOM_TJS_HOST_IMPORT_TABLE_V2', 'VENOM_TJS_HOST_IMPORT_TABLE_V1', 'VENOM_TURBOJS_HOST_IMPORTS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, importCount: Number.parseInt(plan.values.get('import_count') || plan.values.get('entry_count') || '0', 10) >>> 0, tableHash: plan.values.get('table_hash') || plan.values.get('abi_hash') || '', table: plan.values.get('table') || '', design: plan.values.get('design') || 'host-call-import-table' });
}

function parseTurboJsHeapLimitsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CONTEXT_LIMITS_V1', 'VENOM_TURBOJS_HEAP_LIMITS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, heapLimit: Number.parseInt(plan.values.get('default_heap_limit') || '8388608', 10) >>> 0, stackLimit: Number.parseInt(plan.values.get('default_stack_limit') || '262144', 10) >>> 0, maxContexts: Number.parseInt(plan.values.get('max_contexts') || '64', 10) >>> 0, accountingHostCall: plan.values.get('accounting_host_call') || 'turbojs.contextHeapAccounting' });
}

function parseTurboJsScriptBufferMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_SCRIPT_BUFFER_V1', 'VENOM_TURBOJS_SCRIPT_BUFFER_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, capacity: Number.parseInt(plan.values.get('max_script_bytes') || plan.values.get('max_script_buffer') || '786432', 10) >>> 0, allocExport: plan.values.get('alloc_export') || 'venom_tjs_script_buffer_alloc', ptrExport: plan.values.get('ptr_export') || 'venom_tjs_script_buffer_ptr', freeExport: plan.values.get('free_export') || 'venom_tjs_script_buffer_free' });
}

function parseTurboJsConsoleAbiMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CONSOLE_CALLBACK_ABI_V1', 'VENOM_TURBOJS_CONSOLE_ABI_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, abi: Number.parseInt(plan.values.get('abi') || '1', 10) >>> 0, eventFormat: plan.values.get('event_format') || 'json-console-event-v1', eventExport: plan.values.get('event_export') || 'venom_tjs_console_event_ptr' });
}

function parseTurboJsParityProbeMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_PARITY_PROBE_V7', 'VENOM_TJS_PARITY_PROBE_V6', 'VENOM_TJS_PARITY_PROBE_V5', 'VENOM_TJS_PARITY_PROBE_V4', 'VENOM_TJS_PARITY_PROBE_V3', 'VENOM_TJS_PARITY_PROBE_V2', 'VENOM_TJS_PARITY_PROBE_V1', 'VENOM_TURBOJS_PARITY_PROBE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, expected: plan.values.get('expected') || 'turbojs:3', native: plan.values.get('native') || '', wasm: plan.values.get('wasm') || '', hostCall: plan.values.get('host_call') || 'turbojs.wasmParityProbe' });
}

function parseTurboJsReleaseFallbackMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RELEASE_FALLBACK_V1', 'VENOM_TURBOJS_RELEASE_FALLBACK_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, policy: plan.values.get('policy') || 'deny-host-fallback-unless-real-engine-ready', denyHostFallback: plan.values.get('deny_host_fallback') !== 'false', allowWhen: plan.values.get('allow_when') || 'real-engine-ready', denyWhen: plan.values.get('deny_when') || 'engine-module-unavailable-or-compatible-fallback|wasm-interpreter-unavailable' });
}

function parseTurboJsBytecodeManifestMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_BYTECODE_MANIFEST_V3', 'VENOM_TJS_BYTECODE_MANIFEST_V2', 'VENOM_TJS_BYTECODE_MANIFEST_V1', 'VENOM_TURBOJS_BYTECODE_MANIFEST_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, format: plan.values.get('format') || 'native-turbojs-object-bytecode-v3', magic: plan.values.get('magic') || 'VTJSBC03', chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0, execClaim: plan.values.get('exec_claim') || 'runtime-hands-opaque-records-to-turbojs-wasm-boundary' });
}

function parseTurboJsModuleResolverMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_MODULE_RESOLVER_V1', 'VENOM_TURBOJS_MODULE_RESOLVER_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, abi: Number.parseInt(plan.values.get('resolver_abi') || '1', 10) >>> 0, mode: plan.values.get('mode') || 'package-relative-module-map', denyRemoteModules: plan.values.get('deny_remote_modules') !== 'false', denyDynamicImport: plan.values.get('deny_dynamic_import_until_real_engine') !== 'false' });
}

function parseTurboJsExceptionAbiMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_EXCEPTION_ABI_V1', 'VENOM_TURBOJS_EXCEPTION_ABI_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, abi: Number.parseInt(plan.values.get('exception_abi') || '1', 10) >>> 0, schema: plan.values.get('schema') || 'ok|abi|context|code|kind|message|sourceBytes|sourceHash' });
}

function parseTurboJsHostTrapPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_TRAP_POLICY_V1', 'VENOM_TURBOJS_HOST_TRAP_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, policy: plan.values.get('policy') || 'record-unknown-host-imports', unknownImport: plan.values.get('unknown_import') || 'deny' });
}


function parseTurboJsExecutionLifecycleMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_EXECUTION_LIFECYCLE_V1', 'VENOM_TURBOJS_EXECUTION_LIFECYCLE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, states: plan.values.get('states') || 'created|configured|loaded|executing|trapped|disposed', initial: plan.values.get('initial') || 'created', trapState: plan.values.get('trap_state') || 'trapped', strictRelease: plan.values.get('strict_release') === 'true' });
}

function parseTurboJsScriptBufferPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_SCRIPT_BUFFER_POLICY_V1', 'VENOM_TURBOJS_SCRIPT_BUFFER_POLICY_V1']);
  const bool = (key, fallback) => plan.values.has(key) ? plan.values.get(key) === 'true' : fallback;
  return Object.freeze({ version: plan.version, enabled: plan.enabled, rejectZeroSize: bool('reject_zero_size', true), rejectOversized: bool('reject_oversized', true), validateHashBeforeExecute: bool('validate_hash_before_execute', true), trackAllocCounter: bool('track_alloc_counter', true), trackFreeCounter: bool('track_free_counter', true), maxScriptBytes: Number.parseInt(plan.values.get('max_script_bytes') || '786432', 10) >>> 0 });
}

function parseTurboJsContextLimitPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CONTEXT_LIMIT_POLICY_V1', 'VENOM_TURBOJS_CONTEXT_LIMIT_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, maxHeapBytes: Number.parseInt(plan.values.get('max_heap_bytes') || '8388608', 10) >>> 0, maxStackBytes: Number.parseInt(plan.values.get('max_stack_bytes') || '262144', 10) >>> 0, maxScriptBytes: Number.parseInt(plan.values.get('max_script_bytes') || '786432', 10) >>> 0, maxContexts: Number.parseInt(plan.values.get('max_contexts') || '64', 10) >>> 0, maxHostCalls: Number.parseInt(plan.values.get('max_host_calls') || '4096', 10) >>> 0, maxConsoleEvents: Number.parseInt(plan.values.get('max_console_events') || '1024', 10) >>> 0, maxModuleRecords: Number.parseInt(plan.values.get('max_module_records') || '512', 10) >>> 0 });
}

function parseTurboJsHostCallDispatchMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_CALL_DISPATCH_V3', 'VENOM_TJS_HOST_CALL_DISPATCH_V2', 'VENOM_TURBOJS_HOST_CALL_DISPATCH_V2', 'VENOM_TJS_HOST_CALL_DISPATCH_V1', 'VENOM_TURBOJS_HOST_CALL_DISPATCH_V1']);
  const calls = [];
  if (section) {
    for (const line of textDecoder.decode(section.data).split(/\r?\n/)) {
      if (line.startsWith('host_call\t')) {
        const parts = line.split('\t');
        if (parts.length >= 5) calls.push(Object.freeze({ id: Number.parseInt(parts[1] || '0', 10) >>> 0, name: parts[2], signature: parts[3], mode: parts[4] }));
      }
    }
  }
  return Object.freeze({ version: plan.version, enabled: plan.enabled, strictRelease: plan.values.get('strict_release') === 'true', unknownHostCall: plan.values.get('unknown_host_call') || 'deny', dispatchHash: plan.values.get('dispatch_hash') || '', entryCount: Number.parseInt(plan.values.get('entry_count') || String(calls.length), 10) >>> 0, calls: Object.freeze(calls) });
}

function parseTurboJsParityContractMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_PARITY_CONTRACT_V1', 'VENOM_TURBOJS_PARITY_CONTRACT_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '5', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, compare: plan.values.get('compare') || '', abiHash: plan.values.get('abi_hash') || '', hostImportHash: plan.values.get('host_import_hash') || '', hostDispatchHash: plan.values.get('host_dispatch_hash') || '', releaseOnMismatch: plan.values.get('release_on_mismatch') || 'fail-closed' });
}

function parseTurboJsReleaseFailClosedMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RELEASE_FAILCLOSED_V1', 'VENOM_TURBOJS_RELEASE_FAILCLOSED_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, strictRelease: plan.values.get('strict_release') === 'true', debugPolicy: plan.values.get('debug_policy') || 'warn-and-record', releasePolicy: plan.values.get('release_policy') || 'fail-closed', failOn: plan.values.get('fail_on') || '' });
}

function parseTurboJsModuleGraphMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_MODULE_GRAPH_V1', 'VENOM_TURBOJS_MODULE_GRAPH_V1']);
  const modules = [];
  if (section) {
    for (const line of textDecoder.decode(section.data).split(/\r?\n/)) {
      if (line.startsWith('module\t')) {
        const parts = line.split('\t');
        if (parts.length >= 8) modules.push(Object.freeze({ order: Number.parseInt(parts[1] || '0', 10) >>> 0, route: parts[2] || '', source: parts[3] || '', bytes: Number.parseInt(String(parts[4] || '0').replace(/^bytes=/, ''), 10) >>> 0, hash: String(parts[5] || '').replace(/^hash=/, ''), flags: String(parts[6] || '').replace(/^flags=/, ''), isModule: /(^|[|,])module($|[|,])/.test(String(parts[6] || '')) || String(parts[7] || '').replace(/^is_module=/, '') === 'true' }));
      }
    }
  }
  return Object.freeze({ version: plan.version, enabled: plan.enabled, graphFormat: plan.values.get('graph_format') || 'route-scoped-module-graph-v1', resolver: plan.values.get('resolver') || 'package-relative-static-imports', dynamicImport: plan.values.get('dynamic_import') || 'literal-package-graph-candidate', chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0, moduleCount: Number.parseInt(plan.values.get('module_count') || String(modules.length), 10) >>> 0, modules: Object.freeze(modules) });
}

function parseTurboJsModuleExecutionMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_MODULE_EXECUTION_V1', 'VENOM_TURBOJS_MODULE_EXECUTION_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '5', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, mode: plan.values.get('mode') || 'route-scoped-esm-prototype', moduleCount: Number.parseInt(plan.values.get('module_count') || '0', 10) >>> 0, staticImports: plan.values.get('static_imports') || 'package-relative', dynamicImport: plan.values.get('dynamic_import') || 'literal-package-graph-candidate', hostFallback: plan.values.get('host_fallback') || 'esm-transform-prototype' });
}

function parseTurboJsModuleCacheMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_MODULE_CACHE_V1', 'VENOM_TURBOJS_MODULE_CACHE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '5', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, moduleCount: Number.parseInt(plan.values.get('module_count') || '0', 10) >>> 0, cacheKey: plan.values.get('cache_key') || 'normalized-specifier|route|source-hash', namespaceModel: plan.values.get('namespace_model') || 'frozen-export-record', instantiateOnce: plan.values.get('instantiate_once') !== 'false', evaluateOnce: plan.values.get('evaluate_once') !== 'false' });
}

function parseTurboJsResolverAuditMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RESOLVER_AUDIT_V1', 'VENOM_TURBOJS_RESOLVER_AUDIT_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '5', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, moduleCount: Number.parseInt(plan.values.get('module_count') || '0', 10) >>> 0, record: plan.values.get('record') || 'specifier|referrer|normalized|status|host-call-id', unknownSpecifier: plan.values.get('unknown_specifier') || 'record-and-empty-namespace', dynamicImport: plan.values.get('dynamic_import') || 'literal-package-graph-candidate' });
}

function parseTurboJsInteropFallbackMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_INTEROP_FALLBACK_V1', 'VENOM_TURBOJS_INTEROP_FALLBACK_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '5', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, fallbackKind: plan.values.get('fallback_kind') || 'host-esm-transform-prototype', allowedSyntax: plan.values.get('allowed_syntax') || '', deniedSyntax: plan.values.get('denied_syntax') || '', releaseBehavior: plan.values.get('release_behavior') || 'fail-closed-if-required-contract-missing' });
}

function parseTurboJsExecutionPipelineMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_EXECUTION_PIPELINE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, stages: plan.values.get('stages') || '', chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0, releaseRequires: plan.values.get('release_requires') || '' });
}

function parseTurboJsScriptRecordsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_SCRIPT_RECORDS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0, recordFields: plan.values.get('record_fields') || '', rejectUnpreparedEval: plan.values.get('reject_unprepared_eval') !== 'false' });
}

function parseTurboJsEvalResultsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_EVAL_RESULTS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, resultFields: plan.values.get('result_fields') || '', resultHash: plan.values.get('result_hash') || 'fnv1a32-json-record' });
}

function parseTurboJsConsoleCaptureMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CONSOLE_CAPTURE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, levels: plan.values.get('levels') || 'debug|log|info|warn|error', schema: plan.values.get('capture_schema') || '', flushHostCall: plan.values.get('flush_host_call') || 'console.capture.flush' });
}

function parseTurboJsFailureReportsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_FAILURE_REPORTS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, schema: plan.values.get('schema') || '', releaseBehavior: plan.values.get('release_behavior') || 'fail-closed-with-report' });
}

function parseTurboJsExecutionJournalMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_EXECUTION_JOURNAL_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, recordFields: plan.values.get('record_fields') || '', checkpointLink: plan.values.get('checkpoint_link') || '', maxRecords: Number.parseInt(plan.values.get('max_records') || '64', 10) >>> 0 });
}

function parseTurboJsCheckpointPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CHECKPOINT_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, maxCheckpoints: Number.parseInt(plan.values.get('max_checkpoints') || '64', 10) >>> 0, capture: plan.values.get('capture') || 'context|script|eval|console|exception|lifecycle', restorePolicy: plan.values.get('restore_policy') || 'same-context-only' });
}

function parseTurboJsReplayCursorMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_REPLAY_CURSOR_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, cursorFields: plan.values.get('cursor_fields') || '', deterministicInput: plan.values.get('deterministic_input') || 'script_hash|host_call_index|module_cache_index' });
}

function parseTurboJsResumeStateMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RESUME_STATE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, stateFields: plan.values.get('state_fields') || '', failureBehavior: plan.values.get('failure_behavior') || 'trap-and-report' });
}

function parseTurboJsDeterminismAuditMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_DETERMINISM_AUDIT_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, auditFields: plan.values.get('audit_fields') || '', releaseRequires: plan.values.get('release_requires') || 'hash-match|host-call-order|checkpoint-sequence' });
}

function parseTurboJsSnapshotPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_SNAPSHOT_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, capture: plan.values.get('capture') || 'post-checkpoint', validate: plan.values.get('validate') || 'hash-match', releaseOnMismatch: plan.values.get('release_on_mismatch') || 'trap' });
}

function parseTurboJsSnapshotRecordsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_SNAPSHOT_RECORDS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, chunkCount: Number.parseInt(plan.values.get('chunk_count') || '0', 10) >>> 0, recordFields: plan.values.get('record_fields') || '' });
}

function parseTurboJsReplayValidationMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_REPLAY_VALIDATION_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, checks: plan.values.get('checks') || '', onMismatch: plan.values.get('on_mismatch') || 'trap' });
}

function parseTurboJsDeterminismLedgerMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_DETERMINISM_LEDGER_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, chainHash: plan.values.get('chain_hash') || 'fnv1a64-linked' });
}

function parseTurboJsAuditSealMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_AUDIT_SEAL_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '8', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, releaseRequires: plan.values.get('release_requires') || 'snapshot-valid|ledger-linked|abi-match' });
}

function parseTurboJsExecutionCommitMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_EXECUTION_COMMIT_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, commitHostCall: plan.values.get('commit_host_call') || 'execution.commit', releaseRequires: plan.values.get('release_requires') || 'commit-record-present-and-linked' });
}

function parseTurboJsRollbackPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_ROLLBACK_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, rollbackHostCall: plan.values.get('rollback_host_call') || 'execution.rollback', releaseBehavior: plan.values.get('release_behavior') || 'fail-closed-on-unmatched-rollback' });
}

function parseTurboJsHostCallReceiptsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_CALL_RECEIPTS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, receiptHostCall: plan.values.get('receipt_host_call') || 'host.receipt', releaseRequires: plan.values.get('release_requires') || 'all-host-calls-receipted' });
}

function parseTurboJsReleaseAcceptanceMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RELEASE_ACCEPTANCE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, checks: plan.values.get('acceptance_checks') || '', releaseOnFailure: plan.values.get('release_on_failure') || 'trap' });
}

function parseTurboJsCommitAuditMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_COMMIT_AUDIT_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, auditFields: plan.values.get('audit_fields') || '', releaseRequires: plan.values.get('release_requires') || 'commit-audit-present' });
}


function parseTurboJsCapabilityPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CAPABILITY_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, mode: plan.values.get('mode') || 'deny-by-default-host-io', capabilities: plan.values.get('capabilities') || '', releaseRequires: plan.values.get('release_requires') || 'capability-policy-present' });
}

function parseTurboJsHostIoPolicyMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_IO_POLICY_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, network: plan.values.get('network') || 'blocked-unless-capability', filesystem: plan.values.get('filesystem') || 'blocked', releaseBehavior: plan.values.get('release_behavior') || 'fail-closed-on-unsealed-io' });
}

function parseTurboJsPermissionSealMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_PERMISSION_SEAL_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, algorithm: plan.values.get('algorithm') || 'fnv1a-policy-chain', releaseRequires: plan.values.get('release_requires') || 'permission-seal-present' });
}

function parseTurboJsPolicyReceiptsMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_POLICY_RECEIPTS_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, hostCall: plan.values.get('host_call') || 'host.receipt', releaseRequires: plan.values.get('release_requires') || 'policy-receipts-linked' });
}

function parseTurboJsReleaseGateMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RELEASE_GATE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, gate: plan.values.get('gate') || 'abi+policy+seal+receipts', failOn: plan.values.get('fail_on') || 'missing-policy|missing-seal|unreceipted-host-call' });
}

function parseTurboJsHostIoDecisionMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_IO_DECISION_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, fields: plan.values.get('fields') || 'context|io_class|capability|decision|payload_hash', releaseRequires: plan.values.get('release_requires') || 'decision-record-linked' });
}

function parseTurboJsHostIoDenyTraceMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_HOST_IO_DENY_TRACE_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, fields: plan.values.get('fields') || 'context|io_class|capability|reason|payload_hash', releaseBehavior: plan.values.get('release_behavior') || 'fail-closed-on-any-denial' });
}

function parseTurboJsCapabilityLedgerMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_CAPABILITY_LEDGER_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, fields: plan.values.get('fields') || 'capability|decision|receipt_hash|host_call_id', releaseRequires: plan.values.get('release_requires') || 'ledger-linked-to-seal' });
}

function parseTurboJsPolicySealAuditMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_POLICY_SEAL_AUDIT_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, fields: plan.values.get('fields') || 'seal_hash|decision_hash|ledger_hash|deny_trace_hash', releaseRequires: plan.values.get('release_requires') || 'policy-seal-audit-present' });
}

function parseTurboJsRuntimeDenylistMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_TJS_RUNTIME_DENYLIST_V1']);
  return Object.freeze({ version: plan.version, enabled: plan.enabled, runtimeAbi: Number.parseInt(plan.values.get('runtime_abi') || '11', 10) >>> 0, packageVersion: Number.parseInt(plan.values.get('package_version') || '40', 10) >>> 0, denylist: plan.values.get('denylist') || plan.values.get('deny') || 'fetch|unlisted-host-call|unknown-capability', releaseBehavior: plan.values.get('release_behavior') || 'fail-closed-on-deny-trace' });
}

function parseAsyncHostQueueMetadata(section) {
  const plan = parseKeyValueMetadata(section, ['VENOM_ASYNC_HOST_QUEUE_V10', 'VENOM_ASYNC_HOST_QUEUE_V9', 'VENOM_ASYNC_HOST_QUEUE_V8', 'VENOM_ASYNC_HOST_QUEUE_V7', 'VENOM_ASYNC_HOST_QUEUE_V6', 'VENOM_ASYNC_HOST_QUEUE_V5', 'VENOM_ASYNC_HOST_QUEUE_V4', 'VENOM_ASYNC_HOST_QUEUE_V3', 'VENOM_ASYNC_HOST_QUEUE_V2', 'VENOM_ASYNC_HOST_QUEUE_V1']);
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    requestIdBits: Number.parseInt(plan.values.get('request_id_bits') || '32', 10) >>> 0,
    state: plan.values.get('state') || 'pending|fulfilled|rejected',
    maxPending: Number.parseInt(plan.values.get('max_pending') || '128', 10) >>> 0,
    maxCompleted: Number.parseInt(plan.values.get('max_completed') || '128', 10) >>> 0,
    maxHostCallsPerRoute: Number.parseInt(plan.values.get('max_host_calls_per_route') || '8192', 10) >>> 0,
    maxConcurrentFetches: Number.parseInt(plan.values.get('max_concurrent_fetches') || '16', 10) >>> 0,
    maxDomHandles: Number.parseInt(plan.values.get('max_dom_handles') || '4096', 10) >>> 0,
    maxFetchResponseBytes: Number.parseInt(plan.values.get('max_fetch_response_bytes') || '1048576', 10) >>> 0,
    maxFetchRequestBytes: Number.parseInt(plan.values.get('max_fetch_request_bytes') || '65536', 10) >>> 0,
    maxTimerDelayMs: Number.parseInt(plan.values.get('max_timer_delay_ms') || '86400000', 10) >>> 0,
    maxRouteLifetimeMs: Number.parseInt(plan.values.get('max_route_lifetime_ms') || '86400000', 10) >>> 0,
    teardown: plan.values.get('teardown') || 'cancel-timers|abort-fetches|reject-pending|destroy-contexts',
    wasmRequestBoundary: plan.values.get('wasm_request_boundary') || 'planned',
    turbojsRequestBoundary: plan.values.get('turbojs_request_boundary') || 'planned',
  });
}

