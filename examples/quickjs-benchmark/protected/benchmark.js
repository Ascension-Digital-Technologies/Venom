// @venom: protected module

const MASK = 0xffffffff;

function normalizeIterations(value) {
  const parsed = Math.trunc(Number(value) || 0);
  return Math.max(1, Math.min(parsed, 50000000));
}

function arithmetic(iterations, seed) {
  let value = (seed || 1) + 0.5;
  for (let i = 1; i <= iterations; i++) {
    value = (value * 1.000000119 + i * 0.00003125) % 1000003.25;
    value += value / (i % 97 + 1);
  }
  return value;
}

function integerBitwise(iterations, seed) {
  let value = (seed | 0) ^ 0x9e3779b9;
  for (let i = 0; i < iterations; i++) {
    value ^= value << 13;
    value ^= value >>> 17;
    value ^= value << 5;
    value = (value + Math.imul(i + 1, 2654435761)) | 0;
  }
  return value >>> 0;
}

function controlFlow(iterations, seed) {
  let score = seed | 0;
  for (let i = 0; i < iterations; i++) {
    const lane = (i + score) & 7;
    if (lane === 0) score += i & 31;
    else if (lane === 1) score ^= i;
    else if (lane === 2) score -= i & 15;
    else if (lane === 3) score = Math.imul(score, 3) | 0;
    else if (lane === 4) score += score >>> 3;
    else if (lane === 5) score ^= score << 7;
    else if (lane === 6) score += 17;
    else score -= 9;
  }
  return score >>> 0;
}

function functionCalls(iterations, seed) {
  function mix(a, b) {
    return (Math.imul(a ^ b, 1664525) + 1013904223) | 0;
  }
  function rotate(value, amount) {
    return (value << amount) | (value >>> (32 - amount));
  }
  let value = seed | 0;
  for (let i = 0; i < iterations; i++) {
    value = rotate(mix(value, i), (i % 15) + 1);
  }
  return value >>> 0;
}

function arrays(iterations, seed) {
  const length = 2048;
  const values = new Array(length);
  for (let i = 0; i < length; i++) values[i] = (seed + i * 17) & 65535;
  let checksum = 0;
  for (let i = 0; i < iterations; i++) {
    const index = i & (length - 1);
    const next = (index + 1) & (length - 1);
    values[index] = (values[index] + values[next] + i) & 65535;
    checksum = (checksum + values[index]) | 0;
  }
  return checksum >>> 0;
}

function typedArrays(iterations, seed) {
  const values = new Uint32Array(2048);
  for (let i = 0; i < values.length; i++) values[i] = (seed + i * 2654435761) >>> 0;
  let checksum = 0;
  for (let i = 0; i < iterations; i++) {
    const index = i & 2047;
    values[index] = (Math.imul(values[index] ^ i, 2246822519) + 3266489917) >>> 0;
    checksum ^= values[index];
  }
  return checksum >>> 0;
}

function objects(iterations, seed) {
  const records = new Array(512);
  for (let i = 0; i < records.length; i++) {
    records[i] = { id: i, x: seed + i, y: i * 3, active: (i & 1) === 0 };
  }
  let checksum = 0;
  for (let i = 0; i < iterations; i++) {
    const record = records[i & 511];
    record.x = (record.x + record.y + i) & 0xfffffff;
    record.active = !record.active;
    checksum = (checksum + record.x + (record.active ? record.id : -record.id)) | 0;
  }
  return checksum >>> 0;
}

function strings(iterations, seed) {
  let checksum = seed | 0;
  const words = ["venom", "quickjs", "wasm", "runtime", "protected", "benchmark"];
  for (let i = 0; i < iterations; i++) {
    const value = `${words[i % words.length]}-${(i ^ checksum).toString(36)}`;
    const transformed = value.slice(1).toUpperCase().replace(/A/g, "_");
    checksum = (checksum + transformed.length + transformed.charCodeAt(i % transformed.length)) | 0;
  }
  return checksum >>> 0;
}

function json(iterations, seed) {
  let checksum = seed | 0;
  const batch = 32;
  for (let i = 0; i < iterations; i += batch) {
    const payload = {
      id: i,
      seed: checksum,
      flags: [true, false, (i & 1) === 0],
      values: [i, i + 1, i + 2, i + 3],
      label: `run-${i.toString(36)}`
    };
    const encoded = JSON.stringify(payload);
    const decoded = JSON.parse(encoded);
    checksum = (checksum + decoded.id + decoded.label.length + encoded.length) | 0;
  }
  return checksum >>> 0;
}

function mapsSets(iterations, seed) {
  const map = new Map();
  const set = new Set();
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) {
    const key = i & 1023;
    map.set(key, ((map.get(key) || seed) + i) >>> 0);
    set.add((key + checksum) & 2047);
    if ((i & 255) === 0) set.delete((key + 17) & 2047);
    checksum ^= map.get(key) || 0;
  }
  return (checksum + map.size + set.size) >>> 0;
}

function mathBuiltins(iterations, seed) {
  let value = (seed || 1) / 7;
  for (let i = 1; i <= iterations; i++) {
    value += Math.sin(i * 0.001) * Math.cos(value * 0.0001);
    value = Math.sqrt(Math.abs(value) + 1) * 31.4159265359;
    value += Math.log(i + 1) + Math.atan2(value, i);
  }
  return value;
}

function sorting(iterations, seed) {
  let checksum = seed | 0;
  const rounds = Math.max(1, Math.floor(iterations / 2048));
  for (let round = 0; round < rounds; round++) {
    const values = new Array(512);
    let state = (seed + round) | 0;
    for (let i = 0; i < values.length; i++) {
      state = (Math.imul(state, 1664525) + 1013904223) | 0;
      values[i] = state >>> 0;
    }
    values.sort((a, b) => a - b);
    checksum ^= values[0] ^ values[values.length - 1];
  }
  return checksum >>> 0;
}

function allocation(iterations, seed) {
  let checksum = seed | 0;
  const rounds = Math.max(1, Math.floor(iterations / 128));
  for (let round = 0; round < rounds; round++) {
    const batch = [];
    for (let i = 0; i < 128; i++) {
      batch.push({ index: i, round, data: [seed + i, round + i, i * i], text: `v${round}-${i}` });
    }
    for (let i = 0; i < batch.length; i++) checksum = (checksum + batch[i].data[2] + batch[i].text.length) | 0;
  }
  return checksum >>> 0;
}

const CASES = {
  arithmetic,
  bitwise: integerBitwise,
  control: controlFlow,
  calls: functionCalls,
  arrays,
  typedArrays,
  objects,
  strings,
  json,
  mapsSets,
  math: mathBuiltins,
  sorting,
  allocation
};

// @venom: input name:string, iterations:integer, seed?:integer
// @venom: output name:string, iterations:integer, checksum:number, operations:number, engine:string
export function runBenchmarkCase(input) {
  const name = String(input && input.name || "");
  const benchmark = CASES[name];
  if (!benchmark) throw new Error(`Unknown QuickJS benchmark case: ${name}`);
  const iterations = normalizeIterations(input.iterations);
  const seed = Math.trunc(Number(input.seed) || 1337);
  const result = benchmark(iterations, seed);
  const checksum = typeof result === "number" && Number.isFinite(result) ? result : 0;
  return {
    name,
    iterations,
    checksum: Math.round(checksum * 1000) / 1000,
    operations: iterations,
    engine: "QuickJS/WASM protected runtime"
  };
}

export function benchmarkIdentity() {
  return {
    protected: true,
    engine: "QuickJS/WASM",
    cases: Object.keys(CASES).length,
    architecture: "sequential protected subsystem benchmark"
  };
}
