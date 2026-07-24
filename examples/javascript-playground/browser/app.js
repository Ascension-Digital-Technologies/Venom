// @venom: browser
import { examples } from "./examples";

const editor = document.querySelector("#code-editor");
const output = document.querySelector("#output");
const status = document.querySelector("#status");
const runButton = document.querySelector("#run");
const clearButton = document.querySelector("#clear");
const exampleSelect = document.querySelector("#examples");
const inputEditor = document.querySelector("#input-editor");
const lineNumbers = document.querySelector("#line-numbers");
const metricDuration = document.querySelector("#metric-duration");
const metricLogs = document.querySelector("#metric-logs");
const metricResult = document.querySelector("#metric-result");
const PLAYGROUND_DEFAULT_LIMITS = Object.freeze({ sourceBytes: 262144, inputBytes: 65536, resultBytes: 262144, consoleEvents: 128, consoleBytes: 131072, executionMs: 2000, heapBytes: 8388608, stackBytes: 262144 });
let playgroundSession = null;
async function getPlaygroundSession() {
  if (playgroundSession) return playgroundSession;
  const response = await fetch("/__venom/playground/session", { cache: "no-store", credentials: "same-origin" });
  const payload = await response.json().catch(() => ({ ok: false }));
  if (!response.ok || !payload.ok || !payload.token) throw new Error(payload.error || "Playground session unavailable");
  playgroundSession = Object.freeze({ token: String(payload.token), limits: Object.freeze({ ...PLAYGROUND_DEFAULT_LIMITS, ...(payload.limits || {}) }) });
  return playgroundSession;
}

for (const [index, item] of examples.entries()) {
  const option = document.createElement("option");
  option.value = String(index);
  option.textContent = item.name;
  exampleSelect.append(option);
}

function updateLines() {
  const count = Math.max(1, editor.value.split("\n").length);
  lineNumbers.textContent = Array.from({ length: count }, (_, index) => String(index + 1)).join("\n");
}
function setExample(index) {
  const selected = examples[index] || examples[0];
  editor.value = selected.code;
  document.querySelector("#example-description").textContent = selected.description;
  updateLines();
}
function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, character => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#039;" }[character] || character));
}
function asArray(value) {
  if (Array.isArray(value)) return value;
  if (value && Array.isArray(value.events)) return value.events;
  if (value && Array.isArray(value.entries)) return value.entries;
  return [];
}
function displayValue(value) {
  if (value && typeof value === "object" && (value.__venomPlaygroundType === "undefined" || value.type === "undefined")) return "undefined";
  if (value === undefined) return "undefined";
  if (value === null) return "null";
  if (typeof value === "string") return value;
  try {
    const encoded = JSON.stringify(value, null, 2);
    return encoded === undefined ? String(value) : encoded;
  } catch { return String(value); }
}
function bridgeEnvelope(report) {
  const candidates = [
    report && report.report,
    report && report.executionRecord,
    report
  ];
  for (const candidate of candidates) {
    if (!candidate || typeof candidate !== "object") continue;
    if (Object.prototype.hasOwnProperty.call(candidate, "result") ||
        Object.prototype.hasOwnProperty.call(candidate, "error") ||
        Object.prototype.hasOwnProperty.call(candidate, "ok")) return candidate;
  }
  return null;
}
function normalizeConsole(raw) {
  const records = asArray(raw && raw.consoleCapture).concat(asArray(raw && raw.wasmConsoleEvent));
  return records.map(record => ({
    level: String(record.level || record.kind || "log").toLowerCase(),
    text: record.text || record.message || displayValue(record.values || record.args || record)
  }));
}
function renderReport(report) {
  const envelope = bridgeEnvelope(report);
  const logs = normalizeConsole(envelope || report);
  const entries = logs.map(entry => `<div class="console-line ${escapeHtml(entry.level)}"><span>${escapeHtml(entry.level.toUpperCase())}</span><code>${escapeHtml(entry.text)}</code></div>`).join("");
  const failed = !report || !report.executed || !!report.exceptionMessage || !!report.failureReport || !!(envelope && envelope.ok === false);
  let resultValue;
  if (envelope && Object.prototype.hasOwnProperty.call(envelope, "result")) resultValue = envelope.result;
  else if (report && Object.prototype.hasOwnProperty.call(report, "evalResult")) resultValue = report.evalResult;
  else if (report && Object.prototype.hasOwnProperty.call(report, "executionRecord")) resultValue = report.executionRecord;
  else resultValue = { __venomPlaygroundType: "undefined" };
  const failureValue = envelope && envelope.ok === false
    ? envelope.error
    : report && (report.failureReport || report.exceptionRecord || report.statusRecord || report);
  const failureMessage = report && report.exceptionMessage
    ? report.exceptionMessage
    : envelope && envelope.error && envelope.error.message
      ? envelope.error.message
      : "TurboJS execution failed";
  const result = failed
    ? `<div class="error-block"><strong>${escapeHtml(failureMessage)}</strong><pre>${escapeHtml(displayValue(failureValue))}</pre></div>`
    : `<div class="result-block"><span>TURBOJS RESULT</span><pre>${escapeHtml(displayValue(resultValue))}</pre></div>`;
  output.innerHTML = entries + result;
  status.textContent = failed ? "Execution failed" : "Execution complete";
  status.dataset.state = failed ? "error" : "success";
  metricLogs.textContent = String(logs.length);
  metricResult.textContent = report && report.turboJsWasm ? "WASM" : "unknown";
}

function decodeBase64(value) {
  const binary = atob(String(value || ""));
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index++) bytes[index] = binary.charCodeAt(index);
  return bytes;
}
function fnv1a32(bytes) {
  let hash = 2166136261 >>> 0;
  for (const value of bytes) {
    hash ^= value >>> 0;
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  return hash >>> 0;
}
function handoffFor(chunk, bytecodeBytes) {
  const byteHash = fnv1a32(bytecodeBytes);
  const binding = `${chunk.route}\n${chunk.source}\n${chunk.order >>> 0}\n${bytecodeBytes.length >>> 0}\n${byteHash}`;
  const bindingHash = fnv1a32(new TextEncoder().encode(binding));
  return Object.freeze({
    version: 1,
    producer: "development-turbojs-compiler",
    consumer: "turbojs-execution-wasm",
    byteLength: bytecodeBytes.length >>> 0,
    byteHash,
    bindingHash
  });
}
async function compileWithTurboJs(source) {
  const session = await getPlaygroundSession();
  if (new TextEncoder().encode(source).byteLength > session.limits.sourceBytes) throw new Error(`Source exceeds ${session.limits.sourceBytes} bytes`);
  const response = await fetch("/__venom/playground/compile", {
    method: "POST",
    credentials: "same-origin",
    headers: { "Content-Type": "application/json", "X-Venom-Playground-Token": session.token },
    body: JSON.stringify({ source })
  });
  const payload = await response.json().catch(() => ({ ok: false, error: `compiler returned HTTP ${response.status}` }));
  if (!response.ok || !payload.ok || !payload.bytecode) {
    throw new Error(payload.error || "TurboJS compilation failed");
  }
  return decodeBase64(payload.bytecode);
}
async function executeInTurboJs(source, input) {
  const session = await getPlaygroundSession();
  if (new TextEncoder().encode(JSON.stringify(input)).byteLength > session.limits.inputBytes) throw new Error(`JSON input exceeds ${session.limits.inputBytes} bytes`);
  const runtime = globalThis.__venomRuntime;
  if (!runtime || typeof runtime.executeTurboJsChunk !== "function") {
    throw new Error("Venom TurboJS runtime bridge is not ready");
  }
  if (typeof runtime.clearTurboJsConsoleEvents === "function") runtime.clearTurboJsConsoleEvents();
  const candidate = "__venomPlaygroundExecute";
  const wrapped = `globalThis.__venomProtectedBridge = globalThis.__venomProtectedBridge || Object.create(null);\n` +
    `globalThis.__venomProtectedBridge[${JSON.stringify(candidate)}] = async function (input) {\n` +
    `  const LIMITS = ${JSON.stringify({ resultBytes: session.limits.resultBytes, consoleEvents: session.limits.consoleEvents, consoleBytes: session.limits.consoleBytes })};\n` +
    `  const consoleCapture = []; let consoleBytes = 0;\n` +
    `  const normalize = (value, depth = 0, seen = []) => {\n` +
    `    if (value === undefined) return { type: "undefined" };\n` +
    `    if (value === null || typeof value === "boolean" || typeof value === "number") return value;\n` +
    `    if (typeof value === "string") return value.length > 16384 ? value.slice(0, 16384) + "…" : value;\n` +
    `    if (typeof value === "bigint") return { type: "bigint", value: String(value) };\n` +
    `    if (typeof value === "symbol" || typeof value === "function") return { type: typeof value, value: String(value) };\n` +
    `    if (value instanceof Error) return { name: value.name, message: String(value.message || "").slice(0, 16384), stack: String(value.stack || "").slice(0, 32768) };\n` +
    `    if (depth >= 8) return { type: "truncated", reason: "max-depth" };\n` +
    `    if (seen.indexOf(value) >= 0) return { type: "circular" };\n` +
    `    const nextSeen = seen.concat([value]);\n` +
    `    if (Array.isArray(value)) return value.slice(0, 1024).map(item => normalize(item, depth + 1, nextSeen));\n` +
    `    const out = {}; let count = 0; for (const key of Object.keys(value)) { if (count++ >= 1024) { out.__truncated = true; break; } out[key] = normalize(value[key], depth + 1, nextSeen); } return out;\n` +
    `  };\n` +
    `  const capture = (level, values) => { if (consoleCapture.length >= LIMITS.consoleEvents || consoleBytes >= LIMITS.consoleBytes) return; const event = { level, values: values.map(value => normalize(value)) }; const encoded = JSON.stringify(event); consoleBytes += encoded.length; if (consoleBytes <= LIMITS.consoleBytes) consoleCapture.push(event); };\n` +
    `  const console = Object.freeze({ log: (...values) => capture("log", values), info: (...values) => capture("info", values), warn: (...values) => capture("warn", values), error: (...values) => capture("error", values) });\n` +
    `  try { const result = normalize(await eval(${JSON.stringify(source)})); const encodedResult = JSON.stringify(result); if (encodedResult.length > LIMITS.resultBytes) throw new Error("TurboJS result exceeds the configured result limit"); return { ok: true, result, consoleCapture }; }\n` +
    `  catch (error) { return { ok: false, error: normalize(error), consoleCapture }; }\n` +
    `};`;
  const bytecodeBytes = await compileWithTurboJs(wrapped);
  const chunk = {
    route: "/",
    source: "playground/user-script.js",
    order: Date.now() >>> 0,
    flags: 0,
    code: "",
    bytecodeBytes,
    bridgeCandidate: candidate,
    bridgeArgs: [input],
    executionPolicy: { isolation: "ephemeral", timeoutMs: session.limits.executionMs, heapLimitBytes: session.limits.heapBytes, stackLimitBytes: session.limits.stackBytes, interruptBudget: 250000, pendingJobLimit: 128, maxResultBytes: session.limits.resultBytes, maxBridgeInputBytes: session.limits.inputBytes + 16384 }
  };
  chunk.bytecodeTrustHandoff = handoffFor(chunk, bytecodeBytes);
  return runtime.executeTurboJsChunk(chunk, { route: "/" });
}

async function run() {
  runButton.disabled = true;
  status.textContent = "Compiling inside TurboJS/WASM…";
  status.dataset.state = "running";
  output.innerHTML = `<div class="empty-state"><div class="spinner"></div><p>Creating isolated TurboJS context…</p></div>`;
  let input = {};
  try { input = inputEditor.value.trim() ? JSON.parse(inputEditor.value) : {}; }
  catch (error) {
    output.innerHTML = `<div class="error-block"><strong>Invalid JSON input</strong><pre>${escapeHtml(error)}</pre></div>`;
    status.textContent = "Input validation failed"; status.dataset.state = "error"; runButton.disabled = false; return;
  }
  const started = performance.now();
  try {
    const report = await executeInTurboJs(editor.value, input);
    metricDuration.textContent = `${Math.max(0, performance.now() - started).toFixed(1)} ms`;
    renderReport(report);
  } catch (error) {
    output.innerHTML = `<div class="error-block"><strong>TurboJS bridge failure</strong><pre>${escapeHtml(error && error.stack ? error.stack : error)}</pre></div>`;
    status.textContent = "Protected runtime unavailable"; status.dataset.state = "error";
  } finally { runButton.disabled = false; }
}

exampleSelect.addEventListener("change", () => setExample(Number(exampleSelect.value)));
editor.addEventListener("input", updateLines);
editor.addEventListener("scroll", () => { lineNumbers.scrollTop = editor.scrollTop; });
runButton.addEventListener("click", run);
clearButton.addEventListener("click", () => { output.innerHTML = `<div class="empty-state"><p>Output cleared. Run the script to create a new TurboJS execution.</p></div>`; status.textContent = "Ready"; status.dataset.state = "idle"; });
document.addEventListener("keydown", event => { if ((event.ctrlKey || event.metaKey) && event.key === "Enter") { event.preventDefault(); run(); } });
inputEditor.value = JSON.stringify({ value: 42, label: "Venom" }, null, 2);
setExample(0);
