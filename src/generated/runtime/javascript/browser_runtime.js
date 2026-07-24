
const PACKAGE_FLAG = Object.freeze({
  PROTECT: 1,
  RELEASE: 2,
  POLYMORPHIC: 4,
  COMPRESSED_SECTIONS: 8,
  CRYPTO_PROVIDER_READY: 16,
  INTEGRITY_METADATA: 32,
  AEAD_PROVIDER_READY: 64,
  RUNTIME_HARDENED: 128,
  WASM_RUNTIME: 256,
  HOST_BRIDGE: 512,
  BINARY_DOM_OPS: 1024,
  FETCH_BRIDGE: 2048,
  ASYNC_HOST_QUEUE: 4096,
  TIMER_BRIDGE: 8192,
  EVENT_QUEUE: 16384,
  TURBOJS_BRIDGE: 32768,
  SCRIPT_ISOLATION: 65536,
  SCRIPT_POLICY: 131072,
  TURBOJS_CHUNKS: 262144,
  TURBOJS_ENGINE: 524288,
  SCRIPT_ENGINE_FALLBACK: 1048576,
  TURBOJS_ENGINE_MODULE: 2097152,
  TURBOJS_CONTEXT_LIFECYCLE: 4194304,
  HOST_CAPABILITIES: 8388608,
  TURBOJS_ADAPTER_DIAGNOSTICS: 16777216,
  TURBOJS_WASM_RUNTIME: 33554432,
  TURBOJS_SOURCE_TRANSFER: 67108864,
  TURBOJS_CONSOLE_BRIDGE: 134217728,
  TURBOJS_EXECUTION_RECORDS: 268435456,
  TURBOJS_RESULT_BRIDGE: 536870912,
  TURBOJS_FALLBACK_POLICY: 1073741824,
  TURBOJS_ENGINE_BACKEND: 2147483648,
});

const SECTION_FLAG = Object.freeze({
  COMPRESSED: 1,
  ENCRYPTED: 2,
});

const SECTION = Object.freeze({
  MANIFEST: 1,
  ROUTES: 2,
  DOM_TEMPLATES: 3,
  CSS: 4,
  JAVASCRIPT: 5,
  TURBOJS: 6,
  VM_BYTECODE: 7,
  ASSET: 8,
  INTEGRITY: 9,
  STRINGS: 10,
  ASSET_MANIFEST: 11,
  HOST_BRIDGE: 12,
  FETCH_BRIDGE: 13,
  ASYNC_HOST_QUEUE: 14,
  TIMER_BRIDGE: 15,
  EVENT_QUEUE: 16,
  TURBOJS_BRIDGE: 17,
  SCRIPT_ISOLATION: 18,
  SCRIPT_POLICY: 19,
  TURBOJS_CHUNKS: 20,
  TURBOJS_ENGINE: 21,
  SCRIPT_ENGINE_POLICY: 22,
  TURBOJS_ENGINE_MODULE: 23,
  TURBOJS_CONTEXT_LIFECYCLE: 24,
  HOST_CAPABILITIES: 25,
  TURBOJS_ADAPTER_DIAGNOSTICS: 26,
  TURBOJS_WASM_RUNTIME: 27,
  TURBOJS_SOURCE_TRANSFER: 28,
  TURBOJS_CONSOLE_BRIDGE: 29,
  TURBOJS_EXECUTION_RECORDS: 30,
  TURBOJS_RESULT_BRIDGE: 31,
  TURBOJS_FALLBACK_POLICY: 32,
  TURBOJS_ENGINE_BACKEND: 33,
  TURBOJS_NATIVE_PARITY: 34,
  TURBOJS_EXECUTION_MODE: 35,
  TURBOJS_RUNTIME_ABI: 36,
  TURBOJS_HOST_IMPORTS: 37,
  TURBOJS_HEAP_LIMITS: 38,
  TURBOJS_SCRIPT_BUFFER: 39,
  TURBOJS_CONSOLE_ABI: 40,
  TURBOJS_PARITY_PROBE: 41,
  TURBOJS_RELEASE_FALLBACK: 42,
  TURBOJS_BYTECODE_MANIFEST: 43,
  TURBOJS_MODULE_RESOLVER: 44,
  TURBOJS_EXCEPTION_ABI: 45,
  TURBOJS_HOST_TRAP_POLICY: 46,
  TURBOJS_EXECUTION_LIFECYCLE: 47,
  TURBOJS_SCRIPT_BUFFER_POLICY: 48,
  TURBOJS_CONTEXT_LIMIT_POLICY: 49,
  TURBOJS_HOST_CALL_DISPATCH: 50,
  TURBOJS_PARITY_CONTRACT: 51,
  TURBOJS_RELEASE_FAILCLOSED: 52,
  TURBOJS_MODULE_GRAPH: 53,
  TURBOJS_MODULE_EXECUTION: 54,
  TURBOJS_MODULE_CACHE: 55,
  TURBOJS_RESOLVER_AUDIT: 56,
  TURBOJS_INTEROP_FALLBACK: 57,
  TURBOJS_EXECUTION_PIPELINE: 58,
  TURBOJS_SCRIPT_RECORDS: 59,
  TURBOJS_EVAL_RESULTS: 60,
  TURBOJS_CONSOLE_CAPTURE: 61,
  TURBOJS_FAILURE_REPORTS: 62,
  TURBOJS_EXECUTION_JOURNAL: 63,
  TURBOJS_CHECKPOINT_POLICY: 64,
  TURBOJS_REPLAY_CURSOR: 65,
  TURBOJS_RESUME_STATE: 66,
  TURBOJS_DETERMINISM_AUDIT: 67,
  TURBOJS_SNAPSHOT_POLICY: 68,
  TURBOJS_SNAPSHOT_RECORDS: 69,
  TURBOJS_REPLAY_VALIDATION: 70,
  TURBOJS_DETERMINISM_LEDGER: 71,
  TURBOJS_AUDIT_SEAL: 72,
  TURBOJS_EXECUTION_COMMIT: 73,
  TURBOJS_ROLLBACK_POLICY: 74,
  TURBOJS_HOST_CALL_RECEIPTS: 75,
  TURBOJS_RELEASE_ACCEPTANCE: 76,
  TURBOJS_COMMIT_AUDIT: 77,
  TURBOJS_CAPABILITY_POLICY: 78,
  TURBOJS_HOST_IO_POLICY: 79,
  TURBOJS_PERMISSION_SEAL: 80,
  TURBOJS_POLICY_RECEIPTS: 81,
  TURBOJS_RELEASE_GATE: 82,
  TURBOJS_HOST_IO_DECISION: 83,
  TURBOJS_HOST_IO_DENY_TRACE: 84,
  TURBOJS_CAPABILITY_LEDGER: 85,
  TURBOJS_POLICY_SEAL_AUDIT: 86,
  TURBOJS_RUNTIME_DENYLIST: 87,
  PACKAGE_FEATURES: 88,
});

const LOGICAL = Object.freeze({
  NOP: 0,
  LOAD_CONST: 1,
  CREATE_ELEMENT: 2,
  SET_ATTRIBUTE: 3,
  SET_TEXT: 4,
  APPEND_CHILD: 5,
  ENTER_ELEMENT: 6,
  LEAVE_ELEMENT: 7,
  CALL_TURBOJS: 8,
  HALT: 9,
});

const textDecoder = new TextDecoder();
const textEncoder = new TextEncoder();

function readU32(view, offset) {
  if (offset + 4 > view.byteLength) throw new Error('truncated package while reading u32');
  return view.getUint32(offset, true);
}

function readU64(view, offset) {
  if (offset + 8 > view.byteLength) throw new Error('truncated package while reading u64');
  const lo = BigInt(view.getUint32(offset, true));
  const hi = BigInt(view.getUint32(offset + 4, true));
  const value = (hi << 32n) | lo;
  if (value > BigInt(Number.MAX_SAFE_INTEGER)) throw new Error('package offset exceeds JavaScript safe integer range');
  return Number(value);
}

function fnv1a64(bytes, zeroHashField = false) {
  let hash = 1469598103934665603n;
  const prime = 0x100000001b3n;
  for (let i = 0; i < bytes.length; ++i) {
    const value = zeroHashField && i >= 72 && i < 80 ? 0 : bytes[i];
    hash ^= BigInt(value);
    hash = BigInt.asUintN(64, hash * prime);
  }
  return hash;
}

function fnv1a32(bytes) {
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < bytes.length; ++i) {
    hash ^= bytes[i];
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  return hash >>> 0;
}

function readHash64(view, offset) {
  const lo = BigInt(view.getUint32(offset, true));
  const hi = BigInt(view.getUint32(offset + 4, true));
  return (hi << 32n) | lo;
}


function sealNameHash(name) {
  return fnv1a64(textEncoder.encode(String(name || '')));
}


function protectedSectionAlias(type, canonicalName) {
  let hash = 1469598103934665603n;
  const prime = 0x100000001b3n;
  const rawType = type >>> 0;
  for (let i = 0; i < 4; ++i) {
    hash ^= BigInt((rawType >> (i * 8)) & 0xff);
    hash = BigInt.asUintN(64, hash * prime);
  }
  const bytes = textEncoder.encode(String(canonicalName || ''));
  for (const byte of bytes) {
    hash ^= BigInt(byte);
    hash = BigInt.asUintN(64, hash * prime);
  }
  return 's.' + hash.toString(16).padStart(16, '0');
}

function shouldRedactSectionName(type) {
  return type !== SECTION.ASSET;
}

function sectionNameMatches(type, storedName, canonicalName) {
  if (storedName === canonicalName) return true;
  return shouldRedactSectionName(type) && storedName === protectedSectionAlias(type, canonicalName);
}

function fnvUpdateBytes(hash, bytes) {
  const prime = 0x100000001b3n;
  for (const byte of bytes) {
    hash ^= BigInt(byte);
    hash = BigInt.asUintN(64, hash * prime);
  }
  return hash;
}

function fnvUpdateU64(hash, value) {
  let v = BigInt.asUintN(64, BigInt(value));
  const prime = 0x100000001b3n;
  for (let i = 0; i < 8; ++i) {
    hash ^= (v >> BigInt(i * 8)) & 0xffn;
    hash = BigInt.asUintN(64, hash * prime);
  }
  return hash;
}

function mix64(value) {
  value = BigInt.asUintN(64, value + 0x9e3779b97f4a7c15n);
  value = BigInt.asUintN(64, (value ^ (value >> 30n)) * 0xbf58476d1ce4e5b9n);
  value = BigInt.asUintN(64, (value ^ (value >> 27n)) * 0x94d049bb133111ebn);
  return BigInt.asUintN(64, value ^ (value >> 31n));
}

function deriveLegacySealSeed(type, nameDigest, plainSize, plainHash) {
  let h = 1469598103934665603n;
  h = fnvUpdateU64(h, BigInt(type >>> 0));
  h = fnvUpdateU64(h, nameDigest);
  h = fnvUpdateU64(h, BigInt(plainSize));
  h = fnvUpdateU64(h, plainHash);
  h ^= 0x7f4a7c159e3779b9n;
  h = mix64(h ^ 0xd6e8feb86659fd93n);
  return h === 0n ? 0x7f4a7c159e3779b9n : h;
}

function nextSealWord(stateRef) {
  stateRef.value = BigInt.asUintN(64, stateRef.value + 0x9e3779b97f4a7c15n);
  return mix64(stateRef.value);
}

function xorSealStream(input, seed) {
  const out = new Uint8Array(input.length);
  const state = { value: seed };
  let word = 0n;
  for (let i = 0; i < input.length; ++i) {
    if ((i & 7) === 0) word = nextSealWord(state);
    out[i] = input[i] ^ Number((word >> BigInt((i & 7) * 8)) & 0xffn);
  }
  return out;
}

function deriveAeadStreamSeed(type, nameDigest, plainSize, nonceA, nonceB) {
  let h = 1469598103934665603n ^ 0xa41bc927f35d69e1n;
  h = fnvUpdateU64(h, BigInt(type >>> 0));
  h = fnvUpdateU64(h, nameDigest);
  h = fnvUpdateU64(h, BigInt(plainSize));
  h = fnvUpdateU64(h, nonceA);
  h = fnvUpdateU64(h, nonceB);
  return mix64(h ^ 0x6c8e9cf570932bd5n);
}

function aeadTag(type, nameDigest, plainSize, nonceA, nonceB, ciphertext) {
  let a = 1469598103934665603n ^ 0x3d6f0a9b8c71e245n;
  a = fnvUpdateU64(a, BigInt(type >>> 0));
  a = fnvUpdateU64(a, nameDigest);
  a = fnvUpdateU64(a, BigInt(plainSize));
  a = fnvUpdateU64(a, nonceA);
  a = fnvUpdateU64(a, nonceB);
  a = fnvUpdateBytes(a, ciphertext);
  a = mix64(a ^ 0x9b62d14fa7c8305dn);

  let b = 1469598103934665603n ^ 0x6c8e9cf570932bd5n;
  b = fnvUpdateU64(b, nonceB);
  b = fnvUpdateU64(b, nonceA);
  b = fnvUpdateU64(b, BigInt(plainSize));
  b = fnvUpdateU64(b, nameDigest);
  b = fnvUpdateU64(b, BigInt(type >>> 0));
  b = fnvUpdateBytes(b, ciphertext);
  b = mix64(b ^ 0xa41bc927f35d69e1n);
  return [a, b];
}

function hasMagic(input, magic) {
  if (!input || input.length < magic.length) return false;
  for (let i = 0; i < magic.length; ++i) {
    if (input[i] !== magic.charCodeAt(i)) return false;
  }
  return true;
}

function openLegacySealedSectionV1(input, type, name) {
  requireAscii(input, 'VSEAL001', 'sealed section');
  if (input.length < 40) throw new Error('truncated sealed section: ' + name);
  const view = new DataView(input.buffer, input.byteOffset, input.byteLength);
  const version = readU32(view, 8);
  const sealedType = readU32(view, 12);
  const plainSize = readU64(view, 16);
  const plainHash = readHash64(view, 24);
  const sealedNameHash = readHash64(view, 32);
  if (version !== 1) throw new Error('unsupported sealed section version: ' + name);
  if (sealedType !== (type >>> 0)) throw new Error('sealed section type mismatch: ' + name);
  if (sealedNameHash !== sealNameHash(name)) throw new Error('sealed section name mismatch: ' + name);
  const ciphertext = input.slice(40);
  if (plainSize !== ciphertext.length) throw new Error('sealed section size mismatch: ' + name);
  const seed = deriveLegacySealSeed(type, sealedNameHash, plainSize, plainHash);
  const plaintext = xorSealStream(ciphertext, seed);
  if (fnv1a64(plaintext) !== plainHash) throw new Error('sealed section authentication mismatch: ' + name);
  return plaintext;
}

function openSealedSectionV1(input, type, name) {
  if (hasMagic(input, 'VSODIUM1')) throw new Error('libsodium-sealed packages require the native/audited crypto runtime path, not the browser JS fallback: ' + name);
  if (hasMagic(input, 'VSEAL001')) return openLegacySealedSectionV1(input, type, name);
  requireAscii(input, 'VAEAD001', 'AEAD sealed section');
  if (input.length < 64) throw new Error('truncated AEAD sealed section: ' + name);
  const view = new DataView(input.buffer, input.byteOffset, input.byteLength);
  const version = readU32(view, 8);
  const sealedType = readU32(view, 12);
  const plainSize = readU64(view, 16);
  const sealedNameHash = readHash64(view, 24);
  const nonceA = readHash64(view, 32);
  const nonceB = readHash64(view, 40);
  const tagA = readHash64(view, 48);
  const tagB = readHash64(view, 56);
  if (version !== 1) throw new Error('unsupported AEAD sealed section version: ' + name);
  if (sealedType !== (type >>> 0)) throw new Error('AEAD sealed section type mismatch: ' + name);
  if (sealedNameHash !== sealNameHash(name)) throw new Error('AEAD sealed section name mismatch: ' + name);
  const ciphertext = input.slice(64);
  if (plainSize !== ciphertext.length) throw new Error('AEAD sealed section size mismatch: ' + name);
  const expectedTag = aeadTag(type, sealedNameHash, plainSize, nonceA, nonceB, ciphertext);
  if (expectedTag[0] !== tagA || expectedTag[1] !== tagB) throw new Error('AEAD sealed section authentication mismatch: ' + name);
  const seed = deriveAeadStreamSeed(type, sealedNameHash, plainSize, nonceA, nonceB);
  return xorSealStream(ciphertext, seed);
}


function hexFromBytes(bytes) {
  let out = '';
  for (const byte of bytes) out += byte.toString(16).padStart(2, '0');
  return out;
}

function rotr32(value, bits) {
  return ((value >>> bits) | (value << (32 - bits))) >>> 0;
}

function sha256Hex(bytes) {
  const k = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
  ];
  const bitLen = bytes.length * 8;
  const paddedSize = Math.ceil((bytes.length + 9) / 64) * 64;
  const msg = new Uint8Array(paddedSize);
  msg.set(bytes);
  msg[bytes.length] = 0x80;
  const hi = Math.floor(bitLen / 0x100000000);
  const lo = bitLen >>> 0;
  msg[paddedSize - 8] = (hi >>> 24) & 0xff;
  msg[paddedSize - 7] = (hi >>> 16) & 0xff;
  msg[paddedSize - 6] = (hi >>> 8) & 0xff;
  msg[paddedSize - 5] = hi & 0xff;
  msg[paddedSize - 4] = (lo >>> 24) & 0xff;
  msg[paddedSize - 3] = (lo >>> 16) & 0xff;
  msg[paddedSize - 2] = (lo >>> 8) & 0xff;
  msg[paddedSize - 1] = lo & 0xff;

  const h = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19];
  const w = new Uint32Array(64);
  for (let chunk = 0; chunk < msg.length; chunk += 64) {
    for (let i = 0; i < 16; ++i) {
      const base = chunk + i * 4;
      w[i] = ((msg[base] << 24) | (msg[base + 1] << 16) | (msg[base + 2] << 8) | msg[base + 3]) >>> 0;
    }
    for (let i = 16; i < 64; ++i) {
      const s0 = (rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >>> 3)) >>> 0;
      const s1 = (rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >>> 10)) >>> 0;
      w[i] = (w[i - 16] + s0 + w[i - 7] + s1) >>> 0;
    }
    let a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (let i = 0; i < 64; ++i) {
      const s1 = (rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25)) >>> 0;
      const ch = ((e & f) ^ ((~e) & g)) >>> 0;
      const temp1 = (hh + s1 + ch + k[i] + w[i]) >>> 0;
      const s0 = (rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22)) >>> 0;
      const maj = ((a & b) ^ (a & c) ^ (b & c)) >>> 0;
      const temp2 = (s0 + maj) >>> 0;
      hh = g; g = f; f = e; e = (d + temp1) >>> 0; d = c; c = b; b = a; a = (temp1 + temp2) >>> 0;
    }
    h[0] = (h[0] + a) >>> 0;
    h[1] = (h[1] + b) >>> 0;
    h[2] = (h[2] + c) >>> 0;
    h[3] = (h[3] + d) >>> 0;
    h[4] = (h[4] + e) >>> 0;
    h[5] = (h[5] + f) >>> 0;
    h[6] = (h[6] + g) >>> 0;
    h[7] = (h[7] + hh) >>> 0;
  }
  const out = new Uint8Array(32);
  for (let i = 0; i < 8; ++i) {
    out[i * 4] = (h[i] >>> 24) & 0xff;
    out[i * 4 + 1] = (h[i] >>> 16) & 0xff;
    out[i * 4 + 2] = (h[i] >>> 8) & 0xff;
    out[i * 4 + 3] = h[i] & 0xff;
  }
  return hexFromBytes(out);
}

function asciiOf(bytes) {
  if (!bytes) return '';
  let out = '';
  for (let i = 0; i < bytes.length; ++i) out += String.fromCharCode(bytes[i]);
  return out;
}

function requireAscii(bytes, expected, label) {
  const actual = asciiOf(bytes.slice(0, expected.length));
  if (actual !== expected) throw new Error('invalid ' + label + ' magic');
}

function decodeName(bytes, offset, size) {
  return textDecoder.decode(bytes.slice(offset, offset + size));
}

function assertRange(offset, size, total, label) {
  if (offset < 0 || size < 0 || offset > total || offset + size > total) {
    throw new Error('invalid package ' + label + ' range');
  }
}

function decompressRleV1(input, expectedSize) {
  requireAscii(input, 'VCLZ0008', 'compressed section');
  const view = new DataView(input.buffer, input.byteOffset, input.byteLength);
  const version = readU32(view, 8);
  const declaredSize = readU64(view, 12);
  if (version !== 1) throw new Error('unsupported compressed section version: ' + version);
  if (declaredSize !== expectedSize) throw new Error('compressed section raw size mismatch');

  const out = new Uint8Array(expectedSize);
  let src = 20;
  let dst = 0;
  while (src < input.length) {
    const token = input[src++];
    if ((token & 0x80) !== 0) {
      const length = (token & 0x7f) + 4;
      if (src + 2 > input.length) throw new Error('truncated compressed backref');
      const distance = input[src] | (input[src + 1] << 8);
      src += 2;
      if (distance === 0 || distance > dst) throw new Error('invalid compressed backref distance');
      if (dst + length > out.length) throw new Error('compressed section expands past expected size');
      const back = dst - distance;
      for (let i = 0; i < length; ++i) {
        out[dst++] = out[back + i];
      }
    } else {
      const literal = token + 1;
      if (src + literal > input.length) throw new Error('truncated compressed literal');
      if (dst + literal > out.length) throw new Error('compressed section expands past expected size');
      out.set(input.slice(src, src + literal), dst);
      src += literal;
      dst += literal;
    }
  }
  if (dst !== expectedSize) throw new Error('compressed section expanded to the wrong size');
  return out;
}

let venomPackageSectionDecoder = null;
let venomPackageParser = null;
let venomRouteResolver = null;

function decodeSectionData(storedData, flags, rawSize, name, type, sectionIndex, pkg) {
  if (venomPackageSectionDecoder) {
    return venomPackageSectionDecoder(pkg, sectionIndex, rawSize, name, type);
  }
  let data = storedData;
  if ((flags & SECTION_FLAG.ENCRYPTED) !== 0) {
    data = openSealedSectionV1(data, type, name);
  }
  if ((flags & SECTION_FLAG.COMPRESSED) !== 0) {
    return decompressRleV1(data, rawSize);
  }
  if (data.length !== rawSize) throw new Error('uncompressed section raw size mismatch: ' + name);
  return data;
}

function isIntegrityAuthSection(section) {
  return section && section.type === SECTION.INTEGRITY && sectionNameMatches(SECTION.INTEGRITY, section.name, 'integrity-auth.vsig');
}

function integrityKey(type, name) {
  return String(type) + '\t' + name;
}

function verifyLazySectionDigest(pkg, section) {
  if (!section || section.integrityVerified) return;
  if (isIntegrityAuthSection(section)) {
    section.integrityVerified = true;
    return;
  }
  if (pkg.integrityMap) {
    const entry = pkg.integrityMap.get(integrityKey(section.type, section.name));
    if (!entry) throw new Error('integrity metadata does not cover section: ' + section.name);
    if (entry.size !== section._data.length) throw new Error('integrity size mismatch: ' + section.name);
    const actual = sha256Hex(section._data);
    if (actual !== entry.digest) throw new Error('integrity SHA-256 mismatch: ' + section.name);
  } else if (pkg.integrityRequired) {
    throw new Error('section decoded before integrity metadata was installed: ' + section.name);
  }
  if (pkg.featureDigestMap) {
    const featureDigest = pkg.featureDigestMap.get(integrityKey(section.type, section.name));
    if (featureDigest && sha256Hex(section._data) !== featureDigest) {
      throw new Error('package feature digest mismatch: ' + section.name);
    }
  }
  section.integrityVerified = true;
}

function makeLazySection(pkg, index, type, flags, name, dataOffset, dataSize, rawSize, hash, storedData) {
  const section = {
    type,
    flags,
    name,
    offset: dataOffset,
    size: rawSize,
    storedSize: dataSize,
    rawSize,
    hash,
    storedData,
    decoded: false,
    integrityVerified: false,
    _data: null,
  };
  Object.defineProperty(section, 'data', {
    enumerable: true,
    get() {
      if (!section.decoded) {
        section._data = decodeSectionData(section.storedData, section.flags, section.rawSize, section.name, section.type, index, pkg);
        section.decoded = true;
        pkg.decodedSectionCount = (pkg.decodedSectionCount || 0) + 1;
      }
      verifyLazySectionDigest(pkg, section);
      return section._data;
    },
  });
  return section;
}

function verifyIntegrityMetadata(pkg) {
  pkg.integrityRequired = (pkg.flags & PACKAGE_FLAG.INTEGRITY_METADATA) !== 0 || (pkg.flags & PACKAGE_FLAG.RELEASE) !== 0;
  const metadata = pkg.sections.find((section) => isIntegrityAuthSection(section));
  if (!metadata) {
    if (pkg.integrityRequired) throw new Error('package is missing integrity-auth.vsig metadata');
    return;
  }

  const lines = textDecoder.decode(metadata.data).split(/\r?\n/).filter((line) => line.length > 0);
  if (lines[0] !== 'VENOM_INTEGRITY_V1') throw new Error('invalid integrity metadata header');
  let sawProvider = false;
  let sawScope = false;
  let declaredCount = null;
  const expected = new Map();
  for (const line of lines.slice(1)) {
    if (line === 'provider=sha256-software-v1') { sawProvider = true; continue; }
    if (line === 'scope=decoded-sections') { sawScope = true; continue; }
    if (line.startsWith('section_count=')) { declaredCount = Number.parseInt(line.slice('section_count='.length), 10); continue; }
    if (!line.startsWith('section\t')) continue;
    const parts = line.split('\t');
    if (parts.length !== 5) throw new Error('invalid integrity section entry');
    const type = Number.parseInt(parts[1], 10);
    const name = parts[2];
    const size = Number.parseInt(parts[3], 10);
    const digest = parts[4];
    if (!/^[0-9a-f]{64}$/i.test(digest)) throw new Error('invalid integrity SHA-256 digest');
    const key = integrityKey(type, name);
    if (expected.has(key)) throw new Error('duplicate integrity metadata entry: ' + name);
    expected.set(key, { size, digest: digest.toLowerCase() });
  }
  if (!sawProvider || !sawScope || declaredCount === null) throw new Error('integrity metadata is missing required fields');
  if (declaredCount !== expected.size) throw new Error('integrity metadata section count mismatch');

  let covered = 0;
  for (const section of pkg.sections) {
    if (isIntegrityAuthSection(section)) continue;
    const entry = expected.get(integrityKey(section.type, section.name));
    if (!entry) throw new Error('integrity metadata does not cover section: ' + section.name);
    covered++;
  }
  if (covered !== expected.size) throw new Error('integrity metadata references missing sections');
  pkg.integrityMap = expected;
  for (const section of pkg.sections) {
    if (section.decoded && !isIntegrityAuthSection(section)) {
      section.integrityVerified = false;
      verifyLazySectionDigest(pkg, section);
    }
  }
}


function verifyPackageFeatures(pkg) {
  const metadata = pkg.sections.find((section) => section.type === SECTION.PACKAGE_FEATURES && sectionNameMatches(SECTION.PACKAGE_FEATURES, section.name, 'package-features.vfeat'));
  const release = (pkg.flags & PACKAGE_FLAG.RELEASE) !== 0;
  if (!metadata) {
    if (release) throw new Error('release package is missing package-features.vfeat');
    return Object.freeze({ version: 0, packageVersion: pkg.version, featureCount: 0, requiredInRelease: 0, features: Object.freeze([]) });
  }
  const lines = textDecoder.decode(metadata.data).split(/\r?\n/).filter((line) => line.length > 0);
  if (lines[0] !== 'VENOM_PACKAGE_FEATURES_V2') throw new Error('invalid package features metadata header');
  let version = 0;
  let packageVersion = 0;
  let declaredCount = null;
  let requiredInRelease = 0;
  const features = [];
  const digestMap = new Map();
  for (const line of lines.slice(1)) {
    if (line.startsWith('version=')) { version = Number.parseInt(line.slice('version='.length), 10) >>> 0; continue; }
    if (line.startsWith('package_version=')) { packageVersion = Number.parseInt(line.slice('package_version='.length), 10) >>> 0; continue; }
    if (line.startsWith('feature_count=')) { declaredCount = Number.parseInt(line.slice('feature_count='.length), 10) >>> 0; continue; }
    if (!line.startsWith('feature\t')) continue;
    const parts = line.split('\t');
    if (parts.length !== 8) throw new Error('invalid package feature entry');
    const type = Number.parseInt(parts[5], 10) >>> 0;
    const name = parts[6];
    const digest = parts[7].toLowerCase();
    if (!/^[0-9a-f]{64}$/i.test(digest)) throw new Error('invalid package feature SHA-256 digest');
    const section = pkg.sections.find((candidate) => candidate.type === type && sectionNameMatches(type, candidate.name, name));
    if (!section) throw new Error('package feature references missing section: ' + name);
    digestMap.set(integrityKey(section.type, section.name), digest);
    const required = parts[4] === 'true';
    if (required) requiredInRelease++;
    features.push(Object.freeze({ id: Number.parseInt(parts[1], 10) >>> 0, name: parts[2], featureVersion: parts[3], requiredInRelease: required, sectionType: type, sectionName: name, digest }));
  }
  if (version !== 2 || packageVersion !== 40 || declaredCount === null) throw new Error('package features metadata is missing required fields');
  if (declaredCount !== features.length) throw new Error('package features count mismatch');
  if (release && requiredInRelease === 0) throw new Error('release package feature table has no required features');
  pkg.featureDigestMap = digestMap;
  for (const section of pkg.sections) {
    if (section.decoded) verifyLazySectionDigest(pkg, section);
  }
  pkg.features = Object.freeze({ version, packageVersion, featureCount: features.length, requiredInRelease, features: Object.freeze(features) });
  return pkg.features;
}


async function loadPackage(url, expectedPackageHash = null, suppliedBytes = null) {
  let bytes;
  if (suppliedBytes instanceof ArrayBuffer) bytes = new Uint8Array(suppliedBytes);
  else if (ArrayBuffer.isView(suppliedBytes)) bytes = new Uint8Array(suppliedBytes.buffer, suppliedBytes.byteOffset, suppliedBytes.byteLength);
  else {
    const res = await fetch(url, { cache: 'force-cache' });
    if (!res.ok) throw new Error('failed to load package: ' + res.status);
    bytes = new Uint8Array(await res.arrayBuffer());
  }
  if (venomPackageParser) return venomPackageParser(bytes, expectedPackageHash);
  if (bytes.length < 80) throw new Error('package is smaller than the VBC header');
  requireAscii(bytes, 'VENOMVBC', 'package');

  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const pkg = {
    bytes,
    view,
    version: readU32(view, 8),
    runtimeAbi: readU32(view, 12),
    flags: readU32(view, 16),
    sectionCount: readU32(view, 20),
    sectionTableOffset: readU64(view, 24),
    sectionTableSize: readU64(view, 32),
    nameTableOffset: readU64(view, 40),
    nameTableSize: readU64(view, 48),
    payloadOffset: readU64(view, 56),
    payloadSize: readU64(view, 64),
    packageHash: readHash64(view, 72),
    sections: [],
    decodedSectionCount: 0,
    integrityRequired: false,
    integrityMap: null,
    featureDigestMap: null,
  };

  if (pkg.version !== 40) throw new Error('unsupported VBC package version: ' + pkg.version);
  if (pkg.runtimeAbi !== 1) throw new Error('unsupported VBC runtime ABI: ' + pkg.runtimeAbi);
  const knownPackageFlags = PACKAGE_FLAG.PROTECT | PACKAGE_FLAG.RELEASE | PACKAGE_FLAG.POLYMORPHIC | PACKAGE_FLAG.COMPRESSED_SECTIONS | PACKAGE_FLAG.CRYPTO_PROVIDER_READY | PACKAGE_FLAG.INTEGRITY_METADATA | PACKAGE_FLAG.AEAD_PROVIDER_READY | PACKAGE_FLAG.RUNTIME_HARDENED | PACKAGE_FLAG.WASM_RUNTIME | PACKAGE_FLAG.HOST_BRIDGE | PACKAGE_FLAG.BINARY_DOM_OPS | PACKAGE_FLAG.FETCH_BRIDGE | PACKAGE_FLAG.ASYNC_HOST_QUEUE | PACKAGE_FLAG.TIMER_BRIDGE | PACKAGE_FLAG.EVENT_QUEUE | PACKAGE_FLAG.TURBOJS_BRIDGE | PACKAGE_FLAG.SCRIPT_ISOLATION | PACKAGE_FLAG.SCRIPT_POLICY | PACKAGE_FLAG.TURBOJS_CHUNKS | PACKAGE_FLAG.TURBOJS_ENGINE | PACKAGE_FLAG.SCRIPT_ENGINE_FALLBACK | PACKAGE_FLAG.TURBOJS_ENGINE_MODULE | PACKAGE_FLAG.TURBOJS_CONTEXT_LIFECYCLE | PACKAGE_FLAG.HOST_CAPABILITIES | PACKAGE_FLAG.TURBOJS_ADAPTER_DIAGNOSTICS | PACKAGE_FLAG.TURBOJS_WASM_RUNTIME | PACKAGE_FLAG.TURBOJS_SOURCE_TRANSFER | PACKAGE_FLAG.TURBOJS_CONSOLE_BRIDGE | PACKAGE_FLAG.TURBOJS_EXECUTION_RECORDS | PACKAGE_FLAG.TURBOJS_RESULT_BRIDGE | PACKAGE_FLAG.TURBOJS_FALLBACK_POLICY | PACKAGE_FLAG.TURBOJS_ENGINE_BACKEND;
  if ((pkg.flags & ~knownPackageFlags) !== 0) throw new Error('unknown package flags: ' + (pkg.flags & ~knownPackageFlags));
  if ((pkg.flags & PACKAGE_FLAG.RELEASE) !== 0) {
    if ((pkg.flags & PACKAGE_FLAG.POLYMORPHIC) === 0) throw new Error('release package is missing polymorphic metadata');
    if ((pkg.flags & PACKAGE_FLAG.CRYPTO_PROVIDER_READY) === 0) throw new Error('release package is missing crypto-provider-ready metadata');
    if ((pkg.flags & PACKAGE_FLAG.INTEGRITY_METADATA) === 0) throw new Error('release package is missing integrity metadata');
    if ((pkg.flags & PACKAGE_FLAG.RUNTIME_HARDENED) === 0) throw new Error('release package is missing runtime hardening metadata');
  }
  if (pkg.sectionTableOffset !== 80) throw new Error('invalid section table offset');
  if (pkg.sectionTableSize !== pkg.sectionCount * 48) throw new Error('invalid section table size');

  const computedPackageHash = fnv1a64(bytes, true);
  if (computedPackageHash !== pkg.packageHash) throw new Error('package hash mismatch');
  if (expectedPackageHash !== null && expectedPackageHash !== undefined && String(expectedPackageHash).length > 0) {
    const expected = BigInt(String(expectedPackageHash));
    if (expected !== computedPackageHash) throw new Error('package hash allowlist mismatch');
  }

  for (let i = 0; i < pkg.sectionCount; ++i) {
    const base = pkg.sectionTableOffset + i * 48;
    const type = readU32(view, base + 0);
    if (type < SECTION.MANIFEST || type > SECTION.PACKAGE_FEATURES) throw new Error('unknown package section type: ' + type);
    const flags = readU32(view, base + 4);
    const nameOffset = readU32(view, base + 8);
    const nameSize = readU32(view, base + 12);
    const dataOffset = readU64(view, base + 16);
    const dataSize = readU64(view, base + 24);
    const rawSize = readU64(view, base + 32);
    const hash = readHash64(view, base + 40);

    const knownSectionFlags = SECTION_FLAG.COMPRESSED | SECTION_FLAG.ENCRYPTED;
    if ((flags & ~knownSectionFlags) !== 0) throw new Error('unknown section flags: ' + (flags & ~knownSectionFlags));
    assertRange(pkg.nameTableOffset + nameOffset, nameSize, bytes.length, 'section name');
    assertRange(dataOffset, dataSize, bytes.length, 'section data');
    if (dataOffset < pkg.payloadOffset || dataOffset + dataSize > pkg.payloadOffset + pkg.payloadSize) {
      throw new Error('section payload range is invalid');
    }
    const encrypted = (flags & SECTION_FLAG.ENCRYPTED) !== 0;
    if ((flags & SECTION_FLAG.COMPRESSED) !== 0) {
      if (!encrypted && rawSize < dataSize) throw new Error('compressed section raw size is smaller than stored size');
    } else if (!encrypted && rawSize !== dataSize) {
      throw new Error('uncompressed section raw size mismatch');
    }

    const name = decodeName(bytes, pkg.nameTableOffset + nameOffset, nameSize);
    const storedData = bytes.subarray(dataOffset, dataOffset + dataSize);
    if (fnv1a64(storedData) !== hash) throw new Error('section hash mismatch: ' + name);
    pkg.sections.push(makeLazySection(pkg, i, type, flags, name, dataOffset, dataSize, rawSize, hash, storedData));
  }

  verifyPackageFeatures(pkg);
  verifyIntegrityMetadata(pkg);
  return pkg;
}


let activeReleaseDiversification = null;
let activeTurboJsAbiFingerprint = null;
let activeRuntimeIntegritySeal = 0;
let activeRuntimeOpcodeMap = null;
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

function createAsyncHostQueue(fetchPlan, queuePlan, timerPlan = null, turboJsPlan = null) {
  let nextId = 1;
  let nextTimerId = 1;
  const pending = new Map();
  const completed = new Map();
  const timers = new Map();
  const maxPending = queuePlan && queuePlan.maxPending ? queuePlan.maxPending : 1024;
  const maxTimers = timerPlan && timerPlan.maxActive ? timerPlan.maxActive : 256;
  const maxCompleted = queuePlan && queuePlan.maxCompleted ? queuePlan.maxCompleted : 256;
  const maxFetchResponseBytes = queuePlan && queuePlan.maxFetchResponseBytes ? queuePlan.maxFetchResponseBytes : 1048576;
  const maxFetchRequestBytes = queuePlan && queuePlan.maxFetchRequestBytes ? queuePlan.maxFetchRequestBytes : 65536;
  const maxTimerDelayMs = queuePlan && queuePlan.maxTimerDelayMs ? queuePlan.maxTimerDelayMs : 86400000;
  const maxRouteLifetimeMs = queuePlan && queuePlan.maxRouteLifetimeMs ? queuePlan.maxRouteLifetimeMs : 86400000;
  const maxHostCallsPerRoute = queuePlan && queuePlan.maxHostCallsPerRoute ? queuePlan.maxHostCallsPerRoute : 8192;
  const maxConcurrentFetches = queuePlan && queuePlan.maxConcurrentFetches ? queuePlan.maxConcurrentFetches : 16;
  const maxScheduledPerRoute = timerPlan && timerPlan.maxScheduledPerRoute ? timerPlan.maxScheduledPerRoute : 512;
  const abortControllers = new Map();
  let activeFetches = 0;
  let disposed = false;
  let activeGeneration = 0;
  let routeStartedAt = Date.now ? Date.now() : 0;
  let hostCallsThisRoute = 0;
  let fetchesThisRoute = 0;
  let timersScheduledThisRoute = 0;

  function assertRouteLifetime(label = 'route operation') {
    const now = Date.now ? Date.now() : routeStartedAt;
    if (maxRouteLifetimeMs && now - routeStartedAt > maxRouteLifetimeMs) throw new Error('Venom route lifetime limit exceeded for ' + label);
  }

  function snapshot() {
    return Object.freeze({ pending: pending.size, completed: completed.size, timers: timers.size, activeFetches, nextId, nextTimerId, generation: activeGeneration, hostCallsThisRoute, fetchesThisRoute, timersScheduledThisRoute, routeAgeMs: Math.max(0, (Date.now ? Date.now() : routeStartedAt) - routeStartedAt), limits: Object.freeze({ maxPending, maxCompleted, maxTimers, maxHostCallsPerRoute, maxConcurrentFetches, maxScheduledPerRoute, maxRouteLifetimeMs }) });
  }

  function enqueue(kind, payload = {}) {
    if (disposed) throw new Error('Venom async host queue is disposed');
    assertRouteLifetime(kind || 'host.call');
    if (hostCallsThisRoute >= maxHostCallsPerRoute) throw new Error('Venom per-route host-call budget exceeded');
    if (pending.size >= maxPending) throw new Error('Venom async host-call queue capacity exceeded');
    const id = nextId++ >>> 0;
    const record = { id, kind: String(kind || 'host.call'), state: 'pending', payload, generation: activeGeneration, createdAt: Date.now ? Date.now() : 0 };
    pending.set(id, record);
    hostCallsThisRoute++;
    return Object.freeze({ id, kind: record.kind, state: record.state });
  }

  function assertGeneration(generation, label = 'async operation') {
    if ((generation >>> 0) !== (activeGeneration >>> 0)) throw new Error('stale route generation rejected for ' + label);
  }

  function settle(id, state, value) {
    const record = pending.get(id >>> 0);
    if (!record) throw new Error('unknown Venom async host-call id: ' + id);
    assertGeneration(record.generation, record.kind);
    pending.delete(id >>> 0);
    const completedRecord = Object.freeze({ ...record, state, value, completedAt: Date.now ? Date.now() : 0 });
    completed.set(id >>> 0, completedRecord);
    while (completed.size > maxCompleted) completed.delete(completed.keys().next().value);
    return completedRecord;
  }

  async function requestFetch(input, init = {}) {
    if (!fetchPlan || !fetchPlan.enabled) throw new Error('Venom fetch bridge is disabled');
    if (typeof globalThis.fetch !== 'function') throw new Error('global fetch is not available');
    assertRouteLifetime('fetch request');
    if (activeFetches >= maxConcurrentFetches) throw new Error('Venom concurrent fetch limit exceeded');
    const requestBytes = new TextEncoder().encode(String(input) + JSON.stringify(init || {})).byteLength;
    if (requestBytes > maxFetchRequestBytes) throw new Error('Venom fetch request size limit exceeded');
    const controller = typeof AbortController === 'function' ? new AbortController() : null;
    const requestGeneration = activeGeneration;
    const request = enqueue(fetchPlan.requestHostCall || 'fetch.request', { input: String(input), init });
    fetchesThisRoute++;
    activeFetches++;
    if (controller) abortControllers.set(request.id, controller);
    try {
      const authorizedUrl = authorizeHostUrl(input, { sameOriginOnly: true });
      const response = await globalThis.fetch(authorizedUrl.href, controller ? { ...init, signal: controller.signal, credentials: 'same-origin', redirect: 'error' } : { ...init, credentials: 'same-origin', redirect: 'error' });
      assertGeneration(requestGeneration, 'fetch response');
      const headers = {};
      if (response && response.headers && typeof response.headers.forEach === 'function') {
        response.headers.forEach((value, key) => { headers[key] = value; });
      }
      const bridgeHeaders = Object.freeze({
        ...headers,
        get: (name) => { const key = String(name || '').toLowerCase(); return Object.prototype.hasOwnProperty.call(headers, key) ? headers[key] : (Object.prototype.hasOwnProperty.call(headers, String(name || '')) ? headers[String(name || '')] : null); },
        forEach: (cb) => { if (typeof cb === 'function') { for (const [key, value] of Object.entries(headers)) cb(value, key); } },
      });
      const bridgeResponse = {
        ok: !!(response && response.ok),
        status: response && typeof response.status === 'number' ? response.status : 0,
        statusText: response && response.statusText ? String(response.statusText) : '',
        url: response && response.url ? String(response.url) : authorizedUrl.href,
        headers: bridgeHeaders,
        text: async () => { assertGeneration(requestGeneration, 'fetch text'); const value = response && typeof response.text === 'function' ? await response.text() : ''; assertGeneration(requestGeneration, 'fetch text'); if (new TextEncoder().encode(value).byteLength > maxFetchResponseBytes) throw new Error('Venom fetch response size limit exceeded'); return value; },
        json: async () => { assertGeneration(requestGeneration, 'fetch json'); const value = response && typeof response.json === 'function' ? await response.json() : JSON.parse(await bridgeResponse.text()); assertGeneration(requestGeneration, 'fetch json'); return value; },
        arrayBuffer: async () => { assertGeneration(requestGeneration, 'fetch arrayBuffer'); const value = response && typeof response.arrayBuffer === 'function' ? await response.arrayBuffer() : new ArrayBuffer(0); assertGeneration(requestGeneration, 'fetch arrayBuffer'); if (value.byteLength > maxFetchResponseBytes) throw new Error('Venom fetch response size limit exceeded'); return value; },
        raw: response,
        venomRequestId: request.id,
      };
      abortControllers.delete(request.id);
      activeFetches = Math.max(0, activeFetches - 1);
      settle(request.id, 'fulfilled', { status: bridgeResponse.status, ok: bridgeResponse.ok });
      return Object.freeze(bridgeResponse);
    } catch (error) {
      abortControllers.delete(request.id);
      activeFetches = Math.max(0, activeFetches - 1);
      if (pending.has(request.id) && (requestGeneration >>> 0) === (activeGeneration >>> 0)) {
        settle(request.id, 'rejected', { message: error && error.message ? error.message : String(error) });
      }
      throw error;
    }
  }

  function scheduleTimer(callback, delay = 0, repeat = false) {
    if (!timerPlan || !timerPlan.enabled) throw new Error('Venom timer bridge is disabled');
    assertRouteLifetime('timer schedule');
    if (timers.size >= maxTimers) throw new Error('Venom timer bridge capacity exceeded');
    if (timersScheduledThisRoute >= maxScheduledPerRoute) throw new Error('Venom per-route timer schedule budget exceeded');
    const id = nextTimerId++ >>> 0;
    timersScheduledThisRoute++;
    const ms = Math.min(maxTimerDelayMs, Math.max(Number(delay) || 0, timerPlan.minimumDelayMs || 0));
    const timerGeneration = activeGeneration;
    const request = enqueue(timerPlan.scheduleHostCall || 'timer.schedule', { timerId: id, delayMs: ms, repeat: !!repeat });
    const runner = () => {
      if ((timerGeneration >>> 0) !== (activeGeneration >>> 0)) {
        if (repeat && typeof globalThis.clearInterval === 'function') globalThis.clearInterval(nativeId);
        if (!repeat && typeof globalThis.clearTimeout === 'function') globalThis.clearTimeout(nativeId);
        timers.delete(id);
        return;
      }
      try {
        if (typeof callback === 'function') callback();
        if (!repeat) {
          timers.delete(id);
          settle(request.id, 'fulfilled', { timerId: id, fired: true });
        }
      } catch (error) {
        if (!repeat && pending.has(request.id)) settle(request.id, 'rejected', { timerId: id, message: error && error.message ? error.message : String(error) });
        throw error;
      }
    };
    const nativeId = repeat ? globalThis.setInterval(runner, ms) : globalThis.setTimeout(runner, ms);
    timers.set(id, Object.freeze({ id, nativeId, repeat: !!repeat, requestId: request.id, delayMs: ms, generation: timerGeneration }));
    return Object.freeze({ id, requestId: request.id, state: 'scheduled', repeat: !!repeat, delayMs: ms });
  }

  function cancelTimer(id) {
    const record = timers.get(id >>> 0);
    if (!record) return Object.freeze({ id: id >>> 0, cancelled: false });
    timers.delete(id >>> 0);
    if (record.repeat && typeof globalThis.clearInterval === 'function') globalThis.clearInterval(record.nativeId);
    if (!record.repeat && typeof globalThis.clearTimeout === 'function') globalThis.clearTimeout(record.nativeId);
    if (pending.has(record.requestId)) settle(record.requestId, 'fulfilled', { timerId: record.id, cancelled: true });
    const cancelRecord = enqueue(timerPlan && timerPlan.cancelHostCall ? timerPlan.cancelHostCall : 'timer.cancel', { timerId: record.id });
    settle(cancelRecord.id, 'fulfilled', { timerId: record.id, cancelled: true });
    return Object.freeze({ id: record.id, cancelled: true });
  }

  function clearRouteResources(reason = 'route-disposed', finalDispose = false) {
    if (disposed) return;
    if (finalDispose) disposed = true;
    for (const record of timers.values()) {
      if (record.repeat && typeof globalThis.clearInterval === 'function') globalThis.clearInterval(record.nativeId);
      if (!record.repeat && typeof globalThis.clearTimeout === 'function') globalThis.clearTimeout(record.nativeId);
    }
    timers.clear();
    for (const controller of abortControllers.values()) { try { controller.abort(reason); } catch (_) {} }
    abortControllers.clear();
    activeFetches = 0;
    for (const [id] of pending) {
      const record = pending.get(id); pending.delete(id); completed.set(id, Object.freeze({ ...record, state: 'rejected', value: { message: reason } }));
    }
    while (completed.size > maxCompleted) completed.delete(completed.keys().next().value);
  }

  function setRouteGeneration(generation, reason = 'route-generation-change') {
    const next = generation >>> 0;
    if (next === (activeGeneration >>> 0)) return activeGeneration;
    clearRouteResources(reason, false);
    activeGeneration = next;
    routeStartedAt = Date.now ? Date.now() : 0;
    hostCallsThisRoute = 0;
    fetchesThisRoute = 0;
    timersScheduledThisRoute = 0;
    return activeGeneration;
  }
  function resetRoute(reason = 'route-reset') { clearRouteResources(reason, false); activeGeneration = (activeGeneration + 1) >>> 0; routeStartedAt = Date.now ? Date.now() : 0; hostCallsThisRoute = 0; fetchesThisRoute = 0; timersScheduledThisRoute = 0; return activeGeneration; }
  function dispose(reason = 'queue-disposed') { clearRouteResources(reason, true); }

  function callTurboJs(entry, payload = {}) {
    const request = enqueue(turboJsPlan && turboJsPlan.callHostCall ? turboJsPlan.callHostCall : 'turbojs.call', { entry: String(entry || ''), payload });
    const mode = turboJsPlan && turboJsPlan.mode ? turboJsPlan.mode : 'planned-boundary';
    return Object.freeze({
      id: request.id,
      state: request.state,
      mode,
      scriptIsolation: turboJsPlan && turboJsPlan.scriptIsolation ? turboJsPlan.scriptIsolation : 'route-scoped',
      bytecodeInput: turboJsPlan && turboJsPlan.bytecodeInput ? turboJsPlan.bytecodeInput : 'planned',
      chunkMetadata: turboJsPlan && turboJsPlan.chunkMetadata ? turboJsPlan.chunkMetadata : '',
      engineMetadata: turboJsPlan && turboJsPlan.engineMetadata ? turboJsPlan.engineMetadata : '',
      fallbackPolicy: turboJsPlan && turboJsPlan.fallbackPolicy ? turboJsPlan.fallbackPolicy : 'host-js-isolated-wrapper',
    });
  }

  return Object.freeze({ enqueue, settle, snapshot, requestFetch, scheduleTimer, cancelTimer, callTurboJs, setRouteGeneration, resetRoute, dispose });
}

function createEventQueue(eventPlan) {
  const records = [];
  const maxRecords = eventPlan && eventPlan.maxRecords ? eventPlan.maxRecords : 256;
  const maxDispatchesPerRoute = eventPlan && eventPlan.maxDispatchesPerRoute ? eventPlan.maxDispatchesPerRoute : 1024;
  let dispatches = 0;
  function push(record = {}) {
    if (!eventPlan || !eventPlan.enabled) return Object.freeze({ queued: false, size: records.length });
    if (dispatches >= maxDispatchesPerRoute) throw new Error('Venom per-route event dispatch budget exceeded');
    if (records.length >= maxRecords) records.shift();
    dispatches++;
    const item = Object.freeze({
      id: records.length + 1,
      eventName: record && record.eventName ? String(record.eventName) : '',
      attrName: record && record.attrName ? String(record.attrName) : '',
      route: globalThis.location && globalThis.location.pathname ? String(globalThis.location.pathname) : '/',
      targetTag: record && record.element && record.element.tagName ? String(record.element.tagName).toLowerCase() : '',
      createdAt: Date.now ? Date.now() : 0,
    });
    records.push(item);
    return Object.freeze({ queued: true, size: records.length, record: item });
  }
  function snapshot() {
    return Object.freeze({ size: records.length, maxRecords, dispatches, maxDispatchesPerRoute, last: records.length ? records[records.length - 1] : null });
  }
  function clear() { const count = records.length; records.length = 0; dispatches = 0; return count; }
  return Object.freeze({ push, snapshot, clear });
}

function isInlineEventAttribute(name) {
  return /^on[a-z][a-z0-9_:-]*$/i.test(String(name || ''));
}

function eventNameFromAttribute(name) {
  return String(name || '').slice(2).toLowerCase();
}

function bindInlineEventAttribute(target, attrName, source, hostBridgePlan = null) {
  if (!target || !isInlineEventAttribute(attrName)) return false;
  const eventName = eventNameFromAttribute(attrName);
  if (!eventName) return false;
  if (typeof target.setAttribute === 'function') {
    target.setAttribute('data-venom-event-' + eventName, 'inline');
  }
  if (typeof target.addEventListener === 'function') {
    const bindBridge = globalThis.__venomRuntime || null;
    const boundGeneration = bindBridge && typeof bindBridge.currentRouteGeneration === 'function' ? bindBridge.currentRouteGeneration() : 0;
    target.addEventListener(eventName, function venomInlineEventHandler(event) {
      const bridge = globalThis.__venomRuntime || null;
      if (!bridge || typeof bridge.isRouteGenerationActive !== 'function' || !bridge.isRouteGenerationActive(boundGeneration)) return undefined;
      if (typeof bridge.dispatchEventBinding === 'function') {
        bridge.dispatchEventBinding({ event, element: target, eventName, source, attrName, routeGeneration: boundGeneration });
      }
      // Protected runtime never evaluates inline event source in the host runtime.
      // The source is treated as metadata and must be dispatched through TurboJS.
      return undefined;
    });
  }
  return true;
}


function createTurboJsEngine(asyncQueue, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsBridgePlan, turboJsEngineModulePlan = null, turboJsEngineModuleUrl = null, turboJsContextLifecyclePlan = null, hostCapabilitiesPlan = null, turboJsAdapterDiagnosticsPlan = null, turboJsWasmRuntimePlan = null, turboJsWasmUrl = null, turboJsSourceTransferPlan = null, turboJsConsoleBridgePlan = null, turboJsExecutionRecordsPlan = null, turboJsResultBridgePlan = null, turboJsFallbackPolicyPlan = null, turboJsRuntimeAbiPlan = null, turboJsHostImportsPlan = null, turboJsHeapLimitsPlan = null, turboJsScriptBufferPlan = null, turboJsConsoleAbiPlan = null, turboJsParityProbePlan = null, turboJsReleaseFallbackPlan = null, turboJsBytecodeManifestPlan = null, turboJsModuleResolverPlan = null, turboJsExceptionAbiPlan = null, turboJsHostTrapPolicyPlan = null, turboJsExecutionLifecyclePlan = null, turboJsScriptBufferPolicyPlan = null, turboJsContextLimitPolicyPlan = null, turboJsHostCallDispatchPlan = null, turboJsParityContractPlan = null, turboJsReleaseFailClosedPlan = null, turboJsModuleGraphPlan = null, turboJsModuleExecutionPlan = null, turboJsModuleCachePlan = null, turboJsResolverAuditPlan = null, turboJsInteropFallbackPlan = null, turboJsExecutionJournalPlan = null, turboJsCheckpointPolicyPlan = null, turboJsReplayCursorPlan = null, turboJsResumeStatePlan = null, turboJsDeterminismAuditPlan = null, turboJsSnapshotPolicyPlan = null, turboJsSnapshotRecordsPlan = null, turboJsReplayValidationPlan = null, turboJsDeterminismLedgerPlan = null, turboJsAuditSealPlan = null, turboJsExecutionCommitPlan = null, turboJsRollbackPolicyPlan = null, turboJsHostCallReceiptsPlan = null, turboJsReleaseAcceptancePlan = null, turboJsCommitAuditPlan = null, turboJsCapabilityPolicyPlan = null, turboJsHostIoPolicyPlan = null, turboJsPermissionSealPlan = null, turboJsPolicyReceiptsPlan = null, turboJsReleaseGatePlan = null, turboJsHostIoDecisionPlan = null, turboJsHostIoDenyTracePlan = null, turboJsCapabilityLedgerPlan = null, turboJsPolicySealAuditPlan = null, turboJsRuntimeDenylistPlan = null) {
  const contexts = new Map();
  const enabled = !!(turboJsEnginePlan && turboJsEnginePlan.enabled);
  const mode = enabled ? (turboJsEnginePlan.mode || 'bootstrap') : 'disabled';
  let engineModulePromise = null;
  let engineModule = null;
  let engineModuleError = null;
  const executionRecords = [];
  const consoleEvents = [];
  const cachedSnapshotPolicy = null;
  const cachedSnapshotRecord = null;
  const cachedReplayValidation = null;
  const cachedDeterminismLedger = null;
  const cachedAuditSeal = null;

  function recordTurboJsExecution(kind, payload = {}) {
    const record = Object.freeze({ id: executionRecords.length + 1, kind: String(kind || 'turbojs.executionRecord'), at: Date.now ? Date.now() : 0, ...payload });
    executionRecords.push(record);
    if (asyncQueue && turboJsExecutionRecordsPlan && turboJsExecutionRecordsPlan.enabled) {
      const request = asyncQueue.enqueue(turboJsExecutionRecordsPlan.recordHostCall || 'turbojs.executionRecord', record);
      asyncQueue.settle(request.id, 'fulfilled', { recorded: record.id, kind: record.kind });
    }
    return record;
  }

  function recordResultBridge(payload = {}) {
    const record = Object.freeze({ id: executionRecords.length + 1, kind: 'turbojs.resultBridge', at: Date.now ? Date.now() : 0, ...payload });
    executionRecords.push(record);
    if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) {
      const request = asyncQueue.enqueue(turboJsResultBridgePlan.resultHostCall || 'turbojs.resultBridge', record);
      asyncQueue.settle(request.id, 'fulfilled', { bridged: record.id, ok: payload.ok !== false });
    }
    return record;
  }

  function makeConsoleBridgeConsole(baseConsole, chunk) {
    const source = chunk && chunk.source ? String(chunk.source) : '';
    const route = chunk && chunk.route ? String(chunk.route) : '';
    const order = chunk && chunk.order ? chunk.order >>> 0 : 0;
    const out = Object.create(null);
    for (const level of ['debug', 'log', 'info', 'warn', 'error']) {
      out[level] = (...args) => {
        const event = Object.freeze({ id: consoleEvents.length + 1, level, route, source, order, args: args.map((arg) => { try { return String(arg); } catch (_) { return '[unprintable]'; } }) });
        consoleEvents.push(event);
        if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) {
          const request = asyncQueue.enqueue(turboJsResultBridgePlan.consoleEventHostCall || 'turbojs.consoleEvent', event);
          asyncQueue.settle(request.id, 'fulfilled', { consoleEvent: event.id });
        }
        if (baseConsole && typeof baseConsole[level] === 'function') baseConsole[level](...args);
      };
    }
    return Object.freeze(out);
  }

  function fallbackAllowed(reason) {
    const mode = turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled ? turboJsFallbackPolicyPlan.mode : 'legacy';
    const denyList = String((turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.denyWhen) || '') + '|' + String((turboJsReleaseFallbackPlan && turboJsReleaseFallbackPlan.denyWhen) || '');
    const strict = !!((turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.strictRelease) || (turboJsReleaseFallbackPlan && turboJsReleaseFallbackPlan.enabled && turboJsReleaseFallbackPlan.denyHostFallback));
    const wasmOrModuleFallback = reason === 'engine-module-unavailable-or-compatible-fallback' || reason === 'wasm-interpreter-unavailable' || reason === 'release-strict-no-fallback';
    const allowed = !(strict && wasmOrModuleFallback) && !denyList.split('|').filter(Boolean).includes(reason);
    if (asyncQueue && turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled) {
      const request = asyncQueue.enqueue(turboJsFallbackPolicyPlan.policyCheckHostCall || 'turbojs.fallbackPolicyCheck', { reason, mode, strict, allowed });
      asyncQueue.settle(request.id, allowed ? 'fulfilled' : 'rejected', { allowed, reason, strict });
    }
    return allowed;
  }

  function contextKey(route, source, order, generation = asyncQueue.snapshot().generation) {
    return String(generation >>> 0) + '::' + String(route || '/') + '::' + String(source || '') + '::' + String(order >>> 0);
  }

  function createContext(route, source, order) {
    const key = contextKey(route, source, order);
    let ctx = contexts.get(key);
    if (!ctx) {
      const request = asyncQueue.enqueue(turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.createHostCall ? turboJsContextLifecyclePlan.createHostCall : (turboJsEnginePlan && turboJsEnginePlan.contextCreateHostCall ? turboJsEnginePlan.contextCreateHostCall : 'turbojs.contextCreate'), { route: String(route || '/'), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(request.id, 'fulfilled', { context: key, mode, engineModule: !!turboJsEngineModuleUrl });
      ctx = Object.freeze({ key, generation: asyncQueue.snapshot().generation >>> 0, route: String(route || '/'), source: String(source || ''), order: order >>> 0, mode, createdAt: Date.now ? Date.now() : 0, reuseCount: 0 });
      contexts.set(key, ctx);
    } else if (turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.enabled) {
      const request = asyncQueue.enqueue(turboJsContextLifecyclePlan.reuseHostCall || 'turbojs.contextReuse', { context: key, route: String(route || '/'), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(request.id, 'fulfilled', { context: key, reused: true });
    }
    return ctx;
  }

  function destroyContext(route, source, order) {
    const key = contextKey(route, source, order);
    const existed = contexts.delete(key);
    const request = asyncQueue.enqueue(turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.destroyHostCall ? turboJsContextLifecyclePlan.destroyHostCall : 'turbojs.contextDestroy', { context: key, existed });
    asyncQueue.settle(request.id, 'fulfilled', { context: key, destroyed: existed });
    return Object.freeze({ context: key, destroyed: existed });
  }

  function destroyRoute(route) {
    const routeMarker = '::' + String(route || '/') + '::';
    let destroyed = 0;
    for (const key of Array.from(contexts.keys())) {
      if (!key.includes(routeMarker)) continue;
      contexts.delete(key);
      destroyed++;
    }
    return Object.freeze({ route: String(route || '/'), destroyed });
  }

  function contextSnapshot() {
    return Object.freeze({ count: contexts.size, keys: Object.freeze(Array.from(contexts.keys())), moduleLoaded: !!engineModule, moduleError: engineModuleError ? String(engineModuleError.message || engineModuleError) : '', wasmUrl: turboJsWasmUrl || '' });
  }

  function recordBrowserApiShimCall(kind, chunk, detail) {
    const payload = Object.freeze({
      kind: String(kind || 'unknown'),
      route: chunk && chunk.route ? String(chunk.route) : '',
      source: chunk && chunk.source ? String(chunk.source) : '',
      order: chunk && chunk.order ? chunk.order >>> 0 : 0,
      ...(detail || {}),
    });
    if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) {
      const request = asyncQueue.enqueue('turbojs.browserApiShim', payload);
      asyncQueue.settle(request.id, 'fulfilled', payload);
    }
    return payload;
  }

  const guardedMissingGlobalNames = Object.freeze(['fpCollect']);

  function createGuardedMissingGlobalFunction(name, nativeGlobal, bridge, chunk) {
    const shim = function venomMissingGlobalFunctionShim(...args) {
      const target = nativeGlobal || globalThis;
      let nativeFn = null;
      try { nativeFn = target && target[name]; } catch (_) { nativeFn = null; }
      if (typeof nativeFn !== 'function') { try { nativeFn = target && target.window && target.window[name]; } catch (_) { nativeFn = null; } }
      if (typeof nativeFn !== 'function') { try { nativeFn = target && target.self && target.self[name]; } catch (_) { nativeFn = null; } }
      if (typeof nativeFn === 'function' && nativeFn !== shim) {
        try {
          const result = nativeFn.apply(target, args);
          recordBrowserApiShimCall('global.' + name + '.forwarded', chunk, { api: name, actualArgs: args.length, result: 'forwarded' });
          return result;
        } catch (error) {
          recordBrowserApiShimCall('global.' + name + '.trapped', chunk, { api: name, actualArgs: args.length, result: null, message: error && error.message ? String(error.message) : String(error) });
          return null;
        }
      }
      recordBrowserApiShimCall('global.' + name + '.missing', chunk, { api: name, actualArgs: args.length, result: null });
      return null;
    };
    try { Object.defineProperty(shim, 'name', { value: 'venom_' + name + '_shim' }); } catch (_) {}
    return shim;
  }

  function createGuardedMissingGlobalShims(nativeGlobal, bridge, chunk) {
    const shims = Object.create(null);
    for (const name of guardedMissingGlobalNames) shims[name] = createGuardedMissingGlobalFunction(name, nativeGlobal, bridge, chunk);
    return Object.freeze(shims);
  }

  function getGuardedMissingGlobalShim(missingGlobalShims, prop) {
    const key = String(prop);
    return missingGlobalShims && Object.prototype.hasOwnProperty.call(missingGlobalShims, key) ? missingGlobalShims[key] : undefined;
  }

  function createNavigatorShim(nativeNavigator, bridge, chunk) {
    const target = nativeNavigator || Object.create(null);
    const mediaDeviceTarget = target && target.mediaDevices ? target.mediaDevices : Object.create(null);
    const legacyMediaAliases = ['getUserMedia', 'webkitGetUserMedia', 'mozGetUserMedia', 'msGetUserMedia'];
    function makeMediaError(api, message) {
      try {
        const error = new TypeError(message);
        error.name = 'TypeError';
        error.api = api;
        return error;
      } catch (_) {
        return { name: 'TypeError', api, message };
      }
    }
    function settleMediaErrorCallback(callback, error) {
      if (typeof callback !== 'function') return;
      try { callback(error); } catch (callbackError) {
        recordBrowserApiShimCall('navigator.getUserMedia.errorCallbackTrapped', chunk, { api: 'navigator.getUserMedia', result: false, message: callbackError && callbackError.message ? String(callbackError.message) : String(callbackError) });
      }
    }
    function findLegacyGetUserMedia(obj) {
      for (const name of legacyMediaAliases) {
        try {
          if (obj && typeof obj[name] === 'function') return { name, fn: obj[name] };
        } catch (_) {}
      }
      return { name: 'getUserMedia', fn: null };
    }
    function safeResolvedNullPromise() {
      return typeof Promise !== 'undefined' && Promise && typeof Promise.resolve === 'function' ? Promise.resolve(null) : null;
    }
    const sendBeaconShim = function venomSendBeaconShim(url, data) {
      if (arguments.length < 1) {
        recordBrowserApiShimCall('navigator.sendBeacon.arity', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: 0, result: false });
        return false;
      }
      const nativeSendBeacon = target && target.sendBeacon;
      if (typeof nativeSendBeacon !== 'function') {
        recordBrowserApiShimCall('navigator.sendBeacon.unavailable', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: arguments.length, result: false });
        return false;
      }
      try {
        const result = arguments.length > 1 ? nativeSendBeacon.call(target, url, data) : nativeSendBeacon.call(target, url);
        recordBrowserApiShimCall('navigator.sendBeacon.forwarded', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: arguments.length, result: !!result });
        return !!result;
      } catch (error) {
        recordBrowserApiShimCall('navigator.sendBeacon.trapped', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: arguments.length, result: false, message: error && error.message ? String(error.message) : String(error) });
        return false;
      }
    };
    const mediaDevicesGetUserMediaShim = function venomMediaDevicesGetUserMediaShim(constraints) {
      const api = 'navigator.mediaDevices.getUserMedia';
      if (arguments.length < 1 || constraints == null) {
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.arity', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null });
        return safeResolvedNullPromise();
      }
      const nativeMediaGetUserMedia = mediaDeviceTarget && mediaDeviceTarget.getUserMedia;
      if (typeof nativeMediaGetUserMedia !== 'function') {
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.unavailable', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null });
        return safeResolvedNullPromise();
      }
      try {
        const result = nativeMediaGetUserMedia.call(mediaDeviceTarget, constraints);
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.forwarded', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: 'promise' });
        return result && typeof result.then === 'function'
          ? result.catch((error) => {
              recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.trapped', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null, message: error && error.message ? String(error.message) : String(error) });
              return null;
            })
          : Promise.resolve(result || null);
      } catch (error) {
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.trapped', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null, message: error && error.message ? String(error.message) : String(error) });
        return safeResolvedNullPromise();
      }
    };
    function createMediaDevicesShim(nativeMediaDevices) {
      const mediaTarget = nativeMediaDevices || Object.create(null);
      if (typeof Proxy === 'undefined') {
        const shim = Object.create(mediaTarget || null);
        shim.getUserMedia = mediaDevicesGetUserMediaShim;
        return shim;
      }
      return new Proxy(mediaTarget, {
        get(obj, prop) {
          if (prop === 'getUserMedia') return mediaDevicesGetUserMediaShim;
          try {
            const value = obj ? obj[prop] : undefined;
            return typeof value === 'function' ? value.bind(obj) : value;
          } catch (_) {
            return undefined;
          }
        },
        has(obj, prop) {
          return prop === 'getUserMedia' || !!(obj && prop in obj);
        },
        set(obj, prop, value) {
          try { obj[prop] = value; return true; } catch (_) { return false; }
        },
      });
    }
    const mediaDevicesShim = createMediaDevicesShim(mediaDeviceTarget);
    const legacyGetUserMediaShim = function venomGetUserMediaShim(constraints, successCallback, errorCallback) {
      const api = 'navigator.getUserMedia';
      if (arguments.length < 3 || typeof successCallback !== 'function' || typeof errorCallback !== 'function') {
        recordBrowserApiShimCall('navigator.getUserMedia.arity', chunk, { api, requiredArgs: 3, actualArgs: arguments.length, result: false });
        settleMediaErrorCallback(errorCallback, makeMediaError(api, 'Venom guarded getUserMedia shim rejected missing constraints or callbacks'));
        return false;
      }
      const nativeLegacy = findLegacyGetUserMedia(target);
      if (typeof nativeLegacy.fn !== 'function') {
        recordBrowserApiShimCall('navigator.getUserMedia.unavailable', chunk, { api: nativeLegacy.name, requiredArgs: 3, actualArgs: arguments.length, result: false });
        settleMediaErrorCallback(errorCallback, makeMediaError(api, 'getUserMedia is unavailable in this runtime'));
        return false;
      }
      try {
        const result = nativeLegacy.fn.call(target, constraints, successCallback, errorCallback);
        recordBrowserApiShimCall('navigator.getUserMedia.forwarded', chunk, { api: nativeLegacy.name, requiredArgs: 3, actualArgs: arguments.length, result: true });
        return result;
      } catch (error) {
        recordBrowserApiShimCall('navigator.getUserMedia.trapped', chunk, { api: nativeLegacy.name, requiredArgs: 3, actualArgs: arguments.length, result: false, message: error && error.message ? String(error.message) : String(error) });
        settleMediaErrorCallback(errorCallback, error);
        return false;
      }
    };
    if (typeof Proxy === 'undefined') {
      const shim = Object.create(target || null);
      shim.sendBeacon = sendBeaconShim;
      shim.mediaDevices = mediaDevicesShim;
      for (const name of legacyMediaAliases) shim[name] = legacyGetUserMediaShim;
      return shim;
    }
    return new Proxy(target, {
      get(obj, prop) {
        if (prop === 'sendBeacon') return sendBeaconShim;
        if (prop === 'mediaDevices') return mediaDevicesShim;
        if (legacyMediaAliases.indexOf(String(prop)) !== -1) return legacyGetUserMediaShim;
        try {
          const value = obj ? obj[prop] : undefined;
          return typeof value === 'function' ? value.bind(obj) : value;
        } catch (_) {
          return undefined;
        }
      },
      has(obj, prop) {
        return prop === 'sendBeacon' || prop === 'mediaDevices' || legacyMediaAliases.indexOf(String(prop)) !== -1 || !!(obj && prop in obj);
      },
      set(obj, prop, value) {
        try { obj[prop] = value; return true; } catch (_) { return false; }
      },
    });
  }

  function createGlobalShim(nativeGlobal, navigatorShim, windowShim, missingGlobalShims) {
    const target = nativeGlobal || globalThis;
    if (typeof Proxy === 'undefined') return target;
    let proxy = null;
    proxy = new Proxy(target, {
      get(obj, prop) {
        if (prop === 'navigator') return navigatorShim;
        if (prop === 'window' || prop === 'self') return windowShim;
        if (prop === 'globalThis') return proxy;
        const missingShim = getGuardedMissingGlobalShim(missingGlobalShims, prop);
        if (missingShim) return missingShim;
        try {
          const value = obj ? obj[prop] : undefined;
          return typeof value === 'function' ? value.bind(obj) : value;
        } catch (_) {
          return undefined;
        }
      },
      has(obj, prop) {
        return prop === 'navigator' || prop === 'window' || prop === 'self' || prop === 'globalThis' || !!getGuardedMissingGlobalShim(missingGlobalShims, prop) || !!(obj && prop in obj);
      },
      set(obj, prop, value) {
        try { obj[prop] = value; return true; } catch (_) { return false; }
      },
    });
    return proxy;
  }

  function createWindowShim(nativeWindow, navigatorShim, missingGlobalShims) {
    const target = nativeWindow || globalThis;
    if (typeof Proxy === 'undefined') return target;
    let proxy = null;
    proxy = new Proxy(target, {
      get(obj, prop) {
        if (prop === 'navigator') return navigatorShim;
        if (prop === 'window' || prop === 'self' || prop === 'globalThis') return proxy;
        const missingShim = getGuardedMissingGlobalShim(missingGlobalShims, prop);
        if (missingShim) return missingShim;
        try {
          const value = obj ? obj[prop] : undefined;
          return typeof value === 'function' ? value.bind(obj) : value;
        } catch (_) {
          return undefined;
        }
      },
      has(obj, prop) {
        return prop === 'navigator' || prop === 'window' || prop === 'self' || prop === 'globalThis' || !!getGuardedMissingGlobalShim(missingGlobalShims, prop) || !!(obj && prop in obj);
      },
      set(obj, prop, value) {
        try { obj[prop] = value; return true; } catch (_) { return false; }
      },
    });
    return proxy;
  }


  function sourceDeclaresInjectedBinding(source, name) {
    const id = String(name || '');
    if (!/^[A-Za-z_$][\w$]*$/.test(id)) return false;
    const pattern = new RegExp('(^|[^A-Za-z0-9_$])(?:const|let|class|function)\\s+' + id + '\\b');
    return pattern.test(String(source || ''));
  }

  function capabilitySetForChunk(chunk) {
    if (!hostCapabilitiesPlan || !hostCapabilitiesPlan.enabled) return null;
    const records = Array.isArray(hostCapabilitiesPlan.chunkCapabilities) ? hostCapabilitiesPlan.chunkCapabilities : [];
    const order = chunk && chunk.order !== undefined ? chunk.order >>> 0 : 0;
    const route = chunk && chunk.route ? String(chunk.route) : '';
    const source = chunk && chunk.source ? String(chunk.source) : '';
    let record = records.find((item) => (item.order >>> 0) === order && (!item.route || item.route === route) && (!item.source || item.source === source));
    if (!record) record = records.find((item) => (item.order >>> 0) === order);
    const names = record && Array.isArray(record.capabilities) ? record.capabilities : hostCapabilitiesPlan.defaultCapabilities;
    return new Set(Array.isArray(names) ? names : []);
  }

  function capabilityDenied(name, chunk) {
    const source = chunk && chunk.source ? String(chunk.source) : '<unknown>';
    const error = new Error('VNM-CAP-1001: undeclared host capability "' + String(name) + '" for ' + source);
    error.name = 'VenomCapabilityError';
    error.code = 'VNM-CAP-1001';
    error.capability = String(name);
    return error;
  }

  function makeScopedRuntimeBridge(bridge, allowed, chunk) {
    if (!bridge) return Object.freeze({});
    const facade = Object.create(null);
    for (const name of ['version', 'packageVersion', 'runtimeHardened', 'failClosed', 'currentRouteGeneration', 'isRouteGenerationActive']) {
      const value = bridge[name];
      if (typeof value === 'function') facade[name] = value.bind(bridge);
      else if (value !== undefined) facade[name] = value;
    }
    const expose = (capability, names) => {
      if (!allowed || allowed.has(capability)) {
        for (const name of names) if (typeof bridge[name] === 'function') facade[name] = bridge[name].bind(bridge);
      }
    };
    expose('fetch', ['fetch', 'requestFetch']);
    expose('timers', ['scheduleTimer', 'cancelTimer']);
    expose('events', ['eventQueueSnapshot', 'dispatchEventBinding']);
    expose('document', ['domRootHandle', 'domHandleSnapshot', 'registerDomHandle', 'resolveDomHandle', 'revokeDomHandle', 'domSetAttribute', 'domAppendChild']);
    Object.defineProperty(facade, 'requireCapability', {
      value(name) {
        if (allowed && !allowed.has(String(name))) throw capabilityDenied(name, chunk);
        return true;
      },
      enumerable: false,
    });
    return Object.freeze(facade);
  }

  function makeBindings(bridge, chunk) {
    const bindings = Object.create(null);
    const allowed = capabilitySetForChunk(chunk);
    const permits = (name) => !allowed || allowed.has(name);
    const moduleEnabled = (name) => typeof __venomRuntimeModuleEnabled === 'function' && __venomRuntimeModuleEnabled(name);
    const exposeGlobal = (name, moduleName) => {
      if (!moduleEnabled(moduleName)) return;
      const value = globalThis[name];
      if (value !== undefined) bindings[name] = value;
    };
    const navigatorShim = createNavigatorShim(globalThis.navigator, bridge, chunk);
    const missingGlobalShims = createGuardedMissingGlobalShims(globalThis, bridge, chunk);
    const windowShim = createWindowShim(globalThis.window || globalThis, navigatorShim, missingGlobalShims);
    const globalShim = createGlobalShim(globalThis, navigatorShim, windowShim, missingGlobalShims);
    if (permits('window')) {
      bindings.globalThis = globalShim;
      bindings.window = windowShim;
      bindings.self = windowShim;
      for (const name of guardedMissingGlobalNames) bindings[name] = missingGlobalShims[name];
    }
    if (permits('navigator')) bindings.navigator = navigatorShim;
    if (permits('document')) bindings.document = globalThis.document;
    if (permits('__venomRuntime')) bindings.__venomRuntime = makeScopedRuntimeBridge(bridge, allowed, chunk);
    if (permits('console')) bindings.console = turboJsConsoleBridgePlan && turboJsConsoleBridgePlan.enabled ? makeConsoleBridgeConsole(globalThis.console, chunk) : globalThis.console;
    if (permits('fetch')) bindings.fetch = bridge && typeof bridge.fetch === 'function' ? bridge.fetch.bind(bridge) : globalThis.fetch;
    if (permits('timers')) {
      bindings.setTimeout = (callback, delay) => bridge && typeof bridge.scheduleTimer === 'function' ? bridge.scheduleTimer(callback, delay, false).id : globalThis.setTimeout(callback, delay);
      bindings.setInterval = (callback, delay) => bridge && typeof bridge.scheduleTimer === 'function' ? bridge.scheduleTimer(callback, delay, true).id : globalThis.setInterval(callback, delay);
      bindings.clearTimeout = (id) => bridge && typeof bridge.cancelTimer === 'function' ? bridge.cancelTimer(id) : globalThis.clearTimeout(id);
      bindings.clearInterval = (id) => bridge && typeof bridge.cancelTimer === 'function' ? bridge.cancelTimer(id) : globalThis.clearInterval(id);
    }
    if (permits('timers') && moduleEnabled('animation')) {
      bindings.requestAnimationFrame = typeof globalThis.requestAnimationFrame === 'function' ? globalThis.requestAnimationFrame.bind(globalThis) : (callback) => bindings.setTimeout(() => callback(Date.now()), 16);
      bindings.cancelAnimationFrame = typeof globalThis.cancelAnimationFrame === 'function' ? globalThis.cancelAnimationFrame.bind(globalThis) : bindings.clearTimeout;
    }
    if (permits('events')) for (const name of ['Event', 'CustomEvent']) exposeGlobal(name, 'events');
    for (const name of ['FormData', 'SubmitEvent']) exposeGlobal(name, 'forms');
    for (const name of ['MutationObserver', 'ResizeObserver', 'IntersectionObserver']) exposeGlobal(name, 'observers');
    if (permits('fetch')) for (const name of ['Headers', 'Request', 'Response', 'Blob', 'File', 'FileReader', 'URL', 'URLSearchParams', 'AbortController', 'XMLHttpRequest', 'WebSocket', 'EventSource']) exposeGlobal(name, 'network');
    bindings.__venomChunk = Object.freeze({ route: chunk.route, source: chunk.source, order: chunk.order, flags: chunk.flags });
    if (hostCapabilitiesPlan && hostCapabilitiesPlan.enabled) {
      const capabilityNames = allowed ? Array.from(allowed) : (hostCapabilitiesPlan.capabilities && hostCapabilitiesPlan.capabilities.length ? hostCapabilitiesPlan.capabilities.map((item) => item.name) : Object.keys(bindings));
      if (bridge && typeof bridge.enqueueHostCall === 'function') {
        const record = bridge.enqueueHostCall(hostCapabilitiesPlan.injectHostCall || 'turbojs.hostCapabilityInject', { route: chunk.route, source: chunk.source, order: chunk.order, capabilities: capabilityNames });
        if (bridge && typeof bridge.settleHostCall === 'function') bridge.settleHostCall(record.id, 'fulfilled', { injected: capabilityNames.length });
      }
      bindings.__venomCapabilities = Object.freeze(capabilityNames.slice());
    }
    return Object.freeze(bindings);
  }

  async function loadEngineModule() {
    if (!turboJsEngineModulePlan || !turboJsEngineModulePlan.enabled || !turboJsEngineModuleUrl) return null;
    if (engineModule) return engineModule;
    if (engineModuleError) return null;
    if (!engineModulePromise) {
      engineModulePromise = import(turboJsEngineModuleUrl).then((mod) => {
        const factoryName = turboJsEngineModulePlan.exportName || 'createVenomTurboJsEngineModule';
        const factory = mod && mod[factoryName];
        if (typeof factory !== 'function') throw new Error('TurboJS engine module missing export: ' + factoryName);
        engineModule = factory({
          version: turboJsEngineModulePlan.version,
          mode: turboJsEngineModulePlan.executionMode,
          fallback: turboJsEngineModulePlan.fallback,
          policy: scriptEnginePolicyPlan,
          bridge: turboJsBridgePlan,
          contextLifecycle: turboJsContextLifecyclePlan,
          hostCapabilities: hostCapabilitiesPlan,
          diagnostics: turboJsAdapterDiagnosticsPlan,
          wasmRuntime: turboJsWasmRuntimePlan,
          wasmUrl: turboJsWasmUrl,
          diversification: activeReleaseDiversification,
          abiFingerprint: activeTurboJsAbiFingerprint,
          sourceTransfer: turboJsSourceTransferPlan,
          consoleBridge: turboJsConsoleBridgePlan,
          executionRecords: turboJsExecutionRecordsPlan,
          resultBridge: turboJsResultBridgePlan,
          fallbackPolicy: turboJsFallbackPolicyPlan,
          runtimeAbi: turboJsRuntimeAbiPlan,
          hostImports: turboJsHostImportsPlan,
          heapLimits: turboJsHeapLimitsPlan,
          scriptBuffer: turboJsScriptBufferPlan,
          consoleAbi: turboJsConsoleAbiPlan,
          parityProbe: turboJsParityProbePlan,
          releaseFallback: turboJsReleaseFallbackPlan,
          bytecodeManifest: turboJsBytecodeManifestPlan,
          moduleResolver: turboJsModuleResolverPlan,
          exceptionAbi: turboJsExceptionAbiPlan,
          hostTrapPolicy: turboJsHostTrapPolicyPlan,
          executionLifecycle: turboJsExecutionLifecyclePlan,
          scriptBufferPolicy: turboJsScriptBufferPolicyPlan,
          contextLimitPolicy: turboJsContextLimitPolicyPlan,
          hostCallDispatch: turboJsHostCallDispatchPlan,
          parityContract: turboJsParityContractPlan,
          releaseFailClosed: turboJsReleaseFailClosedPlan,
          moduleGraph: turboJsModuleGraphPlan,
          moduleExecution: turboJsModuleExecutionPlan,
          moduleCache: turboJsModuleCachePlan,
          resolverAudit: turboJsResolverAuditPlan,
          interopFallback: turboJsInteropFallbackPlan,
          executionJournal: turboJsExecutionJournalPlan,
          checkpointPolicy: turboJsCheckpointPolicyPlan,
          replayCursor: turboJsReplayCursorPlan,
          resumeState: turboJsResumeStatePlan,
          determinismAudit: turboJsDeterminismAuditPlan,
          snapshotPolicy: turboJsSnapshotPolicyPlan,
          snapshotRecords: turboJsSnapshotRecordsPlan,
          replayValidation: turboJsReplayValidationPlan,
          determinismLedger: turboJsDeterminismLedgerPlan,
          auditSeal: turboJsAuditSealPlan,
          executionCommit: turboJsExecutionCommitPlan,
          rollbackPolicy: turboJsRollbackPolicyPlan,
          hostCallReceipts: turboJsHostCallReceiptsPlan,
          releaseAcceptance: turboJsReleaseAcceptancePlan,
          commitAudit: turboJsCommitAuditPlan,
        });
        if (!engineModule || typeof engineModule.executeChunk !== 'function') throw new Error('TurboJS engine module did not return executeChunk');
        return engineModule;
      }).catch((error) => {
        engineModuleError = error;
        return null;
      });
    }
    return engineModulePromise;
  }

  async function executeViaEngineModule(chunk, route, bridge, context, bindings) {
    const module = await loadEngineModule();
    if (!module) return null;
    if (module && typeof module.createContext === 'function') {
      const ctxRecord = asyncQueue.enqueue(turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.moduleCreateHostCall ? turboJsContextLifecyclePlan.moduleCreateHostCall : 'turbojs.moduleContextCreate', { context: context.key, route: chunk.route, source: chunk.source, order: chunk.order });
      const created = module.createContext(context);
      asyncQueue.settle(ctxRecord.id, 'fulfilled', { context: context.key, moduleContext: !!created });
    }
    const moduleRequest = asyncQueue.enqueue('turbojs.moduleExecute', {
      route: chunk.route,
      source: chunk.source,
      order: chunk.order,
      bytes: chunk.bytecodeSize || (chunk.code ? chunk.code.length : 0),
      moduleUrl: String(turboJsEngineModuleUrl || ''),
    });
    try {
      const result = await module.executeChunk({ chunk, route, context, bindings, wasmUrl: turboJsWasmUrl, wasmRuntime: turboJsWasmRuntimePlan, sourceTransfer: turboJsSourceTransferPlan, consoleBridge: turboJsConsoleBridgePlan, executionRecords: turboJsExecutionRecordsPlan, resultBridge: turboJsResultBridgePlan, fallbackPolicy: turboJsFallbackPolicyPlan });
      recordTurboJsExecution('turbojs.executionRecord', { route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, engine: result && result.turboJsWasm ? 'turbojs-wasm-backend-candidate' : 'host-js-fallback', fallbackUsed: !!(result && result.fallback), wasmAccepted: !!(result && result.turboJsWasm), hostBridgeTelemetry: result && result.hostBridgeTelemetry ? result.hostBridgeTelemetry : null, hostBridgeParity: result && result.hostBridgeParity ? result.hostBridgeParity : null });
      recordResultBridge({ route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, ok: !!(result && result.executed), wasmReport: result && result.wasmReport ? result.wasmReport : null, hostBridgeTelemetry: result && result.hostBridgeTelemetry ? result.hostBridgeTelemetry : null });
      asyncQueue.settle(moduleRequest.id, 'fulfilled', { executed: !!(result && result.executed), engineModule: true });
      return Object.freeze({
        ...chunk,
        ...(result || {}),
        executed: !!(result && result.executed),
        engineMode: turboJsEngineModulePlan.executionMode || 'route-scoped-module',
        engineModule: true,
        context: context.key,
      });
    } catch (error) {
      asyncQueue.settle(moduleRequest.id, 'rejected', { message: error && error.message ? error.message : String(error), engineModule: true });
      throw error;
    }
  }

  async function executeChunk(chunk, route, bridge) {
    const context = createContext(route && route.route ? route.route : chunk.route, chunk.source, chunk.order);
    const executionGeneration = context.generation >>> 0;
    const request = asyncQueue.enqueue(turboJsEnginePlan && turboJsEnginePlan.executeHostCall ? turboJsEnginePlan.executeHostCall : 'turbojs.executeChunk', { route: chunk.route, source: chunk.source, order: chunk.order, bytes: chunk.bytecodeSize || (chunk.code ? chunk.code.length : 0), bytecode: !!chunk.bytecode, mode, engineModule: !!turboJsEngineModuleUrl });
    try {
      if (!enabled) throw new Error('TurboJS engine bootstrap is disabled');
      if ((!chunk.code || !String(chunk.code).trim()) && !(chunk.bytecodeBytes && chunk.bytecodeBytes.length)) {
        asyncQueue.settle(request.id, 'fulfilled', { executed: false, empty: true, context: context.key });
        return Object.freeze({ ...chunk, executed: false, engineMode: mode, context: context.key });
      }
      if ((chunk.flags & SCRIPT_FLAG.REMOTE_URL) !== 0) {
        const executed = await injectRemoteScript(chunk);
        asyncQueue.settle(request.id, 'fulfilled', { executed: true, remote: true, context: context.key });
        return Object.freeze({ ...executed, executed: true, engineMode: mode, context: context.key, remote: true });
      }
      const bindings = makeBindings(bridge, chunk);
      const moduleExecuted = await executeViaEngineModule(chunk, route, bridge, context, bindings);
      if (executionGeneration !== (asyncQueue.snapshot().generation >>> 0)) throw new Error('stale route generation rejected for TurboJS execution');
      if (moduleExecuted) {
        asyncQueue.settle(request.id, 'fulfilled', { executed: true, context: context.key, engineModule: true });
        return moduleExecuted;
      }
      if ((chunk.flags & SCRIPT_FLAG.MODULE) !== 0 && typeof URL !== 'undefined' && typeof Blob !== 'undefined' && document.createElement) {
        const blob = new Blob([chunk.code + '\n//# sourceURL=venom://' + safeSourceUrl(chunk.source)], { type: 'text/javascript' });
        const url = URL.createObjectURL(blob);
        try {
          const executed = await injectRemoteScript({ ...chunk, source: url, flags: (chunk.flags | SCRIPT_FLAG.REMOTE_URL) });
          asyncQueue.settle(request.id, 'fulfilled', { executed: true, module: true, context: context.key });
          return Object.freeze({ ...executed, executed: true, engineMode: mode, context: context.key, module: true });
        } finally {
          URL.revokeObjectURL(url);
        }
      }
      if (!fallbackAllowed('engine-module-unavailable-or-compatible-fallback')) throw new Error('TurboJS fallback policy denied host execution');
      if (asyncQueue && turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled) { const decision = asyncQueue.enqueue(turboJsFallbackPolicyPlan.decisionHostCall || 'script.fallbackDecision', { route: chunk.route, source: chunk.source, order: chunk.order, allowed: true }); asyncQueue.settle(decision.id, 'fulfilled', { allowed: true }); }
__VENOM_RUNTIME_ENGINE_FALLBACK_BLOCK__
    } catch (error) {
      if (request && request.id && executionGeneration === (asyncQueue.snapshot().generation >>> 0)) asyncQueue.settle(request.id, 'rejected', { message: error && error.message ? error.message : String(error), context: context.key });
      throw error;
    }
  }

  function abiTable() {
    if (engineModule && typeof engineModule.abiTable === 'function') return engineModule.abiTable();
    return Object.freeze({ abi: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.abi : 0, packageVersion: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.packageVersion : 0, entryCount: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.entryCount : 0, tableHash: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.tableHash : '', table: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.table : '' });
  }

  function hostImportTable() {
    if (engineModule && typeof engineModule.hostImportTable === 'function') return engineModule.hostImportTable();
    return Object.freeze({ importCount: turboJsHostImportsPlan ? turboJsHostImportsPlan.importCount : 0, tableHash: turboJsHostImportsPlan ? turboJsHostImportsPlan.tableHash : '', table: turboJsHostImportsPlan ? turboJsHostImportsPlan.table : '', design: turboJsHostImportsPlan ? turboJsHostImportsPlan.design : '' });
  }

  function parityProbe() {
    if (engineModule && typeof engineModule.parityProbe === 'function') return engineModule.parityProbe();
    return Object.freeze({ expected: turboJsParityProbePlan ? turboJsParityProbePlan.expected : 'turbojs:5', native: turboJsParityProbePlan ? turboJsParityProbePlan.native : '', wasm: turboJsParityProbePlan ? turboJsParityProbePlan.wasm : '', matched: false, loaded: false });
  }

  function bytecodeManifest() {
    if (engineModule && typeof engineModule.bytecodeManifest === 'function') return engineModule.bytecodeManifest();
    return Object.freeze({ version: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.version : 0, format: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.format : '', chunkCount: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.chunkCount : 0, records: turboJsBytecodeManifestPlan && turboJsBytecodeManifestPlan.records ? turboJsBytecodeManifestPlan.records : Object.freeze([]), loaded: false });
  }

  function moduleResolver() {
    if (engineModule && typeof engineModule.moduleResolver === 'function') return engineModule.moduleResolver();
    return Object.freeze({ version: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.version : 0, abi: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.abi : 0, mode: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.mode : '', entries: turboJsModuleResolverPlan && turboJsModuleResolverPlan.entries ? turboJsModuleResolverPlan.entries : Object.freeze([]), loaded: false });
  }

  function exceptionAbi() {
    if (engineModule && typeof engineModule.exceptionAbi === 'function') return engineModule.exceptionAbi();
    return Object.freeze({ version: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.version : 0, abi: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.abi : 0, schema: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.schema : '', loaded: false });
  }

  function hostTrapPolicy() {
    if (engineModule && typeof engineModule.hostTrapPolicy === 'function') return engineModule.hostTrapPolicy();
    return Object.freeze({ version: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.version : 0, policy: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.policy : '', unknownImport: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.unknownImport : '', loaded: false });
  }

  function executionLifecycle() {
    if (engineModule && typeof engineModule.executionLifecycle === 'function') return engineModule.executionLifecycle();
    return Object.freeze({ version: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.version : 0, states: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.states : '', strictRelease: !!(turboJsExecutionLifecyclePlan && turboJsExecutionLifecyclePlan.strictRelease), loaded: false });
  }

  function scriptBufferPolicy() {
    if (engineModule && typeof engineModule.scriptBufferPolicy === 'function') return engineModule.scriptBufferPolicy();
    return Object.freeze({ version: turboJsScriptBufferPolicyPlan ? turboJsScriptBufferPolicyPlan.version : 0, maxScriptBytes: turboJsScriptBufferPolicyPlan ? turboJsScriptBufferPolicyPlan.maxScriptBytes : 786432, validateHashBeforeExecute: !(turboJsScriptBufferPolicyPlan && turboJsScriptBufferPolicyPlan.validateHashBeforeExecute === false), loaded: false });
  }

  function contextLimitPolicy() {
    if (engineModule && typeof engineModule.contextLimitPolicy === 'function') return engineModule.contextLimitPolicy();
    return Object.freeze({ version: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.version : 0, maxHeapBytes: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxHeapBytes : 0, maxStackBytes: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxStackBytes : 0, maxScriptBytes: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxScriptBytes : 0, maxHostCalls: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxHostCalls : 0, maxConsoleEvents: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxConsoleEvents : 0, maxModuleRecords: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxModuleRecords : 0, loaded: false });
  }

  function hostCallDispatch() {
    if (engineModule && typeof engineModule.hostCallDispatch === 'function') return engineModule.hostCallDispatch();
    return Object.freeze({ version: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.version : 0, entryCount: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.entryCount : 0, unknownHostCall: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.unknownHostCall : '', calls: turboJsHostCallDispatchPlan && turboJsHostCallDispatchPlan.calls ? turboJsHostCallDispatchPlan.calls : Object.freeze([]), loaded: false });
  }

  function parityContract() {
    if (engineModule && typeof engineModule.parityContract === 'function') return engineModule.parityContract();
    return Object.freeze({ version: turboJsParityContractPlan ? turboJsParityContractPlan.version : 0, compare: turboJsParityContractPlan ? turboJsParityContractPlan.compare : '', releaseOnMismatch: turboJsParityContractPlan ? turboJsParityContractPlan.releaseOnMismatch : '', loaded: false });
  }

  function releaseFailClosed() {
    if (engineModule && typeof engineModule.releaseFailClosed === 'function') return engineModule.releaseFailClosed();
    return Object.freeze({ version: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.version : 0, releasePolicy: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.releasePolicy : '', failOn: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.failOn : '', loaded: false });
  }

  function moduleGraph() {
    if (engineModule && typeof engineModule.moduleGraph === 'function') return engineModule.moduleGraph();
    return Object.freeze({ version: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.version : 0, moduleCount: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.moduleCount : 0, modules: turboJsModuleGraphPlan && turboJsModuleGraphPlan.modules ? turboJsModuleGraphPlan.modules : Object.freeze([]), cacheSize: 0, executions: 0, resolverAudits: 0, loaded: false });
  }

  function moduleCacheSnapshot() {
    if (engineModule && typeof engineModule.moduleCacheSnapshot === 'function') return engineModule.moduleCacheSnapshot();
    return Object.freeze({ version: turboJsModuleCachePlan ? turboJsModuleCachePlan.version : 0, mode: turboJsModuleCachePlan ? turboJsModuleCachePlan.mode : '', size: 0, entries: Object.freeze([]), loaded: false });
  }

  function resolverAudit() {
    if (engineModule && typeof engineModule.resolverAudit === 'function') return engineModule.resolverAudit();
    return Object.freeze({ version: turboJsResolverAuditPlan ? turboJsResolverAuditPlan.version : 0, mode: turboJsResolverAuditPlan ? turboJsResolverAuditPlan.mode : '', count: 0, records: Object.freeze([]), loaded: false });
  }

  function interopFallback() {
    if (engineModule && typeof engineModule.interopFallback === 'function') return engineModule.interopFallback();
    return Object.freeze({ version: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.version : 0, kind: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.fallbackKind : 'host-esm-transform-prototype', releaseBehavior: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.releaseBehavior : 'fail-closed-if-required-contract-missing', loaded: false });
  }

  function executionJournal() {
    if (engineModule && typeof engineModule.executionJournal === 'function') return engineModule.executionJournal();
    return Object.freeze({ version: turboJsExecutionJournalPlan ? turboJsExecutionJournalPlan.version : 0, maxRecords: turboJsExecutionJournalPlan ? turboJsExecutionJournalPlan.maxRecords : 0, records: Object.freeze(executionRecords.slice()), loaded: false });
  }

  function checkpointPolicy() {
    if (engineModule && typeof engineModule.checkpointPolicy === 'function') return engineModule.checkpointPolicy();
    return Object.freeze({ version: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.version : 0, maxCheckpoints: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.maxCheckpoints : 0, capture: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.capture : '', restorePolicy: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.restorePolicy : '', loaded: false });
  }

  function replayCursor() {
    if (engineModule && typeof engineModule.replayCursor === 'function') return engineModule.replayCursor();
    return Object.freeze({ version: turboJsReplayCursorPlan ? turboJsReplayCursorPlan.version : 0, sequence: executionRecords.length, cursorFields: turboJsReplayCursorPlan ? turboJsReplayCursorPlan.cursorFields : '', loaded: false });
  }

  function resumeState() {
    if (engineModule && typeof engineModule.resumeState === 'function') return engineModule.resumeState();
    return Object.freeze({ version: turboJsResumeStatePlan ? turboJsResumeStatePlan.version : 0, contextCount: contexts.size, executionCount: executionRecords.length, consoleEvents: consoleEvents.length, loaded: false });
  }

  function determinismAudit() {
    if (engineModule && typeof engineModule.determinismAudit === 'function') return engineModule.determinismAudit();
    return Object.freeze({ version: turboJsDeterminismAuditPlan ? turboJsDeterminismAuditPlan.version : 0, executionCount: executionRecords.length, hostImportCount: turboJsHostImportsPlan ? turboJsHostImportsPlan.importCount : 0, releaseRequires: turboJsDeterminismAuditPlan ? turboJsDeterminismAuditPlan.releaseRequires : '', loaded: false });
  }

  function snapshotPolicy() {
    if (engineModule && typeof engineModule.snapshotPolicy === 'function') return engineModule.snapshotPolicy();
    return Object.freeze({ version: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.version : 0, capture: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.capture : '', validate: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.validate : '', loaded: false });
  }

  function snapshotRecord() {
    if (engineModule && typeof engineModule.snapshotRecord === 'function') return engineModule.snapshotRecord();
    return Object.freeze({ version: turboJsSnapshotRecordsPlan ? turboJsSnapshotRecordsPlan.version : 0, count: executionRecords.length, loaded: false });
  }

  function replayValidation() {
    if (engineModule && typeof engineModule.replayValidation === 'function') return engineModule.replayValidation();
    return Object.freeze({ version: turboJsReplayValidationPlan ? turboJsReplayValidationPlan.version : 0, checks: turboJsReplayValidationPlan ? turboJsReplayValidationPlan.checks : '', loaded: false });
  }

  function determinismLedger() {
    if (engineModule && typeof engineModule.determinismLedger === 'function') return engineModule.determinismLedger();
    return Object.freeze({ version: turboJsDeterminismLedgerPlan ? turboJsDeterminismLedgerPlan.version : 0, chainHash: turboJsDeterminismLedgerPlan ? turboJsDeterminismLedgerPlan.chainHash : '', entries: executionRecords.length, loaded: false });
  }

  function auditSeal() {
    if (engineModule && typeof engineModule.auditSeal === 'function') return engineModule.auditSeal();
    return Object.freeze({ version: turboJsAuditSealPlan ? turboJsAuditSealPlan.version : 0, releaseRequires: turboJsAuditSealPlan ? turboJsAuditSealPlan.releaseRequires : '', loaded: false });
  }

  function executionCommit() {
    if (engineModule && typeof engineModule.executionCommit === 'function') return engineModule.executionCommit();
    return Object.freeze({ version: turboJsExecutionCommitPlan ? turboJsExecutionCommitPlan.version : 0, commitHostCall: turboJsExecutionCommitPlan ? turboJsExecutionCommitPlan.commitHostCall : 'execution.commit', committed: executionRecords.length, loaded: false });
  }

  function rollbackPolicy() {
    if (engineModule && typeof engineModule.rollbackPolicy === 'function') return engineModule.rollbackPolicy();
    return Object.freeze({ version: turboJsRollbackPolicyPlan ? turboJsRollbackPolicyPlan.version : 0, rollbackHostCall: turboJsRollbackPolicyPlan ? turboJsRollbackPolicyPlan.rollbackHostCall : 'execution.rollback', loaded: false });
  }

  function hostCallReceipts() {
    if (engineModule && typeof engineModule.hostCallReceipts === 'function') return engineModule.hostCallReceipts();
    return Object.freeze({ version: turboJsHostCallReceiptsPlan ? turboJsHostCallReceiptsPlan.version : 0, receiptHostCall: turboJsHostCallReceiptsPlan ? turboJsHostCallReceiptsPlan.receiptHostCall : 'host.receipt', count: executionRecords.length, loaded: false });
  }

  function releaseAcceptance() {
    if (engineModule && typeof engineModule.releaseAcceptance === 'function') return engineModule.releaseAcceptance();
    return Object.freeze({ version: turboJsReleaseAcceptancePlan ? turboJsReleaseAcceptancePlan.version : 0, checks: turboJsReleaseAcceptancePlan ? turboJsReleaseAcceptancePlan.checks : '', accepted: true, loaded: false });
  }

  function commitAudit() {
    if (engineModule && typeof engineModule.commitAudit === 'function') return engineModule.commitAudit();
    return Object.freeze({ version: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.version : 0, auditFields: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.auditFields : '', releaseRequires: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.releaseRequires : '', loaded: false });
  }

  function capabilityPolicy() {
    if (engineModule && typeof engineModule.capabilityPolicy === 'function') return engineModule.capabilityPolicy();
    return Object.freeze({ version: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.version : 0, mode: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.mode : '', capabilities: turboJsCapabilityPolicyPlan && turboJsCapabilityPolicyPlan.capabilities ? turboJsCapabilityPolicyPlan.capabilities : '', releaseRequires: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.releaseRequires : '', loaded: false });
  }

  function hostIoPolicy() {
    if (engineModule && typeof engineModule.hostIoPolicy === 'function') return engineModule.hostIoPolicy();
    return Object.freeze({ version: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.version : 0, network: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.network : '', filesystem: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.filesystem : '', releaseBehavior: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.releaseBehavior : '', loaded: false });
  }

  function permissionSeal() {
    if (engineModule && typeof engineModule.permissionSeal === 'function') return engineModule.permissionSeal();
    return Object.freeze({ version: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.version : 0, algorithm: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.algorithm : '', releaseRequires: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.releaseRequires : '', loaded: false });
  }

  function policyReceipts() {
    if (engineModule && typeof engineModule.policyReceipts === 'function') return engineModule.policyReceipts();
    return Object.freeze({ version: turboJsPolicyReceiptsPlan ? turboJsPolicyReceiptsPlan.version : 0, hostCall: turboJsPolicyReceiptsPlan ? turboJsPolicyReceiptsPlan.hostCall : 'host.receipt', count: executionRecords.length, loaded: false });
  }

  function releaseGate() {
    if (engineModule && typeof engineModule.releaseGate === 'function') return engineModule.releaseGate();
    return Object.freeze({ version: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.version : 0, gate: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.gate : '', failOn: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.failOn : '', loaded: false });
  }

  function hostIoDecision() {
    if (engineModule && typeof engineModule.hostIoDecision === 'function') return engineModule.hostIoDecision();
    return Object.freeze({ version: turboJsHostIoDecisionPlan ? turboJsHostIoDecisionPlan.version : 0, fields: turboJsHostIoDecisionPlan ? turboJsHostIoDecisionPlan.fields : '', count: executionRecords.length, loaded: false });
  }

  function hostIoDenyTrace() {
    if (engineModule && typeof engineModule.hostIoDenyTrace === 'function') return engineModule.hostIoDenyTrace();
    return Object.freeze({ version: turboJsHostIoDenyTracePlan ? turboJsHostIoDenyTracePlan.version : 0, fields: turboJsHostIoDenyTracePlan ? turboJsHostIoDenyTracePlan.fields : '', count: 0, loaded: false });
  }

  function capabilityLedger() {
    if (engineModule && typeof engineModule.capabilityLedger === 'function') return engineModule.capabilityLedger();
    return Object.freeze({ version: turboJsCapabilityLedgerPlan ? turboJsCapabilityLedgerPlan.version : 0, fields: turboJsCapabilityLedgerPlan ? turboJsCapabilityLedgerPlan.fields : '', entries: executionRecords.length, loaded: false });
  }

  function policySealAudit() {
    if (engineModule && typeof engineModule.policySealAudit === 'function') return engineModule.policySealAudit();
    return Object.freeze({ version: turboJsPolicySealAuditPlan ? turboJsPolicySealAuditPlan.version : 0, fields: turboJsPolicySealAuditPlan ? turboJsPolicySealAuditPlan.fields : '', loaded: false });
  }

  function runtimeDenylist() {
    if (engineModule && typeof engineModule.runtimeDenylist === 'function') return engineModule.runtimeDenylist();
    return Object.freeze({ version: turboJsRuntimeDenylistPlan ? turboJsRuntimeDenylistPlan.version : 0, denylist: turboJsRuntimeDenylistPlan ? turboJsRuntimeDenylistPlan.denylist : '', loaded: false });
  }

  return Object.freeze({
    enabled,
    mode,
    contexts,
    createContext,
    destroyContext,
    destroyRoute,
    contextSnapshot,
    executeChunk,
    bridgeMode: turboJsBridgePlan ? turboJsBridgePlan.mode : 'none',
    moduleEnabled: !!(turboJsEngineModulePlan && turboJsEngineModulePlan.enabled),
    moduleUrl: turboJsEngineModuleUrl || '',
    adapterDiagnostics: turboJsAdapterDiagnosticsPlan ? turboJsAdapterDiagnosticsPlan.records : '',
    executionSnapshot() { return Object.freeze({ count: executionRecords.length, records: Object.freeze(executionRecords.slice()) }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) { const request = asyncQueue.enqueue(turboJsResultBridgePlan.consoleFlushHostCall || 'turbojs.consoleFlush', { count }); asyncQueue.settle(request.id, 'fulfilled', { flushed: count }); } return count; },
    fallbackPolicy() { return Object.freeze({ enabled: !!(turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled), mode: turboJsFallbackPolicyPlan ? turboJsFallbackPolicyPlan.mode : 'legacy', allowWhen: turboJsFallbackPolicyPlan ? turboJsFallbackPolicyPlan.allowWhen : '', strictRelease: !!(turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.strictRelease) }); },
    abiTable,
    hostImportTable,
    parityProbe,
    bytecodeManifest,
    moduleResolver,
    exceptionAbi,
    hostTrapPolicy,
    executionLifecycle,
    scriptBufferPolicy,
    contextLimitPolicy,
    hostCallDispatch,
    parityContract,
    releaseFailClosed,
    moduleGraph,
    moduleCacheSnapshot,
    resolverAudit,
    interopFallback,
    executionJournal,
    checkpointPolicy,
    replayCursor,
    resumeState,
    determinismAudit,
    snapshotPolicy,
    snapshotRecord,
    replayValidation,
    determinismLedger,
    auditSeal,
    executionCommit,
    rollbackPolicy,
    hostCallReceipts,
    releaseAcceptance,
    commitAudit,
    capabilityPolicy,
    hostIoPolicy,
    permissionSeal,
    policyReceipts,
    releaseGate,
    hostIoDecision,
    hostIoDenyTrace,
    capabilityLedger,
    policySealAudit,
    runtimeDenylist,
    moduleStatus() { return Object.freeze({ enabled, mode, moduleEnabled: !!(turboJsEngineModulePlan && turboJsEngineModulePlan.enabled), moduleLoaded: !!engineModule, moduleUrl: turboJsEngineModuleUrl || '', wasmUrl: turboJsWasmUrl || '', wasmRuntime: !!turboJsWasmRuntimePlan, moduleError: engineModuleError ? String(engineModuleError.message || engineModuleError) : '', contextCount: contexts.size }); },
    moduleError() { return engineModuleError ? String(engineModuleError.message || engineModuleError) : ''; },
  });
}

function installVenomHostBridge(root, pkg, routes, assetManifest, runtimePolicy, hostBridgePlan, fetchBridgePlan, asyncQueuePlan, timerBridgePlan, eventQueuePlan, turboJsBridgePlan, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsEngineModulePlan = null, turboJsEngineModuleUrl = null, turboJsContextLifecyclePlan = null, hostCapabilitiesPlan = null, turboJsAdapterDiagnosticsPlan = null, turboJsWasmRuntimePlan = null, turboJsWasmUrl = null, turboJsSourceTransferPlan = null, turboJsConsoleBridgePlan = null, turboJsExecutionRecordsPlan = null, turboJsResultBridgePlan = null, turboJsFallbackPolicyPlan = null, turboJsRuntimeAbiPlan = null, turboJsHostImportsPlan = null, turboJsHeapLimitsPlan = null, turboJsScriptBufferPlan = null, turboJsConsoleAbiPlan = null, turboJsParityProbePlan = null, turboJsReleaseFallbackPlan = null, turboJsBytecodeManifestPlan = null, turboJsModuleResolverPlan = null, turboJsExceptionAbiPlan = null, turboJsHostTrapPolicyPlan = null, turboJsExecutionLifecyclePlan = null, turboJsScriptBufferPolicyPlan = null, turboJsContextLimitPolicyPlan = null, turboJsHostCallDispatchPlan = null, turboJsParityContractPlan = null, turboJsReleaseFailClosedPlan = null, turboJsModuleGraphPlan = null, turboJsModuleExecutionPlan = null, turboJsModuleCachePlan = null, turboJsResolverAuditPlan = null, turboJsInteropFallbackPlan = null, turboJsExecutionJournalPlan = null, turboJsCheckpointPolicyPlan = null, turboJsReplayCursorPlan = null, turboJsResumeStatePlan = null, turboJsDeterminismAuditPlan = null, turboJsSnapshotPolicyPlan = null, turboJsSnapshotRecordsPlan = null, turboJsReplayValidationPlan = null, turboJsDeterminismLedgerPlan = null, turboJsAuditSealPlan = null, turboJsExecutionCommitPlan = null, turboJsRollbackPolicyPlan = null, turboJsHostCallReceiptsPlan = null, turboJsReleaseAcceptancePlan = null, turboJsCommitAuditPlan = null, turboJsCapabilityPolicyPlan = null, turboJsHostIoPolicyPlan = null, turboJsPermissionSealPlan = null, turboJsPolicyReceiptsPlan = null, turboJsReleaseGatePlan = null, turboJsHostIoDecisionPlan = null, turboJsHostIoDenyTracePlan = null, turboJsCapabilityLedgerPlan = null, turboJsPolicySealAuditPlan = null, turboJsRuntimeDenylistPlan = null) {
  const asyncQueue = createAsyncHostQueue(fetchBridgePlan, asyncQueuePlan, timerBridgePlan, turboJsBridgePlan);
  const eventQueue = createEventQueue(eventQueuePlan);
  const domHandles = createDomHandleRegistry(root, asyncQueuePlan && asyncQueuePlan.maxDomHandles ? asyncQueuePlan.maxDomHandles : 4096);
  let bridgeRef = null;
  const turboJsEngine = createTurboJsEngine(asyncQueue, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsBridgePlan, turboJsEngineModulePlan, turboJsEngineModuleUrl, turboJsContextLifecyclePlan, hostCapabilitiesPlan, turboJsAdapterDiagnosticsPlan, turboJsWasmRuntimePlan, turboJsWasmUrl, turboJsSourceTransferPlan, turboJsConsoleBridgePlan, turboJsExecutionRecordsPlan, turboJsResultBridgePlan, turboJsFallbackPolicyPlan, turboJsRuntimeAbiPlan, turboJsHostImportsPlan, turboJsHeapLimitsPlan, turboJsScriptBufferPlan, turboJsConsoleAbiPlan, turboJsParityProbePlan, turboJsReleaseFallbackPlan, turboJsBytecodeManifestPlan, turboJsModuleResolverPlan, turboJsExceptionAbiPlan, turboJsHostTrapPolicyPlan, turboJsExecutionLifecyclePlan, turboJsScriptBufferPolicyPlan, turboJsContextLimitPolicyPlan, turboJsHostCallDispatchPlan, turboJsParityContractPlan, turboJsReleaseFailClosedPlan, turboJsModuleGraphPlan, turboJsModuleExecutionPlan, turboJsModuleCachePlan, turboJsResolverAuditPlan, turboJsInteropFallbackPlan, turboJsExecutionJournalPlan, turboJsCheckpointPolicyPlan, turboJsReplayCursorPlan, turboJsResumeStatePlan, turboJsDeterminismAuditPlan, turboJsSnapshotPolicyPlan, turboJsSnapshotRecordsPlan, turboJsReplayValidationPlan, turboJsDeterminismLedgerPlan, turboJsAuditSealPlan, turboJsExecutionCommitPlan, turboJsRollbackPolicyPlan, turboJsHostCallReceiptsPlan, turboJsReleaseAcceptancePlan, turboJsCommitAuditPlan, turboJsCapabilityPolicyPlan, turboJsHostIoPolicyPlan, turboJsPermissionSealPlan, turboJsPolicyReceiptsPlan, turboJsReleaseGatePlan, turboJsHostIoDecisionPlan, turboJsHostIoDenyTracePlan, turboJsCapabilityLedgerPlan, turboJsPolicySealAuditPlan, turboJsRuntimeDenylistPlan);
  const bridge = Object.freeze({
    version: 1,
    packageVersion: pkg.version,
    packageFeatureTableVersion: pkg.features ? pkg.features.version : 0,
    packageFeatureCount: pkg.features ? pkg.features.featureCount : 0,
    packageFeatureRequiredInRelease: pkg.features ? pkg.features.requiredInRelease : 0,
    routeCount: routes.length,
    assetCount: assetManifest.size,
    runtimeHardened: runtimePolicy.runtimeHardened,
    failClosed: runtimePolicy.failClosed,
    hostBridgeVersion: hostBridgePlan ? hostBridgePlan.version : 0,
    hostCallCount: hostBridgePlan && hostBridgePlan.hostCalls ? hostBridgePlan.hostCalls.size : 0,
    eventBindingMode: hostBridgePlan && hostBridgePlan.inlineEventBindings ? 'inline-attribute' : 'none',
    fetchBridgeVersion: fetchBridgePlan ? fetchBridgePlan.version : 0,
    fetchBridgeMode: fetchBridgePlan && fetchBridgePlan.enabled ? 'async-host-call' : 'none',
    asyncHostQueueVersion: asyncQueuePlan ? asyncQueuePlan.version : 0,
    asyncHostQueueMode: asyncQueuePlan && asyncQueuePlan.enabled ? 'enabled' : 'none',
    timerBridgeVersion: timerBridgePlan ? timerBridgePlan.version : 0,
    timerBridgeMode: timerBridgePlan && timerBridgePlan.enabled ? 'async-host-call' : 'none',
    eventQueueVersion: eventQueuePlan ? eventQueuePlan.version : 0,
    eventQueueMode: eventQueuePlan && eventQueuePlan.enabled ? 'enabled' : 'none',
    turboJsBridgeVersion: turboJsBridgePlan ? turboJsBridgePlan.version : 0,
    turboJsBridgeMode: turboJsBridgePlan ? turboJsBridgePlan.mode : 'none',
    scriptIsolationVersion: scriptIsolationPlan ? scriptIsolationPlan.version : 0,
    scriptIsolationMode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'none',
    scriptPolicyVersion: scriptPolicyPlan ? scriptPolicyPlan.version : 0,
    scriptPolicyMode: scriptPolicyPlan ? 'capability-metadata' : 'none',
    turboJsChunkVersion: turboJsChunkPlan ? turboJsChunkPlan.version : 0,
    turboJsChunkMode: turboJsChunkPlan ? turboJsChunkPlan.mode : 'none',
    turboJsChunkCount: turboJsChunkPlan ? turboJsChunkPlan.chunkCount : 0,
    turboJsEngineVersion: turboJsEnginePlan ? turboJsEnginePlan.version : 0,
    turboJsEngineMode: turboJsEnginePlan ? turboJsEnginePlan.mode : 'none',
    turboJsEngineEnabled: turboJsEnginePlan ? !!turboJsEnginePlan.enabled : false,
    scriptEnginePolicyVersion: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.version : 0,
    scriptEngineFallback: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.fallback : 'none',
    turboJsEngineModuleVersion: turboJsEngineModulePlan ? turboJsEngineModulePlan.version : 0,
    turboJsEngineModuleMode: turboJsEngineModulePlan && turboJsEngineModulePlan.enabled ? turboJsEngineModulePlan.executionMode : 'none',
    turboJsEngineModuleLoaded: !!turboJsEngineModuleUrl,
    turboJsEngineModuleUrl: turboJsEngineModuleUrl || '',
    turboJsContextLifecycleVersion: turboJsContextLifecyclePlan ? turboJsContextLifecyclePlan.version : 0,
    turboJsContextLifecycleMode: turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.enabled ? turboJsContextLifecyclePlan.model : 'none',
    hostCapabilitiesVersion: hostCapabilitiesPlan ? hostCapabilitiesPlan.version : 0,
    hostCapabilityCount: hostCapabilitiesPlan && hostCapabilitiesPlan.capabilities ? hostCapabilitiesPlan.capabilities.length : 0,
    turboJsAdapterDiagnosticsVersion: turboJsAdapterDiagnosticsPlan ? turboJsAdapterDiagnosticsPlan.version : 0,
    turboJsAdapterDiagnosticsMode: turboJsAdapterDiagnosticsPlan && turboJsAdapterDiagnosticsPlan.enabled ? 'enabled' : 'none',
    turboJsWasmRuntimeVersion: turboJsWasmRuntimePlan ? turboJsWasmRuntimePlan.version : 0,
    turboJsWasmRuntimeMode: turboJsWasmRuntimePlan && turboJsWasmRuntimePlan.enabled ? turboJsWasmRuntimePlan.executionMode : 'none',
    turboJsWasmUrl: turboJsWasmUrl || '',
    turboJsSourceTransferVersion: turboJsSourceTransferPlan ? turboJsSourceTransferPlan.version : 0,
    turboJsSourceTransferMode: turboJsSourceTransferPlan && turboJsSourceTransferPlan.enabled ? 'enabled' : 'none',
    turboJsConsoleBridgeVersion: turboJsConsoleBridgePlan ? turboJsConsoleBridgePlan.version : 0,
    turboJsConsoleBridgeMode: turboJsConsoleBridgePlan && turboJsConsoleBridgePlan.enabled ? turboJsConsoleBridgePlan.mode : 'none',
    turboJsExecutionRecordsVersion: turboJsExecutionRecordsPlan ? turboJsExecutionRecordsPlan.version : 0,
    turboJsExecutionRecordsMode: turboJsExecutionRecordsPlan && turboJsExecutionRecordsPlan.enabled ? turboJsExecutionRecordsPlan.retention : 'none',
    turboJsResultBridgeVersion: turboJsResultBridgePlan ? turboJsResultBridgePlan.version : 0,
    turboJsResultBridgeMode: turboJsResultBridgePlan && turboJsResultBridgePlan.enabled ? turboJsResultBridgePlan.format : 'none',
    turboJsFallbackPolicyVersion: turboJsFallbackPolicyPlan ? turboJsFallbackPolicyPlan.version : 0,
    turboJsFallbackPolicyMode: turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled ? turboJsFallbackPolicyPlan.mode : 'none',
    turboJsRuntimeAbi: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.abi : 0,
    turboJsRuntimeAbiHash: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.tableHash : '',
    turboJsHostImportCount: turboJsHostImportsPlan ? turboJsHostImportsPlan.importCount : 0,
    turboJsHeapLimit: turboJsHeapLimitsPlan ? turboJsHeapLimitsPlan.heapLimit : 0,
    turboJsScriptBufferCapacity: turboJsScriptBufferPlan ? turboJsScriptBufferPlan.capacity : 0,
    turboJsConsoleAbi: turboJsConsoleAbiPlan ? turboJsConsoleAbiPlan.abi : 0,
    turboJsParityProbeExpected: turboJsParityProbePlan ? turboJsParityProbePlan.expected : '',
    turboJsReleaseFallbackPolicy: turboJsReleaseFallbackPlan ? turboJsReleaseFallbackPlan.policy : '',
    turboJsBytecodeManifestVersion: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.version : 0,
    turboJsBytecodeManifestMode: turboJsBytecodeManifestPlan && turboJsBytecodeManifestPlan.enabled ? turboJsBytecodeManifestPlan.format : 'none',
    turboJsBytecodeChunkCount: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.chunkCount : 0,
    turboJsModuleResolverVersion: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.version : 0,
    turboJsModuleResolverMode: turboJsModuleResolverPlan && turboJsModuleResolverPlan.enabled ? turboJsModuleResolverPlan.mode : 'none',
    turboJsExceptionAbi: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.abi : 0,
    turboJsExceptionAbiVersion: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.version : 0,
    turboJsHostTrapPolicy: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.policy : '',
    turboJsHostTrapPolicyVersion: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.version : 0,
    turboJsExecutionLifecycleVersion: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.version : 0,
    turboJsExecutionLifecycleStates: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.states : '',
    turboJsScriptBufferPolicyVersion: turboJsScriptBufferPolicyPlan ? turboJsScriptBufferPolicyPlan.version : 0,
    turboJsContextLimitPolicyVersion: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.version : 0,
    turboJsHostCallDispatchVersion: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.version : 0,
    turboJsHostCallDispatchCount: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.entryCount : 0,
    turboJsParityContractVersion: turboJsParityContractPlan ? turboJsParityContractPlan.version : 0,
    turboJsReleaseFailClosedVersion: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.version : 0,
    turboJsModuleGraphVersion: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.version : 0,
    turboJsModuleGraphCount: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.moduleCount : 0,
    turboJsModuleExecutionVersion: turboJsModuleExecutionPlan ? turboJsModuleExecutionPlan.version : 0,
    turboJsModuleExecutionMode: turboJsModuleExecutionPlan && turboJsModuleExecutionPlan.enabled ? turboJsModuleExecutionPlan.mode : 'none',
    turboJsModuleCacheVersion: turboJsModuleCachePlan ? turboJsModuleCachePlan.version : 0,
    turboJsResolverAuditVersion: turboJsResolverAuditPlan ? turboJsResolverAuditPlan.version : 0,
    turboJsInteropFallbackVersion: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.version : 0,
    turboJsExecutionJournalVersion: turboJsExecutionJournalPlan ? turboJsExecutionJournalPlan.version : 0,
    turboJsCheckpointPolicyVersion: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.version : 0,
    turboJsReplayCursorVersion: turboJsReplayCursorPlan ? turboJsReplayCursorPlan.version : 0,
    turboJsResumeStateVersion: turboJsResumeStatePlan ? turboJsResumeStatePlan.version : 0,
    turboJsDeterminismAuditVersion: turboJsDeterminismAuditPlan ? turboJsDeterminismAuditPlan.version : 0,
    turboJsSnapshotPolicyVersion: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.version : 0,
    turboJsSnapshotRecordsVersion: turboJsSnapshotRecordsPlan ? turboJsSnapshotRecordsPlan.version : 0,
    turboJsReplayValidationVersion: turboJsReplayValidationPlan ? turboJsReplayValidationPlan.version : 0,
    turboJsDeterminismLedgerVersion: turboJsDeterminismLedgerPlan ? turboJsDeterminismLedgerPlan.version : 0,
    turboJsAuditSealVersion: turboJsAuditSealPlan ? turboJsAuditSealPlan.version : 0,
    turboJsExecutionCommitVersion: turboJsExecutionCommitPlan ? turboJsExecutionCommitPlan.version : 0,
    turboJsRollbackPolicyVersion: turboJsRollbackPolicyPlan ? turboJsRollbackPolicyPlan.version : 0,
    turboJsHostCallReceiptsVersion: turboJsHostCallReceiptsPlan ? turboJsHostCallReceiptsPlan.version : 0,
    turboJsReleaseAcceptanceVersion: turboJsReleaseAcceptancePlan ? turboJsReleaseAcceptancePlan.version : 0,
    turboJsCommitAuditVersion: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.version : 0,
    turboJsCapabilityPolicyVersion: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.version : 0,
    turboJsHostIoPolicyVersion: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.version : 0,
    turboJsPermissionSealVersion: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.version : 0,
    turboJsPolicyReceiptsVersion: turboJsPolicyReceiptsPlan ? turboJsPolicyReceiptsPlan.version : 0,
    turboJsReleaseGateVersion: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.version : 0,
    turboJsHostIoDecisionVersion: turboJsHostIoDecisionPlan ? turboJsHostIoDecisionPlan.version : 0,
    turboJsHostIoDenyTraceVersion: turboJsHostIoDenyTracePlan ? turboJsHostIoDenyTracePlan.version : 0,
    turboJsCapabilityLedgerVersion: turboJsCapabilityLedgerPlan ? turboJsCapabilityLedgerPlan.version : 0,
    turboJsPolicySealAuditVersion: turboJsPolicySealAuditPlan ? turboJsPolicySealAuditPlan.version : 0,
    turboJsRuntimeDenylistVersion: turboJsRuntimeDenylistPlan ? turboJsRuntimeDenylistPlan.version : 0,
    turboJsExecutionJournal() { return turboJsEngine.executionJournal ? turboJsEngine.executionJournal() : Object.freeze({ plan: turboJsExecutionJournalPlan || null, records: [] }); },
    turboJsCheckpointPolicy() { return turboJsEngine.checkpointPolicy ? turboJsEngine.checkpointPolicy() : Object.freeze({ plan: turboJsCheckpointPolicyPlan || null }); },
    turboJsReplayCursor() { return turboJsEngine.replayCursor ? turboJsEngine.replayCursor() : Object.freeze({ plan: turboJsReplayCursorPlan || null, sequence: 0 }); },
    turboJsResumeState() { return turboJsEngine.resumeState ? turboJsEngine.resumeState() : Object.freeze({ plan: turboJsResumeStatePlan || null }); },
    turboJsDeterminismAudit() { return turboJsEngine.determinismAudit ? turboJsEngine.determinismAudit() : Object.freeze({ plan: turboJsDeterminismAuditPlan || null }); },
    turboJsSnapshotPolicy() { return turboJsEngine.snapshotPolicy ? turboJsEngine.snapshotPolicy() : Object.freeze({ plan: turboJsSnapshotPolicyPlan || null }); },
    turboJsSnapshotRecord() { return turboJsEngine.snapshotRecord ? turboJsEngine.snapshotRecord() : Object.freeze({ plan: turboJsSnapshotRecordsPlan || null }); },
    turboJsReplayValidation() { return turboJsEngine.replayValidation ? turboJsEngine.replayValidation() : Object.freeze({ plan: turboJsReplayValidationPlan || null }); },
    turboJsDeterminismLedger() { return turboJsEngine.determinismLedger ? turboJsEngine.determinismLedger() : Object.freeze({ plan: turboJsDeterminismLedgerPlan || null }); },
    turboJsAuditSeal() { return turboJsEngine.auditSeal ? turboJsEngine.auditSeal() : Object.freeze({ plan: turboJsAuditSealPlan || null }); },
    turboJsExecutionCommit() { return turboJsEngine.executionCommit ? turboJsEngine.executionCommit() : Object.freeze({ plan: turboJsExecutionCommitPlan || null }); },
    turboJsRollbackPolicy() { return turboJsEngine.rollbackPolicy ? turboJsEngine.rollbackPolicy() : Object.freeze({ plan: turboJsRollbackPolicyPlan || null }); },
    turboJsHostCallReceipts() { return turboJsEngine.hostCallReceipts ? turboJsEngine.hostCallReceipts() : Object.freeze({ plan: turboJsHostCallReceiptsPlan || null }); },
    turboJsReleaseAcceptance() { return turboJsEngine.releaseAcceptance ? turboJsEngine.releaseAcceptance() : Object.freeze({ plan: turboJsReleaseAcceptancePlan || null }); },
    turboJsCommitAudit() { return turboJsEngine.commitAudit ? turboJsEngine.commitAudit() : Object.freeze({ plan: turboJsCommitAuditPlan || null }); },
    turboJsCapabilityPolicy() { return turboJsEngine.capabilityPolicy ? turboJsEngine.capabilityPolicy() : Object.freeze({ plan: turboJsCapabilityPolicyPlan || null }); },
    turboJsHostIoPolicy() { return turboJsEngine.hostIoPolicy ? turboJsEngine.hostIoPolicy() : Object.freeze({ plan: turboJsHostIoPolicyPlan || null }); },
    turboJsPermissionSeal() { return turboJsEngine.permissionSeal ? turboJsEngine.permissionSeal() : Object.freeze({ plan: turboJsPermissionSealPlan || null }); },
    turboJsPolicyReceipts() { return turboJsEngine.policyReceipts ? turboJsEngine.policyReceipts() : Object.freeze({ plan: turboJsPolicyReceiptsPlan || null }); },
    turboJsReleaseGate() { return turboJsEngine.releaseGate ? turboJsEngine.releaseGate() : Object.freeze({ plan: turboJsReleaseGatePlan || null }); },
    turboJsHostIoDecision() { return turboJsEngine.hostIoDecision ? turboJsEngine.hostIoDecision() : Object.freeze({ plan: turboJsHostIoDecisionPlan || null }); },
    turboJsHostIoDenyTrace() { return turboJsEngine.hostIoDenyTrace ? turboJsEngine.hostIoDenyTrace() : Object.freeze({ plan: turboJsHostIoDenyTracePlan || null }); },
    turboJsCapabilityLedger() { return turboJsEngine.capabilityLedger ? turboJsEngine.capabilityLedger() : Object.freeze({ plan: turboJsCapabilityLedgerPlan || null }); },
    turboJsPolicySealAudit() { return turboJsEngine.policySealAudit ? turboJsEngine.policySealAudit() : Object.freeze({ plan: turboJsPolicySealAuditPlan || null }); },
    turboJsRuntimeDenylist() { return turboJsEngine.runtimeDenylist ? turboJsEngine.runtimeDenylist() : Object.freeze({ plan: turboJsRuntimeDenylistPlan || null }); },
    asyncHostQueueSnapshot() {
      return asyncQueue.snapshot();
    },
    currentRouteGeneration() { return asyncQueue.snapshot().generation >>> 0; },
    isRouteGenerationActive(generation) { return (generation >>> 0) === (asyncQueue.snapshot().generation >>> 0); },
    activateRouteGeneration(generation) {
      const active = asyncQueue.setRouteGeneration(generation, 'route-activation');
      domHandles.nextGeneration();
      return active;
    },
    enqueueHostCall(kind, payload) {
      return asyncQueue.enqueue(kind, payload);
    },
    settleHostCall(id, state, value) {
      return asyncQueue.settle(id, state, value);
    },
    requestFetch(input, init) {
      return asyncQueue.requestFetch(input, init);
    },
    fetch(input, init) {
      return asyncQueue.requestFetch(input, init);
    },
    scheduleTimer(callback, delay, repeat) {
      return asyncQueue.scheduleTimer(callback, delay, repeat);
    },
    cancelTimer(id) {
      return asyncQueue.cancelTimer(id);
    },
    eventQueueSnapshot() {
      return eventQueue.snapshot();
    },
    callTurboJs(entry, payload) {
      return asyncQueue.callTurboJs(entry, payload);
    },
    createTurboJsContext(route, source, order) {
      return turboJsEngine.createContext(route, source, order);
    },
    executeTurboJsChunk(chunk, route) {
      return turboJsEngine.executeChunk(chunk, route, bridgeRef || this);
    },
    destroyTurboJsContext(route, source, order) {
      return turboJsEngine.destroyContext(route, source, order);
    },
    turboJsContextSnapshot() {
      return turboJsEngine.contextSnapshot();
    },
    turboJsEngineModuleStatus() {
      return turboJsEngine.moduleStatus();
    },
    turboJsExecutionSnapshot() {
      return turboJsEngine.executionSnapshot();
    },
    turboJsConsoleEvents() {
      return turboJsEngine.consoleEvents();
    },
    clearTurboJsConsoleEvents() {
      return turboJsEngine.clearConsoleEvents();
    },
    turboJsFallbackPolicy() {
      return turboJsEngine.fallbackPolicy();
    },
    turboJsAbiTable() {
      return turboJsEngine.abiTable();
    },
    turboJsHostImportTable() {
      return turboJsEngine.hostImportTable();
    },
    turboJsParityProbe() {
      return turboJsEngine.parityProbe();
    },
    turboJsBytecodeManifest() {
      return turboJsEngine.bytecodeManifest();
    },
    turboJsModuleResolver() {
      return turboJsEngine.moduleResolver();
    },
    turboJsExceptionAbi() {
      return turboJsEngine.exceptionAbi();
    },
    turboJsHostTrapPolicy() {
      return turboJsEngine.hostTrapPolicy();
    },
    turboJsExecutionLifecycle() {
      return turboJsEngine.executionLifecycle();
    },
    turboJsScriptBufferPolicy() {
      return turboJsEngine.scriptBufferPolicy();
    },
    turboJsContextLimitPolicy() {
      return turboJsEngine.contextLimitPolicy();
    },
    turboJsHostCallDispatch() {
      return turboJsEngine.hostCallDispatch();
    },
    turboJsParityContract() {
      return turboJsEngine.parityContract();
    },
    turboJsReleaseFailClosed() {
      return turboJsEngine.releaseFailClosed();
    },
    turboJsModuleGraph() {
      return turboJsEngine.moduleGraph();
    },
    turboJsModuleCacheSnapshot() {
      return turboJsEngine.moduleCacheSnapshot();
    },
    turboJsResolverAudit() {
      return turboJsEngine.resolverAudit();
    },
    turboJsInteropFallback() {
      return turboJsEngine.interopFallback();
    },
    createScriptScope(route, source, order) {
      const record = asyncQueue.enqueue('script.scopeCreate', { route: String(route || ''), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(record.id, 'fulfilled', { isolated: true });
      return Object.freeze({ route: String(route || ''), source: String(source || ''), order: order >>> 0, mode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'route-scoped' });
    },
    checkScriptPolicy(chunk) {
      const record = asyncQueue.enqueue(scriptPolicyPlan && scriptPolicyPlan.policyCheckHostCall ? scriptPolicyPlan.policyCheckHostCall : 'script.policyCheck', { source: chunk && chunk.source ? String(chunk.source) : '', route: chunk && chunk.route ? String(chunk.route) : '' });
      asyncQueue.settle(record.id, 'fulfilled', { allowed: true });
      return Object.freeze({ allowed: true, capabilities: scriptPolicyPlan ? scriptPolicyPlan.defaultCapabilities : '' });
    },
    domRootHandle() { return domHandles.rootHandle(); },
    domHandleSnapshot() { return domHandles.snapshot(); },
    registerDomHandle(node) { return domHandles.register(node); },
    resolveDomHandle(handle) { return domHandles.resolve(handle); },
    revokeDomHandle(handle) { return domHandles.revoke(handle); },
    advanceDomGeneration() { return domHandles.nextGeneration(); },
    teardownRoute(reason = 'route-teardown') {
      const route = globalThis.location && globalThis.location.pathname ? normalizeRoute(globalThis.location.pathname) : '/';
      const contexts = turboJsEngine.destroyRoute(route);
      const events = eventQueue.clear();
      asyncQueue.resetRoute(reason);
      return Object.freeze({ route, contexts: contexts.destroyed, events, reason: String(reason) });
    },
    domSetAttribute(handle, name, value) {
      authorizeHostCapability(hostBridgePlan, 2, String(name || '').length + String(value || '').length, 0);
      const node = domHandles.resolve(handle);
      if (!(node instanceof Element)) throw new Error('DOM attribute target denied');
      if (isUnsafeDomAttribute(name, value)) throw new Error('unsafe DOM attribute denied');
      node.setAttribute(String(name), String(value));
      return true;
    },
    domAppendChild(parentHandle, childHandle) {
      authorizeHostCapability(hostBridgePlan, 3, 8, 0);
      const parent = domHandles.resolve(parentHandle);
      const child = domHandles.resolve(childHandle);
      if (!parent || typeof parent.appendChild !== 'function') throw new Error('DOM append target denied');
      parent.appendChild(child);
      return true;
    },
    dispatchEventBinding(record) {
      const generation = record && record.routeGeneration !== undefined ? record.routeGeneration >>> 0 : asyncQueue.snapshot().generation >>> 0;
      if (generation !== (asyncQueue.snapshot().generation >>> 0)) throw new Error('stale route generation rejected for event dispatch');
      const queued = eventQueue.push(record || {});
      const asyncRecord = asyncQueue.enqueue(eventQueuePlan && eventQueuePlan.dispatchHostCall ? eventQueuePlan.dispatchHostCall : 'event.dispatch', {
        eventName: record && record.eventName ? String(record.eventName) : '',
        attrName: record && record.attrName ? String(record.attrName) : '',
      });
      asyncQueue.settle(asyncRecord.id, 'fulfilled', { handled: true });
      return Object.freeze({
        eventName: record && record.eventName ? String(record.eventName) : '',
        attrName: record && record.attrName ? String(record.attrName) : '',
        queued: queued.queued,
        handled: true,
      });
    },
    root,
  });
  bridgeRef = bridge;
  Object.defineProperty(globalThis, '__venomRuntime', {
    value: bridge,
    configurable: false,
    enumerable: false,
    writable: false,
  });
  return bridge;
}

function safeSourceUrl(source) {
  return String(source || 'chunk').replace(/[^a-zA-Z0-9_./#:-]/g, '_');
}

function injectRemoteScript(chunk) {
  return new Promise((resolve, reject) => {
    const source = String(chunk && chunk.source ? chunk.source : '');
    if (/^(?:https?:)?\/\//i.test(source)) {
      reject(new Error('unvendored network script denied by production runtime: ' + source));
      return;
    }
    if (!document.createElement) {
      reject(new Error('document.createElement is required for remote script chunks'));
      return;
    }
    const script = document.createElement('script');
    if ((chunk.flags & SCRIPT_FLAG.MODULE) !== 0) script.type = 'module';
    if ((chunk.flags & SCRIPT_FLAG.ASYNC) !== 0) script.async = true;
    if ((chunk.flags & SCRIPT_FLAG.DEFER) !== 0) script.defer = true;
    script.src = chunk.source;
    script.onload = () => resolve(chunk);
    script.onerror = () => reject(new Error('failed to load remote script: ' + chunk.source));
    const target = document.head || document.body || document.documentElement;
    if (!target || !target.appendChild) {
      reject(new Error('no document insertion target for remote script'));
      return;
    }
    target.appendChild(script);
  });
}

function removeInjectedScript(script) {
  if (!script) return;
  if (typeof script.remove === 'function') { script.remove(); return; }
  const parent = script.parentNode;
  if (parent && typeof parent.removeChild === 'function') parent.removeChild(script);
}

function normalizeBrowserModulePath(value) {
  const parts = [];
  for (const part of String(value || '').replace(/\\/g, '/').split('/')) {
    if (!part || part === '.') continue;
    if (part === '..') { if (parts.length) parts.pop(); continue; }
    parts.push(part);
  }
  return parts.join('/');
}

function browserModuleDirectory(value) {
  const normalized = normalizeBrowserModulePath(value);
  const slash = normalized.lastIndexOf('/');
  return slash < 0 ? '' : normalized.slice(0, slash + 1);
}


function parseBrowserModuleIdentity(source) {
  const match = String(source || '').match(/\/\*@venom-module-source-v1(?:\r?\n)([0-9a-f]+)\r?\n\*\/(?:\r?\n)?/i);
  if (!match || (match[1].length & 1) !== 0) return '';
  const bytes = new Uint8Array(match[1].length / 2);
  for (let i = 0; i < bytes.length; ++i) bytes[i] = Number.parseInt(match[1].slice(i * 2, i * 2 + 2), 16);
  return normalizeBrowserModulePath(textDecoder.decode(bytes));
}

function parseBrowserModuleMap(source) {
  const match = String(source || '').match(/\/\*@venom-module-map-v1(?:\r?\n)([\s\S]*?)\*\/(?:\r?\n)?/);
  const map = new Map();
  if (!match) return map;
  const decodeHex = (value) => {
    if (!value || (value.length & 1) !== 0 || !/^[0-9a-f]+$/i.test(value)) return '';
    const bytes = new Uint8Array(value.length / 2);
    for (let i = 0; i < bytes.length; ++i) bytes[i] = Number.parseInt(value.slice(i * 2, i * 2 + 2), 16);
    return textDecoder.decode(bytes);
  };
  for (const line of match[1].split('\n')) {
    if (!line) continue;
    const fields = line.split('\t');
    if (fields.length !== 2) continue;
    const specifier = decodeHex(fields[0]);
    const target = decodeHex(fields[1]);
    if (specifier && target) map.set(specifier, normalizeBrowserModulePath(target));
  }
  return map;
}

function createBrowserModuleLinker(chunks, routeName) {
  const modules = new Map();
  const importMaps = new Map();
  const urls = new Map();
  const state = new Map();
  const authoredAliases = new Map();
  const ambiguousAliases = new Set();
  const route = String(routeName || '');
  for (const chunk of chunks || []) {
    if (!chunk || (chunk.flags & SCRIPT_FLAG.BROWSER) === 0 || (chunk.flags & SCRIPT_FLAG.MODULE) === 0) continue;
    const routeMatches = chunk.route === route || chunk.route === '*' || chunk.route === '';
    const dependencyChunk = (chunk.flags & SCRIPT_FLAG.DEPENDENCY) !== 0;
    if (!routeMatches && !dependencyChunk) continue;
    const normalizedSource = normalizeBrowserModulePath(chunk.source);
    modules.set(normalizedSource, chunk);
    importMaps.set(normalizedSource, parseBrowserModuleMap(chunk.code));
    const authoredSource = parseBrowserModuleIdentity(chunk.code);
    if (authoredSource) {
      const prior = authoredAliases.get(authoredSource);
      if (prior && prior !== normalizedSource) {
        authoredAliases.delete(authoredSource);
        ambiguousAliases.add(authoredSource);
      } else if (!ambiguousAliases.has(authoredSource)) {
        authoredAliases.set(authoredSource, normalizedSource);
      }
    }
  }

  const resolve = (referrer, specifier) => {
    if (!specifier || specifier[0] !== '.') return '';
    const normalizedReferrer = normalizeBrowserModulePath(referrer);
    const mapped = importMaps.get(normalizedReferrer);
    const opaqueTarget = mapped && mapped.get(specifier);
    if (opaqueTarget && modules.has(opaqueTarget)) return opaqueTarget;

    // Production packaging can replace source labels at more than one stage. If
    // the compiler-authored map is present but attached to a different opaque
    // alias, accept it only when the specifier resolves uniquely. This remains
    // fail-closed for ambiguous graphs.
    let uniqueMappedTarget = '';
    for (const candidateMap of importMaps.values()) {
      const candidate = candidateMap && candidateMap.get(specifier);
      if (!candidate || !modules.has(candidate)) continue;
      if (uniqueMappedTarget && uniqueMappedTarget !== candidate) {
        uniqueMappedTarget = '';
        break;
      }
      uniqueMappedTarget = candidate;
    }
    if (uniqueMappedTarget) return uniqueMappedTarget;
    const base = normalizeBrowserModulePath(browserModuleDirectory(referrer) + specifier);
    const candidates = [base];
    if (!/\.[A-Za-z0-9]+$/.test(base)) {
      for (const ext of ['.js','.mjs','.cjs','.jsx','.ts','.tsx','.mts','.cts']) candidates.push(base + ext);
      for (const ext of ['.js','.mjs','.jsx','.ts','.tsx','.mts']) candidates.push(base + '/index' + ext);
    } else if (base.endsWith('.js')) {
      const stem = base.slice(0, -3);
      candidates.push(stem + '.ts', stem + '.tsx', stem + '.mts');
    }
    for (const candidate of candidates) {
      if (modules.has(candidate)) return candidate;
      const aliased = authoredAliases.get(candidate);
      if (aliased && modules.has(aliased)) return aliased;
    }
    return '';
  };

  const rewrite = (source, referrer, buildUrl) => {
    const patterns = [
      /((?:^|[;\n\r])\s*import\s+(?:[^'";]+?\s+from\s*)?)(['"])([^'"]+)\2/g,
      /(\bexport\s+(?:\*|\{[^}]*\})\s+from\s*)(['"])([^'"]+)\2/g
    ];
    let output = source;
    for (const pattern of patterns) {
      output = output.replace(pattern, (whole, prefix, quote, specifier) => {
        if (!String(specifier).startsWith('.')) return whole;
        const dependency = resolve(referrer, specifier);
        if (!dependency) throw new Error('browser module dependency not found: ' + referrer + ' -> ' + specifier);
        return prefix + quote + buildUrl(dependency) + quote;
      });
    }
    return output;
  };

  const buildUrl = (sourceName) => {
    const normalized = normalizeBrowserModulePath(sourceName);
    if (urls.has(normalized)) return urls.get(normalized);
    if (state.get(normalized) === 1) throw new Error('browser module cycle requires bundling support: ' + normalized);
    const chunk = modules.get(normalized);
    if (!chunk) throw new Error('browser module is unavailable: ' + normalized);
    state.set(normalized, 1);
    const linked = rewrite(String(chunk.code || ''), normalized, buildUrl) + '\n//# sourceURL=venom-browser://' + safeSourceUrl(normalized);
    const url = URL.createObjectURL(new Blob([linked], { type: 'text/javascript' }));
    urls.set(normalized, url);
    state.set(normalized, 2);
    return url;
  };

  return {
    urlFor(chunk) { return buildUrl(normalizeBrowserModulePath(chunk && chunk.source)); },
    dispose() { for (const url of urls.values()) URL.revokeObjectURL(url); urls.clear(); }
  };
}

async function executeBrowserScriptChunk(chunk, moduleLinker = null) {
  // Manifest V3 extension pages cannot execute blob-backed modules under their
  // mandatory CSP. Chrome-facing browser adapters are emitted as hardened,
  // physical extension resources and loaded by the generated route shell.
  // Keep their packaged copies for graph/audit metadata, but never execute
  // those copies through the website blob-module linker.
  if (typeof location !== 'undefined' && location.protocol === 'chrome-extension:') {
    return Object.freeze({ ...chunk, executed: false, external: true, browser: true, chromeExtension: true });
  }
  if (chunk && chunk.bytecodeBytes && chunk.bytecodeBytes.length) {
    throw new Error('browser script chunk was packaged as TurboJS bytecode: ' + (chunk.source || '<script>'));
  }
  if (!chunk || !chunk.code || !String(chunk.code).trim()) return Object.freeze({ ...chunk, executed: false, empty: true, browser: true });
  if (typeof document === 'undefined' || !document.createElement) throw new Error('browser script execution requires document.createElement');
  const script = document.createElement('script');
  const isModule = (chunk.flags & SCRIPT_FLAG.MODULE) !== 0;
  if (isModule) script.type = 'module';
  script.async = false;
  const source = String(chunk.code) + '\n//# sourceURL=venom-browser://' + safeSourceUrl(chunk.source);
  const target = document.head || document.body || document.documentElement;
  if (!target || !target.appendChild) throw new Error('no document insertion target for browser script');
  if (isModule) {
    const linkedUrl = moduleLinker ? moduleLinker.urlFor(chunk) : '';
    const ownedUrl = !linkedUrl;
    const url = linkedUrl || URL.createObjectURL(new Blob([source], { type: 'text/javascript' }));
    try { await new Promise((resolve, reject) => { script.src=url; script.onload=resolve; script.onerror=()=>reject(new Error('browser module execution failed: '+chunk.source)); target.appendChild(script); }); }
    finally { if (ownedUrl) URL.revokeObjectURL(url); removeInjectedScript(script); }
  } else { script.textContent = source; target.appendChild(script); removeInjectedScript(script); }
  return Object.freeze({ ...chunk, executed: true, browser: true });
}

async function executeScriptChunk(chunk, route, scriptIsolationPlan = null, scriptPolicyPlan = null, turboJsChunkPlan = null, turboJsEnginePlan = null, scriptEnginePolicyPlan = null, browserModuleLinker = null) {
  const bridge = globalThis.__venomRuntime || null;
  if (bridge && typeof bridge.checkScriptPolicy === 'function') bridge.checkScriptPolicy(chunk);
  if (bridge && typeof bridge.createScriptScope === 'function') bridge.createScriptScope(route && route.route ? route.route : chunk.route, chunk.source, chunk.order);
  const startRecord = bridge && typeof bridge.enqueueHostCall === 'function'
    ? bridge.enqueueHostCall(scriptPolicyPlan && scriptPolicyPlan.chunkStartHostCall ? scriptPolicyPlan.chunkStartHostCall : 'script.chunkStart', { route: chunk.route, source: chunk.source, order: chunk.order })
    : null;
  if (bridge && typeof bridge.callTurboJs === 'function') {
    const tjsBoundaryRecord = bridge.callTurboJs('chunk:' + chunk.order, { route: chunk.route, source: chunk.source, bytes: chunk.code ? chunk.code.length : 0, boundary: turboJsChunkPlan ? turboJsChunkPlan.mode : 'engine-input', engine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'bootstrap' });
    if (bridge && typeof bridge.settleHostCall === 'function' && tjsBoundaryRecord && tjsBoundaryRecord.id) {
      bridge.settleHostCall(tjsBoundaryRecord.id, 'fulfilled', { boundary: turboJsChunkPlan ? turboJsChunkPlan.mode : 'engine-input', engine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'bootstrap' });
    }
  }
  let executed;
  if ((chunk.flags & SCRIPT_FLAG.BROWSER) !== 0) executed = await executeBrowserScriptChunk(chunk, browserModuleLinker);
  else if (bridge && typeof bridge.executeTurboJsChunk === 'function') {
    executed = await bridge.executeTurboJsChunk(chunk, route);
  } else {
    const denyHostFallback = !!(scriptEnginePolicyPlan && String(scriptEnginePolicyPlan.fallback || '').includes('deny-host-js'));
    if (denyHostFallback) throw new Error('TurboJS/WASM backend unavailable; protected host JS fallback denied');
    const fallbackRecord = bridge && typeof bridge.enqueueHostCall === 'function'
      ? bridge.enqueueHostCall(scriptEnginePolicyPlan && scriptEnginePolicyPlan.fallback ? 'script.engineFallback' : 'script.engineFallback', { route: chunk.route, source: chunk.source, order: chunk.order })
      : null;
    if ((chunk.flags & SCRIPT_FLAG.REMOTE_URL) !== 0) {
      executed = await injectRemoteScript(chunk);
    } else if ((!chunk.code || !String(chunk.code).trim()) && !(chunk.bytecodeBytes && chunk.bytecodeBytes.length)) {
      executed = Object.freeze({ ...chunk, executed: false, empty: true });
    } else {
__VENOM_RUNTIME_LEGACY_FALLBACK_BLOCK__
    }
    if (bridge && fallbackRecord) bridge.settleHostCall(fallbackRecord.id, 'fulfilled', { executed: !!executed.executed });
  }
  if (bridge && startRecord) bridge.settleHostCall(startRecord.id, 'fulfilled', { executed: !!(executed && executed.executed), isolated: !!scriptIsolationPlan, engine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'none' });
  return Object.freeze({ ...executed, isolated: !!scriptIsolationPlan, isolationMode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'none', turboJsBoundary: turboJsChunkPlan ? turboJsChunkPlan.mode : 'none', turboJsEngine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'none' });
}

async function executeScriptsForRoute(route, jsBundle, scriptIsolationPlan = null, scriptPolicyPlan = null, turboJsChunkPlan = null, turboJsEnginePlan = null, scriptEnginePolicyPlan = null) {
  assertRuntimeIntegrity(activeRuntimeOpcodeMap);
  if (!jsBundle || !Array.isArray(jsBundle.chunks)) return [];
  const selected = jsBundle.chunks.filter((chunk) => ((chunk.route === route.route || chunk.route === '*' || chunk.route === '') && (chunk.flags & SCRIPT_FLAG.DEPENDENCY) === 0));
  const executed = [];

  // Venom reconstructs route DOM after the browser's native DOMContentLoaded/load
  // lifecycle may already have completed. Browser-side application scripts still
  // expect those startup hooks. Capture late registrations during route script
  // execution and replay them once, in document order, after all route scripts load.
  const lateLifecycleHandlers = [];
  const originalDocumentAdd = document.addEventListener;
  const originalWindowAdd = globalThis.addEventListener;
  document.addEventListener = function(type, listener, options) {
    if (type === 'DOMContentLoaded' && document.readyState !== 'loading' && typeof listener === 'function') {
      lateLifecycleHandlers.push({ target: document, type, listener, options });
      return;
    }
    return originalDocumentAdd.call(this, type, listener, options);
  };
  globalThis.addEventListener = function(type, listener, options) {
    if (type === 'load' && document.readyState === 'complete' && typeof listener === 'function') {
      lateLifecycleHandlers.push({ target: globalThis, type, listener, options });
      return;
    }
    return originalWindowAdd.call(this, type, listener, options);
  };

  const browserModuleLinker = createBrowserModuleLinker(jsBundle.chunks, route.route);
  try {
    for (const chunk of selected) {
      // Keep script execution deterministic: execute in document order.
      executed.push(await executeScriptChunk(chunk, route, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan, browserModuleLinker));
    }
  } finally {
    browserModuleLinker.dispose();
    document.addEventListener = originalDocumentAdd;
    globalThis.addEventListener = originalWindowAdd;
  }

  for (const record of lateLifecycleHandlers) {
    const event = new Event(record.type);
    try {
      record.listener.call(record.target, event);
    } catch (error) {
      console.error('[venom] route lifecycle handler failed', error);
    }
  }
  return executed;
}

__VENOM_BROWSER_ASSET_RUNTIME__

function parseNumberLiteral(value) {
  const text = String(value || '').trim();
  if (/^0x[0-9a-f]+$/i.test(text)) return Number.parseInt(text.slice(2), 16) >>> 0;
  return Number.parseInt(text, 10) >>> 0;
}

function parseWordLayout(value) {
  const layout = String(value || '').split(',').map((item) => item.trim().toLowerCase()).filter(Boolean);
  if (layout.length !== 4) throw new Error('invalid polymorphic word layout');
  const seen = new Set(layout);
  for (const required of ['opcode', 'a', 'b', 'c']) {
    if (!seen.has(required)) throw new Error('polymorphic word layout is missing ' + required);
  }
  return layout;
}

function parseOperandMasks(value) {
  const parts = String(value || '').split(',').map((item) => parseNumberLiteral(item));
  if (parts.length !== 3) throw new Error('invalid polymorphic operand mask list');
  return parts;
}

function parsePolymorphicMap(section) {
  const data = section.data;
  requireAscii(data, 'VPOL0010', 'polymorphic VM plan');
  if (data.length < 72) throw new Error('polymorphic VM plan is truncated');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const version = readU32(view, 8);
  if (version !== 2) throw new Error('unsupported polymorphic VM plan version: ' + version);
  const physicalToLogical = new Map();
  const logicalToPhysical = new Map();
  const opcodeMask = readU32(view, 20);
  const operandMasks = [readU32(view, 24), readU32(view, 28), readU32(view, 32)];
  const wordNames = ['opcode', 'a', 'b', 'c'];
  const wordLayout = [];
  for (let i = 0; i < 4; ++i) {
    const id = data[36 + i];
    if (id > 3) throw new Error('invalid polymorphic VM word id');
    wordLayout.push(wordNames[id]);
  }
  const count = readU32(view, 48);
  const mapBase = 72;
  if (mapBase + count * 4 > data.length) throw new Error('polymorphic VM opcode map is truncated');
  for (let i = 0; i < count; ++i) {
    const base = mapBase + i * 4;
    const logical = view.getUint16(base, true);
    const physical = view.getUint16(base + 2, true);
    physicalToLogical.set(physical, logical);
    logicalToPhysical.set(logical, physical);
  }
  if (!physicalToLogical.has(LOGICAL.CREATE_ELEMENT) && physicalToLogical.size === 0) {
    throw new Error('missing polymorphic opcode map');
  }
  return { physicalToLogical, logicalToPhysical, wordLayout, opcodeMask, operandMasks, source: '' };
}

function decodeVmInstruction(vm, pc, opcodeMap) {
  const words = [
    readU32(vm.view, pc + 0),
    readU32(vm.view, pc + 4),
    readU32(vm.view, pc + 8),
    readU32(vm.view, pc + 12),
  ];
  let physical = 0;
  let a = 0;
  let b = 0;
  let c = 0;
  for (let i = 0; i < opcodeMap.wordLayout.length; ++i) {
    const field = opcodeMap.wordLayout[i];
    const value = words[i] >>> 0;
    if (field === 'opcode') physical = (value ^ opcodeMap.opcodeMask) >>> 0;
    else if (field === 'a') a = (value ^ opcodeMap.operandMasks[0]) >>> 0;
    else if (field === 'b') b = (value ^ opcodeMap.operandMasks[1]) >>> 0;
    else if (field === 'c') c = (value ^ opcodeMap.operandMasks[2]) >>> 0;
    else throw new Error('unknown polymorphic instruction field: ' + field);
  }
  return { physical, a, b, c };
}

function normalizeRoute(pathname) {
  let route = pathname || '/';
  if (!route.startsWith('/')) route = '/' + route;
  if (route.length > 1 && route.endsWith('/')) route = route.slice(0, -1);
  if (route === '/index.html' || route === '/index.htm') return '/';
  if (route.length > 5 && route.endsWith('.html')) route = route.slice(0, -5);
  if (route.length > 4 && route.endsWith('.htm')) route = route.slice(0, -4);
  if (route.length > 1 && route.endsWith('/index')) route = route.slice(0, -6) || '/';
  return route;
}

function selectRoute(routes, pathname) {
  if (venomRouteResolver) return venomRouteResolver(pathname);
  const wanted = normalizeRoute(pathname);
  return routes.find((route) => route.route === wanted) ||
         routes.find((route) => route.route === '/') ||
         routes[0];
}

function createDomMutationBatch(maxOperations = 256, maxStringBytes = 65536) {
  const OP_SET_ATTRIBUTE = 1, OP_APPEND_CHILD = 2, OP_APPEND_TEXT = 3;
  const packet = new Uint32Array(Math.max(1, maxOperations) * 4);
  const nodes = [], nodeIds = new WeakMap(), strings = [];
  let operationCount = 0, stringBytes = 0, flushCount = 0;
  function nodeId(node) { let id = nodeIds.get(node); if (id !== undefined) return id; id = nodes.length; nodes.push(node); nodeIds.set(node, id); return id; }
  function stringId(value) { const text = String(value == null ? '' : value); const bytes = new TextEncoder().encode(text).byteLength; if (bytes > maxStringBytes) throw new Error('DOM batch string budget exceeded'); if (operationCount > 0 && stringBytes + bytes > maxStringBytes) flush(); const id = strings.length; strings.push(text); stringBytes += bytes; return id; }
  function emit(op, a, b, c) { if (operationCount >= maxOperations) flush(); const o = operationCount * 4; packet[o] = op >>> 0; packet[o+1] = a >>> 0; packet[o+2] = b >>> 0; packet[o+3] = c >>> 0; operationCount++; }
  function flush() { if (!operationCount) return 0; for (let i=0;i<operationCount;i++) { const o=i*4, op=packet[o], a=packet[o+1], b=packet[o+2], c=packet[o+3]; if (op===OP_SET_ATTRIBUTE) { const target=nodes[a]; if (!(target instanceof Element)) throw new Error('DOM batch attribute target denied'); target.setAttribute(strings[b], strings[c]); } else if (op===OP_APPEND_CHILD) { const parent=nodes[a], child=nodes[b]; if (!parent || typeof parent.appendChild !== 'function' || !child) throw new Error('DOM batch append target denied'); parent.appendChild(child); } else if (op===OP_APPEND_TEXT) { const parent=nodes[a]; if (!parent || typeof parent.appendChild !== 'function') throw new Error('DOM batch text target denied'); parent.appendChild(document.createTextNode(strings[b])); } else throw new Error('unknown DOM batch opcode'); } const flushed=operationCount; packet.fill(0,0,operationCount*4); operationCount=0; stringBytes=0; strings.length=0; flushCount++; return flushed; }
  return Object.freeze({ queueSetAttribute(node,name,value){emit(OP_SET_ATTRIBUTE,nodeId(node),stringId(name),stringId(value));}, queueAppendChild(parent,child){emit(OP_APPEND_CHILD,nodeId(parent),nodeId(child),0);}, queueAppendText(parent,value){emit(OP_APPEND_TEXT,nodeId(parent),stringId(value),0);}, flush, stats(){return Object.freeze({pending:operationCount,flushes:flushCount,nodes:nodes.length});} });
}

function appendPending(stack, pending, domBatch = null) {
  if (!pending) throw new Error('VM append_child without pending node');
  if (domBatch) domBatch.queueAppendChild(stack[stack.length - 1], pending);
  else stack[stack.length - 1].appendChild(pending);
}

function currentElement(stack) {
  const element = stack[stack.length - 1];
  if (!(element instanceof Element)) throw new Error('VM current node is not an element');
  return element;
}

function executeRoute(route, vm, strings, opcodeMap, root, assetManifest = new Map(), assetBaseUrl = document.baseURI, hostBridgePlan = null) {
  if (!route) throw new Error('no route available');
  const start = vm.dataOffset + route.bytecodeOffset;
  const end = start + route.bytecodeSize;
  if (start < vm.dataOffset || end > vm.data.length) throw new Error('route bytecode range is invalid');

  const stack = [root];
  let pending = null;
  let pc = start;
  let executed = 0;
  const domBatch = createDomMutationBatch(256, 65536);

  root.replaceChildren();
  root.setAttribute('data-venom-route', route.route);
  root.setAttribute('data-venom-runtime', 'vm');

  while (pc < end) {
    const decoded = decodeVmInstruction(vm, pc, opcodeMap);
    const { physical, a, b, c } = decoded;
    const op = opcodeMap.physicalToLogical.get(physical);
    pc += vm.instructionSize;
    executed++;

    if (op === undefined) throw new Error('unknown physical opcode: ' + physical);
    if (op === LOGICAL.NOP || op === LOGICAL.LOAD_CONST || op === LOGICAL.CALL_TURBOJS) {
      continue;
    }
    if (op === LOGICAL.CREATE_ELEMENT) {
      if (pending) appendPending(stack, pending, domBatch);
      pending = document.createElement(strings[a] || 'div');
      continue;
    }
    if (op === LOGICAL.ENTER_ELEMENT) {
      if (!pending) throw new Error('VM enter_element without pending element');
      stack.push(pending);
      pending = null;
      continue;
    }
    if (op === LOGICAL.SET_ATTRIBUTE) {
      const target = pending || currentElement(stack);
      if (!(target instanceof Element)) throw new Error('VM set_attribute target is not an element');
      const attrName = strings[a] || '';
      let attrValue = strings[b] || '';
      if (shouldRemapAttribute(attrName)) {
        attrValue = attrName === 'srcset' ? resolveSrcset(route, attrValue, assetManifest, assetBaseUrl) : resolveAssetValue(route, attrValue, assetManifest, assetBaseUrl);
      }
      if (isUnsafeDomAttribute(attrName, attrValue)) throw new Error('unsafe DOM attribute denied');
      domBatch.queueSetAttribute(target, attrName, attrValue);
      if (isInlineEventAttribute(attrName)) bindInlineEventAttribute(target, attrName, attrValue, hostBridgePlan);
      continue;
    }
    if (op === LOGICAL.SET_TEXT) {
      if (pending) domBatch.queueAppendText(pending, strings[a] || '');
      else domBatch.queueAppendText(stack[stack.length - 1], strings[a] || '');
      continue;
    }
    if (op === LOGICAL.LEAVE_ELEMENT) {
      if (stack.length <= 1) throw new Error('VM leave_element tried to pop root');
      if (pending) appendPending(stack, pending, domBatch);
      pending = stack.pop();
      continue;
    }
    if (op === LOGICAL.APPEND_CHILD) {
      appendPending(stack, pending, domBatch);
      pending = null;
      continue;
    }
    if (op === LOGICAL.HALT) {
      break;
    }
    throw new Error('unhandled logical opcode: ' + op + ' a=' + a + ' b=' + b + ' c=' + c);
  }

  if (pending) appendPending(stack, pending, domBatch);
  domBatch.flush();
  root.setAttribute('data-venom-dom-batches', String(domBatch.stats().flushes));
  root.setAttribute('data-venom-instructions', String(executed));
}

function installNavigation(routes, routeRuntimeLoader, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan, scriptIsolationPlan = null, scriptPolicyPlan = null, turboJsChunkPlan = null, turboJsEnginePlan = null, scriptEnginePolicyPlan = null) {
  document.addEventListener('click', (event) => {
    const anchor = event.target && event.target.closest ? event.target.closest('a[href]') : null;
    if (!anchor) return;
    const url = new URL(anchor.getAttribute('href'), location.href);
    if (url.origin !== location.origin) return;
    const targetRoute = selectRoute(routes, url.pathname);
    if (!targetRoute || normalizeRoute(url.pathname) !== targetRoute.route) return;
    event.preventDefault();
    history.pushState({ venomRoute: targetRoute.route }, '', url.pathname);
    try {
      const bridge = globalThis.__venomRuntime || null;
      if (bridge && typeof bridge.teardownRoute === 'function') bridge.teardownRoute('navigation');
      routeRuntimeLoader.dispose();
      const runtime = routeRuntimeLoader(targetRoute);
      if (bridge && typeof bridge.activateRouteGeneration === 'function') bridge.activateRouteGeneration(runtime.routeGeneration);
      setActiveBrowserAssetRoute(runtime.route);
      executeRoute(runtime.route, runtime.vm, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan);
      executeScriptsForRoute(runtime.route, runtime.jsBundle, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan).catch((error) => console.error('[venom] route script failed', error));
    } catch (error) {
      console.error('[venom] route load failed', error);
      throw error;
    }
  });

  window.addEventListener('popstate', () => {
    const targetRoute = selectRoute(routes, location.pathname);
    const bridge = globalThis.__venomRuntime || null;
    if (bridge && typeof bridge.teardownRoute === 'function') bridge.teardownRoute('history-navigation');
    routeRuntimeLoader.dispose();
    const runtime = routeRuntimeLoader(targetRoute);
    if (bridge && typeof bridge.activateRouteGeneration === 'function') bridge.activateRouteGeneration(runtime.routeGeneration);
    setActiveBrowserAssetRoute(runtime.route);
    executeRoute(runtime.route, runtime.vm, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan);
    executeScriptsForRoute(runtime.route, runtime.jsBundle, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan).catch((error) => console.error('[venom] route script failed', error));
  });
}




function parsePackageBindingMetadata(section) {
  const plan = parseKeyValueMetadata(section, 'VENOM_PACKAGE_BINDING_V1');
  const assets = new Map();
  for (const line of plan.lines) {
    if (!line.startsWith('asset\t')) continue;
    const parts = line.split('\t');
    if (parts.length !== 4) throw new Error('invalid package binding asset row');
    assets.set(parts[1], Object.freeze({ role: parts[1], name: parts[2], sha256: parts[3] }));
  }
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    tokenHash: plan.values.get('binding_token_sha256') || '',
    hardened: plan.values.get('hardened') === 'true',
    assetCount: Number.parseInt(plan.values.get('asset_count') || String(assets.size), 10) >>> 0,
    assets,
  });
}

async function fetchBytesForBinding(url, label) {
  if (!url) throw new Error('missing bound asset URL for ' + label);
  const response = await fetch(url, { cache: 'force-cache' });
  if (!response || !response.ok) throw new Error('failed to fetch bound asset ' + label + ': ' + (response ? response.status : 'no-response'));
  return new Uint8Array(await response.arrayBuffer());
}

async function verifyBoundAsset(binding, role, url) {
  const expected = binding.assets.get(role);
  if (!expected) throw new Error('package binding is missing asset role: ' + role);
  const bytes = await fetchBytesForBinding(url, role);
  const actual = sha256Hex(bytes);
  if (actual !== expected.sha256) throw new Error('bound asset hash mismatch: ' + role);
  return true;
}

async function verifyPackageBinding(pkg, options) {
  const section = findOptionalSection(pkg, SECTION.INTEGRITY, 'package-binding.vbind');
  const hardenedFlag = (pkg.flags & PACKAGE_FLAG.RUNTIME_HARDENED) !== 0;
  if (!section) {
    if (hardenedFlag || (options && options.hardened)) throw new Error('hardened package is missing package binding metadata');
    return Object.freeze({ enabled: false, verified: false });
  }
  const binding = parsePackageBindingMetadata(section);
  if (!binding.enabled) throw new Error('package binding metadata is disabled');
  if (binding.hardened && !hardenedFlag) throw new Error('package binding expects runtime-hardened package');
  if (binding.tokenHash) {
    const token = options && options.bindingToken ? String(options.bindingToken) : '';
    if (!token && options && options.hardened) throw new Error('loader did not provide package binding token');
    if (token && sha256Hex(textEncoder.encode(token)) !== binding.tokenHash) throw new Error('loader/package binding token mismatch');
  }
  if (binding.assets.size !== binding.assetCount) throw new Error('package binding asset count mismatch');
  await verifyBoundAsset(binding, 'runtime', options && options.runtimeUrl ? options.runtimeUrl : import.meta.url);
  if (options && options.runtimeWasmUrl) await verifyBoundAsset(binding, 'runtime_wasm', options.runtimeWasmUrl);
  if (options && options.styleUrl) await verifyBoundAsset(binding, 'style', options.styleUrl);
  if (options && options.turboJsEngineUrl) await verifyBoundAsset(binding, 'turbojs_engine', options.turboJsEngineUrl);
  if (options && options.turboJsWasmUrl) await verifyBoundAsset(binding, 'turbojs_wasm', options.turboJsWasmUrl);
  if (options && options.workerUrl) await verifyBoundAsset(binding, 'worker', options.workerUrl);
  return Object.freeze({ enabled: true, verified: true, assets: binding.assets.size });
}

export function renderVenomFailure(root, error) {
  const message = error && error.message ? error.message : String(error || 'unknown error');
  if (!root) return;
  if (typeof root.replaceChildren === 'function' && document && typeof document.createElement === 'function') {
    const panel = document.createElement('div');
    panel.setAttribute('data-venom-failure', 'true');
    panel.setAttribute('role', 'alert');
    panel.textContent = 'Venom boot failed: ' + message;
    root.replaceChildren(panel);
    return;
  }
  root.textContent = 'Venom boot failed: ' + message;
}

export async function bootVenom(options) {
  const root = options.root || document.body;
  if (!root) throw new Error('missing Venom root element');
  const reportBootPhase = options && typeof options.onBootPhase === 'function' ? options.onBootPhase : () => {};
  const phaseStarted = new Map();
  const startPhase = (phase) => { phaseStarted.set(phase, performance.now()); reportBootPhase(Object.freeze({ phase, state: 'start', at: Date.now() })); };
  const finishPhase = (phase, detail) => { const started = phaseStarted.get(phase); const durationMs = started === undefined ? 0 : Math.max(0, performance.now() - started); reportBootPhase(Object.freeze({ phase, state: 'complete', at: Date.now(), durationMs, detail: detail || null })); };

  startPhase('package-load');
  const pkg = await loadPackage(options.packageUrl, options.expectedPackageHash, options.packageBytes || null);
  finishPhase('package-load', { sections: pkg.sections.length, decodedSections: pkg.decodedSectionCount });
  startPhase('package-policy');
  const packageFeatures = verifyPackageFeatures(pkg);
  const packageBinding = await verifyPackageBinding(pkg, options);
  const runtimePolicy = enforceRuntimePolicy(pkg, parseRuntimePolicy(findOptionalSection(pkg, SECTION.INTEGRITY, 'runtime-policy.vhrd')), options);
  activeReleaseDiversification = parseReleaseDiversification(findOptionalSection(pkg, SECTION.INTEGRITY, 'release-diversification.vrd3'), runtimePolicy.releaseProfile);
  activeTurboJsAbiFingerprint = parseTurboJsAbiFingerprint(findOptionalSection(pkg, SECTION.INTEGRITY, 'turbojs-abi-fingerprint.vqaf'), runtimePolicy.releaseProfile);
  const strings = parseStringTable(findSection(pkg, SECTION.STRINGS, 'strings.vstr'));
  const routes = parseRouteTable(findSection(pkg, SECTION.ROUTES, 'routes.vbrt'), strings);
  const lazySectionsPlan = parseLazySectionsMetadata(findOptionalSection(pkg, SECTION.INTEGRITY, 'lazy-sections.vlazy'));
  const routeRuntimeLoader = makeRouteRuntimeLoader(pkg, lazySectionsPlan, runtimePolicy.failClosed);
  const opcodeMap = parsePolymorphicMap(findSection(pkg, SECTION.INTEGRITY, 'vm-polymorph.vpol'));
  activeRuntimeOpcodeMap = opcodeMap;
  activeRuntimeIntegritySeal = computeRuntimeIntegritySeal(opcodeMap);
  assertRuntimeIntegrity(opcodeMap);
  const assetManifest = parseAssetManifest(findOptionalSection(pkg, SECTION.ASSET_MANIFEST, 'asset-manifest.txt'));
  const hostBridgePlan = parseHostBridgeMetadata(findOptionalSection(pkg, SECTION.HOST_BRIDGE, 'host-calls.vhcb'));
  const fetchBridgePlan = parseFetchBridgeMetadata(findOptionalSection(pkg, SECTION.FETCH_BRIDGE, 'fetch-bridge.vfch'));
  const timerBridgePlan = parseTimerBridgeMetadata(findOptionalSection(pkg, SECTION.TIMER_BRIDGE, 'timer-bridge.vtmr'));
  const eventQueuePlan = parseEventQueueMetadata(findOptionalSection(pkg, SECTION.EVENT_QUEUE, 'event-queue.vevq'));
  const turboJsBridgePlan = parseTurboJsBridgeMetadata(findOptionalSection(pkg, SECTION.TURBOJS_BRIDGE, 'turbojs-bridge.vtjs'));
  const scriptIsolationPlan = parseScriptIsolationMetadata(findOptionalSection(pkg, SECTION.SCRIPT_ISOLATION, 'script-isolation.vsis'));
  const scriptPolicyPlan = parseScriptPolicyMetadata(findOptionalSection(pkg, SECTION.SCRIPT_POLICY, 'script-policy.vscp'));
  const turboJsChunkPlan = parseTurboJsChunkMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CHUNKS, 'turbojs-chunks.vqbc'));
  const turboJsEnginePlan = parseTurboJsEngineMetadata(findOptionalSection(pkg, SECTION.TURBOJS_ENGINE, 'turbojs-engine.vqse'));
  const scriptEnginePolicyPlan = parseScriptEnginePolicyMetadata(findOptionalSection(pkg, SECTION.SCRIPT_ENGINE_POLICY, 'script-engine-policy.vsep'));
  const turboJsEngineModulePlan = parseTurboJsEngineModuleMetadata(findOptionalSection(pkg, SECTION.TURBOJS_ENGINE_MODULE, 'turbojs-engine-module.vqem'));
  const turboJsContextLifecyclePlan = parseTurboJsContextLifecycleMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CONTEXT_LIFECYCLE, 'turbojs-context-lifecycle.vqcl'));
  const hostCapabilitiesPlan = parseHostCapabilitiesMetadata(findOptionalSection(pkg, SECTION.HOST_CAPABILITIES, 'host-capabilities.vhcap'));
  const turboJsAdapterDiagnosticsPlan = parseTurboJsAdapterDiagnosticsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_ADAPTER_DIAGNOSTICS, 'turbojs-adapter-diagnostics.vqad'));
  const turboJsWasmRuntimePlan = parseTurboJsWasmRuntimeMetadata(findOptionalSection(pkg, SECTION.TURBOJS_WASM_RUNTIME, 'turbojs-wasm-runtime.vqwr'));
  const turboJsSourceTransferPlan = parseTurboJsSourceTransferMetadata(findOptionalSection(pkg, SECTION.TURBOJS_SOURCE_TRANSFER, 'turbojs-source-transfer.vqst'));
  const turboJsConsoleBridgePlan = parseTurboJsConsoleBridgeMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CONSOLE_BRIDGE, 'turbojs-console-bridge.vqcb'));
  const turboJsExecutionRecordsPlan = parseTurboJsExecutionRecordsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EXECUTION_RECORDS, 'turbojs-execution-records.vqer'));
  const turboJsResultBridgePlan = parseTurboJsResultBridgeMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RESULT_BRIDGE, 'turbojs-result-bridge.vqrb'));
  const turboJsFallbackPolicyPlan = parseTurboJsFallbackPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_FALLBACK_POLICY, 'turbojs-fallback-policy.vqfp'));
  const turboJsRuntimeAbiPlan = parseTurboJsRuntimeAbiMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RUNTIME_ABI, 'turbojs-runtime-abi.vqra'));
  const turboJsHostImportsPlan = parseTurboJsHostImportsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_IMPORTS, 'turbojs-host-imports.vqhi'));
  const turboJsHeapLimitsPlan = parseTurboJsHeapLimitsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HEAP_LIMITS, 'turbojs-heap-limits.vqhl'));
  const turboJsScriptBufferPlan = parseTurboJsScriptBufferMetadata(findOptionalSection(pkg, SECTION.TURBOJS_SCRIPT_BUFFER, 'turbojs-script-buffer.vqsb'));
  const turboJsConsoleAbiPlan = parseTurboJsConsoleAbiMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CONSOLE_ABI, 'turbojs-console-abi.vqca'));
  const turboJsParityProbePlan = parseTurboJsParityProbeMetadata(findOptionalSection(pkg, SECTION.TURBOJS_PARITY_PROBE, 'turbojs-parity-probe.vqpp'));
  const turboJsReleaseFallbackPlan = parseTurboJsReleaseFallbackMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RELEASE_FALLBACK, 'turbojs-release-fallback.vqrf'));
  const turboJsBytecodeManifestPlan = parseTurboJsBytecodeManifestMetadata(findOptionalSection(pkg, SECTION.TURBOJS_BYTECODE_MANIFEST, 'turbojs-bytecode-manifest.vqbm'));
  const turboJsModuleResolverPlan = parseTurboJsModuleResolverMetadata(findOptionalSection(pkg, SECTION.TURBOJS_MODULE_RESOLVER, 'turbojs-module-resolver.vqmr'));
  const turboJsExceptionAbiPlan = parseTurboJsExceptionAbiMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EXCEPTION_ABI, 'turbojs-exception-abi.vqex'));
  const turboJsHostTrapPolicyPlan = parseTurboJsHostTrapPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_TRAP_POLICY, 'turbojs-host-trap-policy.vqtp'));
  const turboJsExecutionLifecyclePlan = parseTurboJsExecutionLifecycleMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EXECUTION_LIFECYCLE, 'turbojs-execution-lifecycle.vqel'));
  const turboJsScriptBufferPolicyPlan = parseTurboJsScriptBufferPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_SCRIPT_BUFFER_POLICY, 'turbojs-script-buffer-policy.vqsp'));
  const turboJsContextLimitPolicyPlan = parseTurboJsContextLimitPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CONTEXT_LIMIT_POLICY, 'turbojs-context-limit-policy.vqlp'));
  const turboJsHostCallDispatchPlan = parseTurboJsHostCallDispatchMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_CALL_DISPATCH, 'turbojs-host-call-dispatch.vqhd'));
  const turboJsParityContractPlan = parseTurboJsParityContractMetadata(findOptionalSection(pkg, SECTION.TURBOJS_PARITY_CONTRACT, 'turbojs-parity-contract.vqpc'));
  const turboJsReleaseFailClosedPlan = parseTurboJsReleaseFailClosedMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RELEASE_FAILCLOSED, 'turbojs-release-failclosed.vqfc'));
  const turboJsModuleGraphPlan = parseTurboJsModuleGraphMetadata(findOptionalSection(pkg, SECTION.TURBOJS_MODULE_GRAPH, 'turbojs-module-graph.vqmg'));
  const turboJsModuleExecutionPlan = parseTurboJsModuleExecutionMetadata(findOptionalSection(pkg, SECTION.TURBOJS_MODULE_EXECUTION, 'turbojs-module-execution.vqmx'));
  const turboJsModuleCachePlan = parseTurboJsModuleCacheMetadata(findOptionalSection(pkg, SECTION.TURBOJS_MODULE_CACHE, 'turbojs-module-cache.vqmc'));
  const turboJsResolverAuditPlan = parseTurboJsResolverAuditMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RESOLVER_AUDIT, 'turbojs-resolver-audit.vqra'));
  const turboJsInteropFallbackPlan = parseTurboJsInteropFallbackMetadata(findOptionalSection(pkg, SECTION.TURBOJS_INTEROP_FALLBACK, 'turbojs-interop-fallback.vqif'));
  const turboJsExecutionPipelinePlan = parseTurboJsExecutionPipelineMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EXECUTION_PIPELINE, 'turbojs-execution-pipeline.vqxp'));
  const turboJsScriptRecordsPlan = parseTurboJsScriptRecordsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_SCRIPT_RECORDS, 'turbojs-script-records.vqsr'));
  const turboJsEvalResultsPlan = parseTurboJsEvalResultsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EVAL_RESULTS, 'turbojs-eval-results.vqev'));
  const turboJsConsoleCapturePlan = parseTurboJsConsoleCaptureMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CONSOLE_CAPTURE, 'turbojs-console-capture.vqcc'));
  const turboJsFailureReportsPlan = parseTurboJsFailureReportsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_FAILURE_REPORTS, 'turbojs-failure-reports.vqfr'));
  const turboJsExecutionJournalPlan = parseTurboJsExecutionJournalMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EXECUTION_JOURNAL, 'turbojs-execution-journal.vqxj'));
  const turboJsCheckpointPolicyPlan = parseTurboJsCheckpointPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CHECKPOINT_POLICY, 'turbojs-checkpoint-policy.vqcp'));
  const turboJsReplayCursorPlan = parseTurboJsReplayCursorMetadata(findOptionalSection(pkg, SECTION.TURBOJS_REPLAY_CURSOR, 'turbojs-replay-cursor.vqrc'));
  const turboJsResumeStatePlan = parseTurboJsResumeStateMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RESUME_STATE, 'turbojs-resume-state.vqrs'));
  const turboJsDeterminismAuditPlan = parseTurboJsDeterminismAuditMetadata(findOptionalSection(pkg, SECTION.TURBOJS_DETERMINISM_AUDIT, 'turbojs-determinism-audit.vqda'));
  const turboJsSnapshotPolicyPlan = parseTurboJsSnapshotPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_SNAPSHOT_POLICY, 'turbojs-snapshot-policy.vqsk'));
  const turboJsSnapshotRecordsPlan = parseTurboJsSnapshotRecordsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_SNAPSHOT_RECORDS, 'turbojs-snapshot-records.vqsn'));
  const turboJsReplayValidationPlan = parseTurboJsReplayValidationMetadata(findOptionalSection(pkg, SECTION.TURBOJS_REPLAY_VALIDATION, 'turbojs-replay-validation.vqrv'));
  const turboJsDeterminismLedgerPlan = parseTurboJsDeterminismLedgerMetadata(findOptionalSection(pkg, SECTION.TURBOJS_DETERMINISM_LEDGER, 'turbojs-determinism-ledger.vqdl'));
  const turboJsAuditSealPlan = parseTurboJsAuditSealMetadata(findOptionalSection(pkg, SECTION.TURBOJS_AUDIT_SEAL, 'turbojs-audit-seal.vqas'));
  const turboJsExecutionCommitPlan = parseTurboJsExecutionCommitMetadata(findOptionalSection(pkg, SECTION.TURBOJS_EXECUTION_COMMIT, 'turbojs-execution-commit.vqxc'));
  const turboJsRollbackPolicyPlan = parseTurboJsRollbackPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_ROLLBACK_POLICY, 'turbojs-rollback-policy.vqrp'));
  const turboJsHostCallReceiptsPlan = parseTurboJsHostCallReceiptsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_CALL_RECEIPTS, 'turbojs-host-call-receipts.vqhr'));
  const turboJsReleaseAcceptancePlan = parseTurboJsReleaseAcceptanceMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RELEASE_ACCEPTANCE, 'turbojs-release-acceptance.vqac'));
  const turboJsCommitAuditPlan = parseTurboJsCommitAuditMetadata(findOptionalSection(pkg, SECTION.TURBOJS_COMMIT_AUDIT, 'turbojs-commit-audit.vqca'));
  const turboJsCapabilityPolicyPlan = parseTurboJsCapabilityPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CAPABILITY_POLICY, 'turbojs-capability-policy.vqcpol'));
  const turboJsHostIoPolicyPlan = parseTurboJsHostIoPolicyMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_IO_POLICY, 'turbojs-host-io-policy.vqio'));
  const turboJsPermissionSealPlan = parseTurboJsPermissionSealMetadata(findOptionalSection(pkg, SECTION.TURBOJS_PERMISSION_SEAL, 'turbojs-permission-seal.vqps'));
  const turboJsPolicyReceiptsPlan = parseTurboJsPolicyReceiptsMetadata(findOptionalSection(pkg, SECTION.TURBOJS_POLICY_RECEIPTS, 'turbojs-policy-receipts.vqpr'));
  const turboJsReleaseGatePlan = parseTurboJsReleaseGateMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RELEASE_GATE, 'turbojs-release-gate.vqrg')); 
  const turboJsHostIoDecisionPlan = parseTurboJsHostIoDecisionMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_IO_DECISION, 'turbojs-host-io-decision.vqid'));
  const turboJsHostIoDenyTracePlan = parseTurboJsHostIoDenyTraceMetadata(findOptionalSection(pkg, SECTION.TURBOJS_HOST_IO_DENY_TRACE, 'turbojs-host-io-deny-trace.vqdt'));
  const turboJsCapabilityLedgerPlan = parseTurboJsCapabilityLedgerMetadata(findOptionalSection(pkg, SECTION.TURBOJS_CAPABILITY_LEDGER, 'turbojs-capability-ledger.vqclg'));
  const turboJsPolicySealAuditPlan = parseTurboJsPolicySealAuditMetadata(findOptionalSection(pkg, SECTION.TURBOJS_POLICY_SEAL_AUDIT, 'turbojs-policy-seal-audit.vqpsa'));
  const turboJsRuntimeDenylistPlan = parseTurboJsRuntimeDenylistMetadata(findOptionalSection(pkg, SECTION.TURBOJS_RUNTIME_DENYLIST, 'turbojs-runtime-denylist.vqrd'));
  const asyncQueuePlan = parseAsyncHostQueueMetadata(findOptionalSection(pkg, SECTION.ASYNC_HOST_QUEUE, 'async-host-queue.vahq'));
  const assetBaseUrl = options.assetBaseUrl || new URL('./', options.packageUrl || document.baseURI).href;
  const turboJsEngineModuleUrl = options.turboJsEngineUrl || (turboJsEngineModulePlan && turboJsEngineModulePlan.assetName ? new URL(turboJsEngineModulePlan.assetName, assetBaseUrl).href : null);
  const turboJsWasmUrl = options.turboJsWasmUrl || (turboJsWasmRuntimePlan && turboJsWasmRuntimePlan.assetName ? new URL(turboJsWasmRuntimePlan.assetName, assetBaseUrl).href : null);
  const route = selectRoute(routes, location.pathname);
  finishPhase('package-policy', { route: route.route, runtimeAbi: pkg.runtimeAbi, failClosed: runtimePolicy.failClosed });

  startPhase('runtime-install');
  installVenomHostBridge(root, pkg, routes, assetManifest, runtimePolicy, hostBridgePlan, fetchBridgePlan, asyncQueuePlan, timerBridgePlan, eventQueuePlan, turboJsBridgePlan, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsEngineModulePlan, turboJsEngineModuleUrl, turboJsContextLifecyclePlan, hostCapabilitiesPlan, turboJsAdapterDiagnosticsPlan, turboJsWasmRuntimePlan, turboJsWasmUrl, turboJsSourceTransferPlan, turboJsConsoleBridgePlan, turboJsExecutionRecordsPlan, turboJsResultBridgePlan, turboJsFallbackPolicyPlan, turboJsRuntimeAbiPlan, turboJsHostImportsPlan, turboJsHeapLimitsPlan, turboJsScriptBufferPlan, turboJsConsoleAbiPlan, turboJsParityProbePlan, turboJsReleaseFallbackPlan, turboJsBytecodeManifestPlan, turboJsModuleResolverPlan, turboJsExceptionAbiPlan, turboJsHostTrapPolicyPlan, turboJsExecutionLifecyclePlan, turboJsScriptBufferPolicyPlan, turboJsContextLimitPolicyPlan, turboJsHostCallDispatchPlan, turboJsParityContractPlan, turboJsReleaseFailClosedPlan, turboJsModuleGraphPlan, turboJsModuleExecutionPlan, turboJsModuleCachePlan, turboJsResolverAuditPlan, turboJsInteropFallbackPlan, turboJsExecutionJournalPlan, turboJsCheckpointPolicyPlan, turboJsReplayCursorPlan, turboJsResumeStatePlan, turboJsDeterminismAuditPlan, turboJsSnapshotPolicyPlan, turboJsSnapshotRecordsPlan, turboJsReplayValidationPlan, turboJsDeterminismLedgerPlan, turboJsAuditSealPlan, turboJsExecutionCommitPlan, turboJsRollbackPolicyPlan, turboJsHostCallReceiptsPlan, turboJsReleaseAcceptancePlan, turboJsCommitAuditPlan, turboJsCapabilityPolicyPlan, turboJsHostIoPolicyPlan, turboJsPermissionSealPlan, turboJsPolicyReceiptsPlan, turboJsReleaseGatePlan, turboJsHostIoDecisionPlan, turboJsHostIoDenyTracePlan, turboJsCapabilityLedgerPlan, turboJsPolicySealAuditPlan, turboJsRuntimeDenylistPlan);
  finishPhase('runtime-install', { protectedRuntime: turboJsWasmRuntimePlan.enabled, hostCapabilities: hostCapabilitiesPlan.capabilities.length });
  installBrowserAssetResolver(route, assetManifest, assetBaseUrl);
  startPhase('route-decode');
  const initialRuntime = routeRuntimeLoader(route);
  finishPhase('route-decode', { lazy: initialRuntime.lazy, modules: initialRuntime.jsBundle.chunks.length });
  const initialBridge = globalThis.__venomRuntime || null;
  if (initialBridge && typeof initialBridge.activateRouteGeneration === 'function') initialBridge.activateRouteGeneration(initialRuntime.routeGeneration);
  startPhase('route-render');
  executeRoute(initialRuntime.route, initialRuntime.vm, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan);
  finishPhase('route-render', { route: initialRuntime.route.route });
  startPhase('script-execution');
  const executedScripts = await executeScriptsForRoute(initialRuntime.route, initialRuntime.jsBundle, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan);
  finishPhase('script-execution', { executedScripts: executedScripts.length });
  startPhase('navigation-install');
  installNavigation(routes, routeRuntimeLoader, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan);
  finishPhase('navigation-install', { routes: routes.length });

  return {
    packageVersion: pkg.version,
    packageFeatures,
    packageBindingVerified: packageBinding.verified,
    packageBindingAssets: packageBinding.assets || 0,
    runtimeAbi: pkg.runtimeAbi,
    sections: pkg.sections.length,
    assets: assetManifest.size,
    scripts: initialRuntime.jsBundle.chunks.length,
    executedScripts: executedScripts.length,
    route: initialRuntime.route.route,
    lazySections: lazySectionsPlan.enabled,
    lazyRouteSections: lazySectionsPlan.routeCount,
    lazyScriptSections: lazySectionsPlan.scriptRouteCount,
    initialRouteLazy: initialRuntime.lazy,
    decodedSectionsAtBoot: pkg.decodedSectionCount,
    runtimeHardened: runtimePolicy.runtimeHardened,
    failClosed: runtimePolicy.failClosed,
    runtimeMode: 'js',
    hostBridgeVersion: hostBridgePlan.version,
    hostCallCount: hostBridgePlan.hostCalls.size,
    fetchBridgeVersion: fetchBridgePlan.version,
    fetchBridgeMode: fetchBridgePlan.enabled ? 'async-host-call' : 'none',
    asyncHostQueueVersion: asyncQueuePlan.version,
    asyncHostQueueMode: asyncQueuePlan.enabled ? 'enabled' : 'none',
    timerBridgeVersion: timerBridgePlan.version,
    timerBridgeMode: timerBridgePlan.enabled ? 'async-host-call' : 'none',
    eventQueueVersion: eventQueuePlan.version,
    eventQueueMode: eventQueuePlan.enabled ? 'enabled' : 'none',
    turboJsBridgeVersion: turboJsBridgePlan.version,
    turboJsBridgeMode: turboJsBridgePlan.mode,
    scriptIsolationVersion: scriptIsolationPlan.version,
    scriptIsolationMode: scriptIsolationPlan.mode,
    scriptPolicyVersion: scriptPolicyPlan.version,
    scriptPolicyMode: 'capability-metadata',
    turboJsChunkVersion: turboJsChunkPlan.version,
    turboJsChunkMode: turboJsChunkPlan.mode,
    turboJsChunkCount: turboJsChunkPlan.chunkCount,
    turboJsEngineVersion: turboJsEnginePlan.version,
    turboJsEngineMode: turboJsEnginePlan.mode,
    turboJsEngineEnabled: turboJsEnginePlan.enabled,
    scriptEnginePolicyVersion: scriptEnginePolicyPlan.version,
    scriptEngineFallback: scriptEnginePolicyPlan.fallback,
    turboJsEngineModuleVersion: turboJsEngineModulePlan.version,
    turboJsEngineModuleMode: turboJsEngineModulePlan.enabled ? turboJsEngineModulePlan.executionMode : 'none',
    turboJsEngineModuleLoaded: !!turboJsEngineModuleUrl,
    turboJsContextLifecycleVersion: turboJsContextLifecyclePlan.version,
    turboJsContextLifecycleMode: turboJsContextLifecyclePlan.enabled ? turboJsContextLifecyclePlan.model : 'none',
    hostCapabilitiesVersion: hostCapabilitiesPlan.version,
    hostCapabilityCount: hostCapabilitiesPlan.capabilities.length,
    turboJsAdapterDiagnosticsVersion: turboJsAdapterDiagnosticsPlan.version,
    turboJsAdapterDiagnosticsMode: turboJsAdapterDiagnosticsPlan.enabled ? 'enabled' : 'none',
    turboJsSnapshotPolicyVersion: turboJsSnapshotPolicyPlan.version,
    turboJsSnapshotRecordsVersion: turboJsSnapshotRecordsPlan.version,
    turboJsReplayValidationVersion: turboJsReplayValidationPlan.version,
    turboJsDeterminismLedgerVersion: turboJsDeterminismLedgerPlan.version,
    turboJsAuditSealVersion: turboJsAuditSealPlan.version,
    turboJsExecutionCommitVersion: turboJsExecutionCommitPlan.version,
    turboJsRollbackPolicyVersion: turboJsRollbackPolicyPlan.version,
    turboJsHostCallReceiptsVersion: turboJsHostCallReceiptsPlan.version,
    turboJsReleaseAcceptanceVersion: turboJsReleaseAcceptancePlan.version,
    turboJsCommitAuditVersion: turboJsCommitAuditPlan.version,
    turboJsCapabilityPolicyVersion: turboJsCapabilityPolicyPlan.version,
    turboJsHostIoPolicyVersion: turboJsHostIoPolicyPlan.version,
    turboJsPermissionSealVersion: turboJsPermissionSealPlan.version,
    turboJsPolicyReceiptsVersion: turboJsPolicyReceiptsPlan.version,
    turboJsReleaseGateVersion: turboJsReleaseGatePlan.version,
    turboJsHostIoDecisionVersion: turboJsHostIoDecisionPlan.version,
    turboJsHostIoDenyTraceVersion: turboJsHostIoDenyTracePlan.version,
    turboJsCapabilityLedgerVersion: turboJsCapabilityLedgerPlan.version,
    turboJsPolicySealAuditVersion: turboJsPolicySealAuditPlan.version,
    turboJsRuntimeDenylistVersion: turboJsRuntimeDenylistPlan.version,
    turboJsWasmRuntimeVersion: turboJsWasmRuntimePlan.version,
    turboJsWasmRuntimeMode: turboJsWasmRuntimePlan.enabled ? turboJsWasmRuntimePlan.executionMode : 'none',
    turboJsWasmRuntimeLoaded: !!turboJsWasmUrl,
    turboJsSourceTransferVersion: turboJsSourceTransferPlan.version,
    turboJsSourceTransferMode: turboJsSourceTransferPlan.enabled ? 'enabled' : 'none',
    turboJsConsoleBridgeVersion: turboJsConsoleBridgePlan.version,
    turboJsConsoleBridgeMode: turboJsConsoleBridgePlan.enabled ? turboJsConsoleBridgePlan.mode : 'none',
    turboJsExecutionRecordsVersion: turboJsExecutionRecordsPlan.version,
    turboJsExecutionRecordsMode: turboJsExecutionRecordsPlan.enabled ? turboJsExecutionRecordsPlan.retention : 'none',
    turboJsResultBridgeVersion: turboJsResultBridgePlan.version,
    turboJsResultBridgeMode: turboJsResultBridgePlan.enabled ? turboJsResultBridgePlan.format : 'none',
    turboJsFallbackPolicyVersion: turboJsFallbackPolicyPlan.version,
    turboJsFallbackPolicyMode: turboJsFallbackPolicyPlan.enabled ? turboJsFallbackPolicyPlan.mode : 'none',
    turboJsRuntimeAbi: turboJsRuntimeAbiPlan.abi,
    turboJsHostImportCount: turboJsHostImportsPlan.importCount,
    turboJsBytecodeManifestVersion: turboJsBytecodeManifestPlan.version,
    turboJsBytecodeManifestMode: turboJsBytecodeManifestPlan.enabled ? turboJsBytecodeManifestPlan.format : 'none',
    turboJsModuleResolverVersion: turboJsModuleResolverPlan.version,
    turboJsModuleResolverMode: turboJsModuleResolverPlan.enabled ? turboJsModuleResolverPlan.mode : 'none',
    turboJsExceptionAbi: turboJsExceptionAbiPlan.abi,
    turboJsExceptionAbiVersion: turboJsExceptionAbiPlan.version,
    turboJsHostTrapPolicy: turboJsHostTrapPolicyPlan.policy,
    turboJsHostTrapPolicyVersion: turboJsHostTrapPolicyPlan.version,
    turboJsExecutionLifecycleVersion: turboJsExecutionLifecyclePlan.version,
    turboJsExecutionLifecycleStates: turboJsExecutionLifecyclePlan.states,
    turboJsScriptBufferPolicyVersion: turboJsScriptBufferPolicyPlan.version,
    turboJsContextLimitPolicyVersion: turboJsContextLimitPolicyPlan.version,
    turboJsHostCallDispatchVersion: turboJsHostCallDispatchPlan.version,
    turboJsHostCallDispatchCount: turboJsHostCallDispatchPlan.entryCount,
    turboJsParityContractVersion: turboJsParityContractPlan.version,
    turboJsReleaseFailClosedVersion: turboJsReleaseFailClosedPlan.version,
    turboJsModuleGraphVersion: turboJsModuleGraphPlan.version,
    turboJsModuleGraphCount: turboJsModuleGraphPlan.moduleCount,
    turboJsModuleExecutionVersion: turboJsModuleExecutionPlan.version,
    turboJsModuleExecutionMode: turboJsModuleExecutionPlan.enabled ? turboJsModuleExecutionPlan.mode : 'none',
    turboJsModuleCacheVersion: turboJsModuleCachePlan.version,
    turboJsResolverAuditVersion: turboJsResolverAuditPlan.version,
    turboJsInteropFallbackVersion: turboJsInteropFallbackPlan.version,
    turboJsExecutionPipelineVersion: turboJsExecutionPipelinePlan.version,
    turboJsScriptRecordsVersion: turboJsScriptRecordsPlan.version,
    turboJsEvalResultsVersion: turboJsEvalResultsPlan.version,
    turboJsConsoleCaptureVersion: turboJsConsoleCapturePlan.version,
    turboJsFailureReportsVersion: turboJsFailureReportsPlan.version,
    turboJsExecutionJournalVersion: turboJsExecutionJournalPlan.version,
    turboJsCheckpointPolicyVersion: turboJsCheckpointPolicyPlan.version,
    turboJsReplayCursorVersion: turboJsReplayCursorPlan.version,
    turboJsResumeStateVersion: turboJsResumeStatePlan.version,
    turboJsDeterminismAuditVersion: turboJsDeterminismAuditPlan.version,
    routes: routes.map((item) => item.route),
  };
}
