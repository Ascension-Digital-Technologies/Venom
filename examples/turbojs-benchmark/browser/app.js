// @venom: browser
import { benchmarkIdentity, runBenchmarkCase } from "../protected/benchmark.js";

const CASES = [
  { id: "arithmetic", label: "Floating-point arithmetic", base: 900000 },
  { id: "bitwise", label: "Integer and bitwise", base: 1400000 },
  { id: "control", label: "Branches and control flow", base: 1200000 },
  { id: "calls", label: "Function calls", base: 900000 },
  { id: "arrays", label: "JavaScript arrays", base: 900000 },
  { id: "typedArrays", label: "Typed arrays", base: 1100000 },
  { id: "objects", label: "Objects and properties", base: 750000 },
  { id: "strings", label: "Strings and RegExp", base: 130000 },
  { id: "json", label: "JSON encode/decode", base: 100000 },
  { id: "mapsSets", label: "Map and Set", base: 500000 },
  { id: "math", label: "Math built-ins", base: 180000 },
  { id: "sorting", label: "Array sorting", base: 100000 },
  { id: "allocation", label: "Allocation and GC pressure", base: 180000 }
];

const PRESETS = {
  quick: { scale: 0.35, label: "Quick", detail: "Fast health check" },
  standard: { scale: 1, label: "Standard", detail: "Balanced engine profile" },
  endurance: { scale: 3, label: "Endurance", detail: "Long-running sustained workload" }
};

const startButton = document.querySelector("#start-benchmark");
const stopButton = document.querySelector("#stop-benchmark");
const presetSelect = document.querySelector("#preset");
const progressBar = document.querySelector("#progress-bar");
const progressText = document.querySelector("#progress-text");
const statusBadge = document.querySelector("#runtime-status");
const totalTimeNode = document.querySelector("#total-time");
const totalOpsNode = document.querySelector("#total-ops");
const scoreNode = document.querySelector("#engine-score");
const fastestNode = document.querySelector("#fastest-case");
const resultBody = document.querySelector("#results-body");
const logNode = document.querySelector("#run-log");

let activeRun = 0;
let running = false;

function formatNumber(value, digits = 0) {
  return new Intl.NumberFormat(undefined, { maximumFractionDigits: digits }).format(value);
}

function setStatus(state, text) {
  statusBadge.dataset.status = state;
  statusBadge.textContent = text;
}

function log(message) {
  const time = new Date().toLocaleTimeString();
  logNode.textContent += `[${time}] ${message}\n`;
  logNode.scrollTop = logNode.scrollHeight;
}

function resetResults() {
  resultBody.innerHTML = "";
  logNode.textContent = "";
  totalTimeNode.textContent = "—";
  totalOpsNode.textContent = "—";
  scoreNode.textContent = "—";
  fastestNode.textContent = "—";
  progressBar.style.width = "0%";
  progressText.textContent = "Ready";
}

function appendPendingRow(test) {
  const row = document.createElement("tr");
  row.dataset.case = test.id;
  row.innerHTML = `<td><strong>${test.label}</strong><span>${test.id}</span></td><td class="status-cell">Queued</td><td>—</td><td>—</td><td>—</td>`;
  resultBody.appendChild(row);
  return row;
}

function updateRow(row, state, elapsed, iterations, checksum) {
  row.dataset.state = state;
  row.children[1].textContent = state === "pass" ? "Passed" : state === "running" ? "Running" : "Failed";
  if (elapsed != null) {
    row.children[2].textContent = `${elapsed.toFixed(2)} ms`;
    row.children[3].textContent = `${formatNumber(iterations / Math.max(elapsed, 0.001) * 1000)} ops/s`;
    row.children[4].textContent = formatNumber(checksum, 3);
  }
}

async function runSuite() {
  if (running) return;
  running = true;
  const runId = ++activeRun;
  startButton.disabled = true;
  stopButton.disabled = false;
  presetSelect.disabled = true;
  resetResults();

  const preset = PRESETS[presetSelect.value] || PRESETS.standard;
  const rows = new Map(CASES.map(test => [test.id, appendPendingRow(test)]));
  const results = [];
  let totalOperations = 0;
  const suiteStart = performance.now();

  setStatus("running", `Running ${preset.label.toLowerCase()} suite…`);
  log(`Starting ${preset.label} preset — ${preset.detail}.`);
  log("Warm-up: protected bridge and persistent TurboJS context.");

  try {
    await runBenchmarkCase({ name: "bitwise", iterations: 25000, seed: 7 });
    for (let index = 0; index < CASES.length; index++) {
      if (runId !== activeRun) throw new DOMException("Benchmark stopped", "AbortError");
      const test = CASES[index];
      const iterations = Math.max(1, Math.round(test.base * preset.scale));
      const row = rows.get(test.id);
      updateRow(row, "running");
      progressText.textContent = `${index + 1}/${CASES.length} · ${test.label}`;
      progressBar.style.width = `${index / CASES.length * 100}%`;
      log(`Running ${test.label} with ${formatNumber(iterations)} iterations…`);

      const start = performance.now();
      const output = await runBenchmarkCase({ name: test.id, iterations, seed: 1337 + index });
      const elapsed = performance.now() - start;
      totalOperations += output.operations;
      results.push({ ...test, elapsed, output });
      updateRow(row, "pass", elapsed, output.operations, output.checksum);
      progressBar.style.width = `${(index + 1) / CASES.length * 100}%`;
      log(`${test.label} completed in ${elapsed.toFixed(2)} ms.`);
      await new Promise(resolve => requestAnimationFrame(resolve));
    }

    const totalElapsed = performance.now() - suiteStart;
    const fastest = results.reduce((best, value) => !best || value.elapsed < best.elapsed ? value : best, null);
    const aggregateOps = totalOperations / Math.max(totalElapsed, 0.001) * 1000;
    const score = Math.round(aggregateOps / 1000);

    totalTimeNode.textContent = `${(totalElapsed / 1000).toFixed(2)} s`;
    totalOpsNode.textContent = formatNumber(totalOperations);
    scoreNode.textContent = formatNumber(score);
    fastestNode.textContent = fastest ? fastest.label : "—";
    progressText.textContent = `Completed ${CASES.length} engine tests`;
    setStatus("ready", "TurboJS benchmark complete");
    log(`Suite complete: ${formatNumber(totalOperations)} operations in ${(totalElapsed / 1000).toFixed(2)} seconds.`);
  } catch (error) {
    const aborted = error && error.name === "AbortError";
    setStatus("error", aborted ? "Benchmark stopped" : "Benchmark failed");
    progressText.textContent = aborted ? "Stopped by user" : "Failed";
    log(aborted ? "Benchmark stopped by user." : `Failure: ${error instanceof Error ? error.message : String(error)}`);
  } finally {
    if (runId === activeRun) {
      running = false;
      startButton.disabled = false;
      stopButton.disabled = true;
      presetSelect.disabled = false;
    }
  }
}

function stopSuite() {
  if (!running) return;
  activeRun++;
  running = false;
  startButton.disabled = false;
  stopButton.disabled = true;
  presetSelect.disabled = false;
  setStatus("error", "Stopping benchmark…");
}

async function boot() {
  await globalThis.venom?.ready?.();
  const identity = await benchmarkIdentity();
  setStatus(identity.protected ? "ready" : "error", `${identity.engine} ready · ${identity.cases} tests`);
  startButton.addEventListener("click", () => { void runSuite(); });
  stopButton.addEventListener("click", stopSuite);
  log("Protected TurboJS/WASM benchmark runtime initialized.");
}

void boot().catch(error => {
  setStatus("error", "Runtime initialization failed");
  log(error instanceof Error ? error.message : String(error));
});
