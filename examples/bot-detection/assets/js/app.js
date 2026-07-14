// @venom: browser

(() => {
  "use strict";


const fingerprintApi = globalThis.VenomFingerprint;
if (!fingerprintApi || typeof fingerprintApi.collectFingerprint !== "function" || typeof fingerprintApi.createBehaviorTracker !== "function") {
  throw new Error("Venom fingerprint collector failed to initialize");
}

if (globalThis.__VenomSentinelAppStarted) {
  return;
}
Object.defineProperty(globalThis, "__VenomSentinelAppStarted", {
  value: true,
  configurable: false,
  enumerable: false,
  writable: false
});

const { collectFingerprint, createBehaviorTracker } = fingerprintApi;
const behaviorTracker = createBehaviorTracker();
const state = {
  fingerprint: null,
  timing: null,
  assessment: null,
  scanning: false,
  assessments: 0,
  protectedSession: null,
  nextSequence: 1,
  lastError: null
};

const elements = {
  scoreRing: document.querySelector("#score-ring"),
  scoreValue: document.querySelector("#score-value"),
  scoreLabel: document.querySelector("#score-label"),
  confidence: document.querySelector("#confidence-value"),
  automationRisk: document.querySelector("#automation-risk"),
  fingerprintId: document.querySelector("#fingerprint-id"),
  runtimeStatus: document.querySelector("#runtime-status"),
  scanStatus: document.querySelector("#scan-status"),
  categoryList: document.querySelector("#category-list"),
  findingsList: document.querySelector("#findings-list"),
  recommendation: document.querySelector("#recommendation"),
  fingerprintSections: document.querySelector("#fingerprint-sections"),
  behaviorGrid: document.querySelector("#behavior-grid"),
  rawJson: document.querySelector("#raw-json"),
  lastUpdated: document.querySelector("#last-updated"),
  signalCount: document.querySelector("#signal-count"),
  rescanButton: document.querySelector("#rescan-button"),
  copyButton: document.querySelector("#copy-button"),
  resetButton: document.querySelector("#reset-button")
};

function escapeHtml(value) {
  return String(value ?? "")
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function titleFromKey(key) {
  return String(key)
    .replace(/([a-z0-9])([A-Z])/g, "$1 $2")
    .replace(/[_-]+/g, " ")
    .replace(/^./, (letter) => letter.toUpperCase());
}

function formatBytes(value) {
  const amount = Number(value || 0);
  if (!amount) return "0 B";
  const units = ["B", "KB", "MB", "GB", "TB"];
  const index = Math.min(Math.floor(Math.log(amount) / Math.log(1024)), units.length - 1);
  return `${(amount / (1024 ** index)).toFixed(index > 1 ? 2 : 0)} ${units[index]}`;
}

function displayValue(key, value) {
  if (key.toLowerCase().includes("bytes")) return formatBytes(value);
  if (value === null || value === undefined || value === "") return "Unavailable";
  if (typeof value === "boolean") return value ? "Yes" : "No";
  if (Array.isArray(value)) return value.length ? value.map((item) => typeof item === "object" ? JSON.stringify(item) : String(item)).join(", ") : "None";
  if (typeof value === "object") return JSON.stringify(value);
  return String(value);
}

function valueClass(value) {
  if (value === true) return "positive";
  if (value === false) return "muted";
  return "";
}

function sectionCard(title, icon, data, description) {
  const rows = Object.entries(data || {}).map(([key, value]) => `
    <div class="data-row">
      <span class="data-label">${escapeHtml(titleFromKey(key))}</span>
      <span class="data-value ${valueClass(value)}" title="${escapeHtml(displayValue(key, value))}">${escapeHtml(displayValue(key, value))}</span>
    </div>
  `).join("");
  return `
    <section class="fingerprint-card">
      <div class="fingerprint-card-header">
        <span class="section-icon" aria-hidden="true">${icon}</span>
        <div>
          <h3>${escapeHtml(title)}</h3>
          <p>${escapeHtml(description)}</p>
        </div>
      </div>
      <div class="data-list">${rows || '<div class="empty-state">No values exposed by this browser.</div>'}</div>
    </section>
  `;
}

function flattenUaData(data) {
  if (!data) return { available: false };
  return {
    available: true,
    brands: data.brands,
    mobile: data.mobile,
    platform: data.platform,
    architecture: data.architecture || "",
    bitness: data.bitness || "",
    platformVersion: data.platformVersion || "",
    uaFullVersion: data.uaFullVersion || "",
    model: data.model || "",
    formFactors: data.formFactors || []
  };
}

function renderFingerprint() {
  if (!state.fingerprint) return;
  const fp = state.fingerprint;
  const sections = [
    ["Browser identity", "◎", { ...fp.browser, userAgentData: undefined }, "Navigator identity, browser product surface, privacy preferences, and plugin exposure."],
    ["User-Agent Client Hints", "CH", flattenUaData(fp.browser.userAgentData), "Structured identity hints exposed by Chromium-family browsers."],
    ["Automation surface", "⚙", fp.automation, "Signals commonly changed by WebDriver, test harnesses, browser instrumentation, or headless runtimes."],
    ["Hardware & input", "⌁", fp.hardware, "CPU, memory, touch, pointer, hover, and attached-device API capabilities."],
    ["Display", "▣", fp.display, "Screen, viewport, pixel ratio, color, orientation, and accessibility media preferences."],
    ["Graphics & media fingerprint", "◇", fp.graphics, "Canvas, audio, WebGL renderer, texture limits, and common local font availability."],
    ["Network", "⇄", fp.network, "Browser-visible connection estimates, page origin, security context, and local WebRTC ICE candidates."],
    ["Locale & timezone", "◷", fp.locale, "Language ordering, formatting system, timezone, calendar, and regional output examples."],
    ["Storage", "▤", fp.storage, "Storage APIs, estimated quota and usage, persistence, caches, and IndexedDB support."],
    ["Permissions", "✓", fp.permissions, "Current permission states queried without opening browser permission prompts."],
    ["Media devices", "◉", fp.mediaDevices, "Counts exposed by enumerateDevices; labels normally remain hidden until permission is granted."],
    ["Platform features", "＋", fp.features, "Execution, worker, crypto, GPU, service worker, and modern browser capability surface."]
  ];
  elements.fingerprintSections.innerHTML = sections.map(([title, icon, data, description]) => sectionCard(title, icon, data, description)).join("");
}

function renderBehavior(behavior, timing, session) {
  const items = {
    "Pointer moves": behavior.pointerMoves,
    "Pointer distance": `${Number(behavior.pointerDistance || 0).toLocaleString()} px`,
    "Trusted events": behavior.trustedEvents,
    "Synthetic events": behavior.syntheticEvents,
    "Clicks": behavior.clicks,
    "Key presses": behavior.keyPresses,
    "Scroll distance": `${Number(behavior.scrollDistance || 0).toLocaleString()} px`,
    "Focused time": `${(Number(behavior.focusMs || 0) / 1000).toFixed(1)} s`,
    "Idle time": `${(Number(behavior.idleMs || 0) / 1000).toFixed(1)} s`,
    "Pointer types": behavior.pointerTypes?.join(", ") || "None yet",
    "Frame mean": `${Number(timing.frameMeanMs || 0).toFixed(2)} ms`,
    "Frame deviation": `${Number(timing.frameStdDevMs || 0).toFixed(3)} ms`,
    "Long frames": timing.longFrames,
    "Session age": `${(Number(session.ageMs || 0) / 1000).toFixed(1)} s`
  };
  elements.behaviorGrid.innerHTML = Object.entries(items).map(([label, value]) => `
    <div class="behavior-stat">
      <span>${escapeHtml(label)}</span>
      <strong>${escapeHtml(value)}</strong>
    </div>
  `).join("");
}

function renderAssessment(payload, assessment) {
  const score = Number(assessment.humanScore || 1);
  const tone = String(assessment.scoreTone || "critical");
  elements.scoreRing.style.setProperty("--score", `${score * 3.6}deg`);
  elements.scoreRing.dataset.tone = tone;
  elements.scoreValue.textContent = String(score);
  elements.scoreLabel.textContent = assessment.classification;
  elements.confidence.textContent = `${assessment.confidence}%`;
  elements.automationRisk.textContent = `${assessment.automationRisk}%`;
  elements.fingerprintId.textContent = assessment.fingerprintId;
  elements.recommendation.textContent = assessment.recommendation;
  elements.lastUpdated.textContent = new Date().toLocaleTimeString();

  elements.categoryList.innerHTML = assessment.categories.map((item) => {
    const percentage = Number(item.percentage || 0);
    return `
      <div class="category-row">
        <div class="category-heading">
          <span>${escapeHtml(item.label)}</span>
          <strong>${item.score}/${item.max}</strong>
        </div>
        <div class="category-track"><span class="category-fill ${escapeHtml(item.level)}" style="width:${percentage}%"></span></div>
        <div class="category-signals">${item.signals.length ? item.signals.map((signal) => `<span>${escapeHtml(signal)}</span>`).join("") : '<span class="neutral">No positive evidence yet</span>'}</div>
      </div>
    `;
  }).join("");

  elements.findingsList.innerHTML = assessment.findings.length ? assessment.findings.map((finding) => `
    <article class="finding ${escapeHtml(finding.severity)}">
      <div class="finding-severity">${escapeHtml(finding.severity)}</div>
      <div>
        <h3>${escapeHtml(finding.title)}</h3>
        <p>${escapeHtml(finding.detail)}</p>
        <code>${escapeHtml(finding.code)}</code>
      </div>
      <strong class="finding-impact">${finding.impact}</strong>
    </article>
  `).join("") : `
    <div class="clean-state">
      <span>✓</span>
      <div><strong>No material automation markers detected</strong><p>The browser surface is coherent. Continued trusted interaction can strengthen the score.</p></div>
    </div>
  `;

  elements.signalCount.textContent = String(assessment.signalCount || 0);
  elements.rawJson.textContent = JSON.stringify({ assessment, fingerprint: state.fingerprint, behavior: payload.behavior, timing: payload.timing, session: payload.session }, null, 2);
}

async function ensureProtectedSession(runtime, force = false) {
  const now = Date.now();
  if (!force && state.protectedSession && Number(state.protectedSession.expiresAtMs || 0) > now + 5000) return state.protectedSession;
  const session = await runtime.call("beginAssessmentSession", {});
  if (!session || session.schemaVersion !== 2 || !session.sessionId || !session.nonce) throw new Error("Protected telemetry session negotiation failed");
  state.protectedSession = session;
  state.nextSequence = Number(session.nextSequence || 1);
  return session;
}

function buildTelemetryEnvelope(payload) {
  return {
    schemaVersion: 2,
    sessionId: state.protectedSession.sessionId,
    nonce: state.protectedSession.nonce,
    sequence: state.nextSequence,
    capturedAtMs: Date.now(),
    telemetry: payload
  };
}

function buildPayload() {
  const behavior = behaviorTracker.snapshot();
  const timing = state.fingerprint?.timing || state.timing || {};
  const session = {
    ageMs: Math.round(performance.now() - behaviorTracker.startedAt),
    assessments: state.assessments,
    documentVisibility: document.visibilityState,
    hasFocus: document.hasFocus(),
    historyLength: history.length,
    referrerPresent: Boolean(document.referrer),
    userActivationActive: Boolean(navigator.userActivation?.isActive),
    userActivationOccurred: Boolean(navigator.userActivation?.hasBeenActive)
  };
  return { ...state.fingerprint, behavior, timing, session };
}

async function assessCurrent() {
  if (!state.fingerprint || state.scanning) return;
  const payload = buildPayload();
  try {
    const runtime = globalThis.venom;
    if (!runtime || typeof runtime.call !== "function") {
      throw new Error("Venom protected bridge is unavailable");
    }
    await runtime.ready();
    await ensureProtectedSession(runtime);
    let envelope = buildTelemetryEnvelope(payload);
    let assessment;
    try {
      assessment = await runtime.call("assessClient", envelope);
    } catch (error) {
      const message = String(error?.message || error);
      if (!/unknown-session|expired-session|replayed-or-out-of-order|nonce-mismatch|stale-telemetry/.test(message)) throw error;
      await ensureProtectedSession(runtime, true);
      envelope = buildTelemetryEnvelope(payload);
      assessment = await runtime.call("assessClient", envelope);
    }
    state.nextSequence += 1;
    state.assessment = assessment;
    state.assessments += 1;
    elements.runtimeStatus.textContent = "Protected QuickJS/WASM engine ready";
    elements.runtimeStatus.className = "status-pill ready";
    renderAssessment(payload, assessment);
    renderBehavior(payload.behavior, payload.timing, payload.session);
  } catch (error) {
    state.lastError = error;
    elements.runtimeStatus.textContent = "Protected engine failed closed";
    elements.runtimeStatus.className = "status-pill error";
    elements.scanStatus.textContent = `Assessment error: ${error?.message || error}`;
    throw error;
  }
}

async function scan() {
  if (state.scanning) return;
  state.scanning = true;
  elements.rescanButton.disabled = true;
  elements.scanStatus.textContent = "Collecting browser, graphics, network, storage, and timing signals…";
  elements.runtimeStatus.textContent = "Starting protected runtime…";
  elements.runtimeStatus.className = "status-pill loading";
  try {
    await globalThis.venom?.ready?.();
    state.fingerprint = await collectFingerprint();
    state.timing = state.fingerprint.timing;
    delete state.fingerprint.timing;
    renderFingerprint();
    elements.scanStatus.textContent = "Passive fingerprint complete. Interact with the page to add behavioral evidence.";
    await assessCurrent();
  } catch (error) {
    state.lastError = error;
    elements.scanStatus.textContent = `Scan failed: ${error?.message || error}`;
    elements.runtimeStatus.textContent = "Protected runtime unavailable";
    elements.runtimeStatus.className = "status-pill error";
    console.error("[bot-detection] scan failed", error);
  } finally {
    state.scanning = false;
    elements.rescanButton.disabled = false;
  }
}

async function copyReport() {
  const value = elements.rawJson.textContent || "";
  if (!value) return;
  try {
    await navigator.clipboard.writeText(value);
    const original = elements.copyButton.textContent;
    elements.copyButton.textContent = "Copied report";
    setTimeout(() => { elements.copyButton.textContent = original; }, 1400);
  } catch {
    const selection = window.getSelection();
    const range = document.createRange();
    range.selectNodeContents(elements.rawJson);
    selection.removeAllRanges();
    selection.addRange(range);
  }
}

elements.rescanButton.addEventListener("click", () => void scan());
elements.copyButton.addEventListener("click", () => void copyReport());
elements.resetButton.addEventListener("click", () => {
  setTimeout(() => {
    behaviorTracker.reset();
    elements.scanStatus.textContent = "Behavior evidence reset. Passive fingerprint retained.";
    void assessCurrent();
  }, 0);
});

let assessmentTimer = null;
function scheduleBehaviorAssessment() {
  clearTimeout(assessmentTimer);
  assessmentTimer = setTimeout(() => void assessCurrent(), 350);
}
["pointerdown", "click", "keydown", "scroll"].forEach((eventName) => window.addEventListener(eventName, scheduleBehaviorAssessment, { passive: true }));
setInterval(() => void assessCurrent(), 2500);

void scan();

})();
