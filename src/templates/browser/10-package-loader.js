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
  const knownPackageFlags = PACKAGE_FLAG.PROTECT | PACKAGE_FLAG.RELEASE | PACKAGE_FLAG.POLYMORPHIC | PACKAGE_FLAG.COMPRESSED_SECTIONS | PACKAGE_FLAG.CRYPTO_PROVIDER_READY | PACKAGE_FLAG.INTEGRITY_METADATA | PACKAGE_FLAG.AEAD_PROVIDER_READY | PACKAGE_FLAG.RUNTIME_HARDENED | PACKAGE_FLAG.WASM_RUNTIME | PACKAGE_FLAG.HOST_BRIDGE | PACKAGE_FLAG.BINARY_DOM_OPS | PACKAGE_FLAG.FETCH_BRIDGE | PACKAGE_FLAG.ASYNC_HOST_QUEUE | PACKAGE_FLAG.TIMER_BRIDGE | PACKAGE_FLAG.EVENT_QUEUE | PACKAGE_FLAG.QUICKJS_BRIDGE | PACKAGE_FLAG.SCRIPT_ISOLATION | PACKAGE_FLAG.SCRIPT_POLICY | PACKAGE_FLAG.QUICKJS_CHUNKS | PACKAGE_FLAG.QUICKJS_ENGINE | PACKAGE_FLAG.SCRIPT_ENGINE_FALLBACK | PACKAGE_FLAG.QUICKJS_ENGINE_MODULE | PACKAGE_FLAG.QUICKJS_CONTEXT_LIFECYCLE | PACKAGE_FLAG.HOST_CAPABILITIES | PACKAGE_FLAG.QUICKJS_ADAPTER_DIAGNOSTICS | PACKAGE_FLAG.QUICKJS_WASM_RUNTIME | PACKAGE_FLAG.QUICKJS_SOURCE_TRANSFER | PACKAGE_FLAG.QUICKJS_CONSOLE_BRIDGE | PACKAGE_FLAG.QUICKJS_EXECUTION_RECORDS | PACKAGE_FLAG.QUICKJS_RESULT_BRIDGE | PACKAGE_FLAG.QUICKJS_FALLBACK_POLICY | PACKAGE_FLAG.QUICKJS_ENGINE_BACKEND;
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
let activeQuickJsAbiFingerprint = null;
let activeRuntimeIntegritySeal = 0;
let activeRuntimeOpcodeMap = null;
