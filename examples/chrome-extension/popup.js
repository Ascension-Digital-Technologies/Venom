// @venom: browser
import { matchAllowedSite, parseAllowedSites } from './lib/allowlist.js';

const $ = (selector) => document.querySelector(selector);
const modes = globalThis.VelocityMatchModes;
const CONTROLLER_KEY = '__VELOCITY_CHESS_CONTROLLER_V0110__';
const CHANNEL_KEY = '__VELOCITY_CHESS_EXTENSION_CHANNEL_V0110__';

function isMissingReceiverError(error) {
  const message = error instanceof Error ? error.message : String(error || '');
  return /Could not establish connection|Receiving end does not exist/i.test(message);
}

async function sendBackgroundMessage(message, attempts = 5) {
  let lastError;
  for (let attempt = 0; attempt < attempts; attempt += 1) {
    try {
      return await chrome.runtime.sendMessage(message);
    } catch (error) {
      lastError = error;
      if (!isMissingReceiverError(error) || attempt + 1 >= attempts) throw error;
      await new Promise((resolve) => setTimeout(resolve, 75 * (attempt + 1)));
    }
  }
  throw lastError || new Error('Velocity background service worker is unavailable');
}
let activeTab = null;
let allowed = false;
let allowlistPromise = null;

function applyMatchMode(id) {
  const preset = modes?.getPreset(id);
  if (!preset) {
    $('#mode-note').textContent = 'Uses the advanced values below.';
    return;
  }
  $('#depth').value = String(preset.maxDepth);
  $('#move-time').value = String(preset.moveTimeMs);
  $('#min-delay').value = String(preset.minDelayMs);
  $('#max-delay').value = String(preset.maxDelayMs);
  $('#drag-min').value = String(preset.dragMinMs);
  $('#drag-max').value = String(preset.dragMaxMs);
  $('#mode-note').textContent = preset.description;
}

function settings() {
  return {
    matchMode: $('#match-mode').value,
    side: $('#side').value,
    skillLevel: $('#skill-level').value,
    playStyle: $('#play-style').value,
    humanBehavior: $('#human-behavior').value,
    maxDepth: Number($('#depth').value),
    moveTimeMs: Number($('#move-time').value),
    minDelayMs: Number($('#min-delay').value),
    maxDelayMs: Number($('#max-delay').value),
    dragMinMs: Number($('#drag-min').value),
    dragMaxMs: Number($('#drag-max').value),
    humanize: $('#humanize').checked,
    varyMoveTiming: $('#vary-timing').checked
  };
}


function setText(id, value) {
  const element = $(id);
  if (element) element.textContent = value;
}

function drawMousePreview(profile) {
  const canvas = $('#mouse-preview');
  if (!canvas) return;
  const context = canvas.getContext('2d');
  context.clearRect(0, 0, canvas.width, canvas.height);
  context.fillStyle = '#081421';
  context.fillRect(0, 0, canvas.width, canvas.height);
  const templates = Array.isArray(profile?.templates) ? profile.templates.filter((item) => Array.isArray(item?.points) && item.points.length >= 4) : [];
  context.strokeStyle = '#20344b';
  context.lineWidth = 1;
  context.beginPath();
  context.moveTo(20, canvas.height / 2);
  context.lineTo(canvas.width - 20, canvas.height / 2);
  context.stroke();
  if (!templates.length) {
    context.fillStyle = '#7890aa';
    context.font = '12px system-ui';
    context.fillText('No saved mouse profile', 92, 79);
    return;
  }
  const chosen = templates.slice(-12);
  for (const template of chosen) {
    context.strokeStyle = 'rgba(56, 189, 248, 0.20)';
    context.lineWidth = 1;
    context.beginPath();
    template.points.forEach((point, index) => {
      const x = 20 + Number(point.p || 0) * (canvas.width - 40);
      const y = canvas.height / 2 - Number(point.n || 0) * (canvas.height - 34);
      if (!index) context.moveTo(x, y); else context.lineTo(x, y);
    });
    context.stroke();
  }
  context.strokeStyle = '#6ee7b7';
  context.lineWidth = 2.4;
  context.beginPath();
  const steps = 42;
  for (let index = 0; index < steps; index++) {
    const t = index / (steps - 1);
    let sumP = 0, sumN = 0, count = 0;
    for (const template of chosen) {
      const points = template.points;
      let right = points.findIndex((point) => Number(point.t) >= t);
      if (right < 0) right = points.length - 1;
      const left = Math.max(0, right - 1);
      const a = points[left], b = points[right];
      const span = Math.max(0.0001, Number(b.t) - Number(a.t));
      const mix = Math.max(0, Math.min(1, (t - Number(a.t)) / span));
      sumP += Number(a.p) + (Number(b.p) - Number(a.p)) * mix;
      sumN += Number(a.n) + (Number(b.n) - Number(a.n)) * mix;
      count++;
    }
    const x = 20 + (sumP / count) * (canvas.width - 40);
    const y = canvas.height / 2 - (sumN / count) * (canvas.height - 34);
    if (!index) context.moveTo(x, y); else context.lineTo(x, y);
  }
  context.stroke();
}

let lastDiagnostics = null;
async function refreshDiagnostics() {
  if (!activeTab?.id || !allowed) return;
  try {
    let data;
    try {
      data = await invokeController(activeTab.id, 'diagnostics');
    } catch (_) {
      await ensureRecorder(activeTab.id, activeTab.url);
      data = await invokeController(activeTab.id, 'diagnostics');
    }
    lastDiagnostics = data;
    const quality = data?.recorder?.quality || data?.mouseProfileQuality || { score: 0, label: 'None' };
    setText('#diag-engine', data?.running ? 'Running' : 'Stopped');
    setText('#diag-board', data?.lastStateReadAt ? `${Math.max(0, Math.round((Date.now() - data.lastStateReadAt) / 1000))}s ago` : 'Not read');
    setText('#diag-recovery', String(data?.totalRecoveries || 0));
    setText('#diag-profile', `${quality.label || 'None'} · ${quality.score || 0}%`);
    setText('#diag-confidence', quality.score ? `${quality.score}% learned` : 'Procedural fallback');
    setText('#diag-move', data?.lastMove?.move || '—');
    const samples = Number(data?.recorder?.savedSamples || 0);
    const advice = quality.score >= 82 ? 'Excellent coverage. Learned paths should be used for most movement distances.'
      : quality.score >= 60 ? 'Good profile. Add a few very short and very long drags for stronger coverage.'
      : samples ? 'Profile is usable but limited. Record more varied distances and directions.'
      : 'Record several short, medium, and long drags to build a reliable profile.';
    setText('#profile-advice', advice);
    drawMousePreview(data?.recorder?.profile || null);
  } catch (_) {
    setText('#diag-engine', 'Unavailable');
  }
}

function show(message, error = false) {
  $('#result').textContent = message;
  $('#result').style.color = error ? '#fda4af' : '#a7f3d0';
}

async function getActiveTab() {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  return tab || null;
}

async function loadAllowlist() {
  if (!allowlistPromise) {
    allowlistPromise = fetch(chrome.runtime.getURL('allowed_sites.txt'))
      .then((response) => {
        if (!response.ok) throw new Error(`Unable to read allowed_sites.txt (${response.status})`);
        return response.text();
      })
      .then(parseAllowedSites)
      .catch((error) => {
        allowlistPromise = null;
        throw error;
      });
  }
  return allowlistPromise;
}

function randomChannel() {
  const bytes = crypto.getRandomValues(new Uint8Array(16));
  return Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');
}

async function isAllowed(url) {
  return matchAllowedSite(url, await loadAllowlist());
}

async function invokeController(tabId, method, args = [], { missingOk = false } = {}) {
  const results = await chrome.scripting.executeScript({
    target: { tabId },
    world: 'ISOLATED',
    func: (key, requestedMethod, requestedArgs, tolerateMissing) => {
      const controller = globalThis[key];
      if (!controller || typeof controller[requestedMethod] !== 'function') {
        if (tolerateMissing) return { ok: true, running: false, status: 'Not started' };
        throw new Error('Velocity page controller was not installed');
      }
      return controller[requestedMethod](...(Array.isArray(requestedArgs) ? requestedArgs : []));
    },
    args: [CONTROLLER_KEY, method, args, missingOk]
  });
  const value = results?.[0]?.result;
  return value && typeof value === 'object' ? value : { ok: true };
}

async function injectPlayer(tabId, url, inputSettings) {
  if (!Number.isInteger(tabId)) throw new Error('No active browser tab was found');
  if (!(await isAllowed(url))) throw new Error('This page is not listed in allowed_sites.txt');
  const response = await sendBackgroundMessage({
    type: 'VELOCITY_START_ACTIVE_TAB',
    tabId,
    url,
    settings: inputSettings || {}
  });
  if (!response || response.ok !== true) {
    throw new Error(response?.error || 'The protected chess engine did not start');
  }
  return response;
}

async function refresh() {
  activeTab = await getActiveTab();
  const status = $('#site-status');
  if (!activeTab?.id || !activeTab.url) {
    status.textContent = 'No supported active tab.';
    status.className = 'blocked';
    $('#start').disabled = true;
    return;
  }

  allowed = await isAllowed(activeTab.url);
  status.textContent = allowed ? 'This page is authorized by allowed_sites.txt.' : 'This page is not in allowed_sites.txt.';
  status.className = allowed ? 'allowed' : 'blocked';
  $('#start').disabled = !allowed;

  if (allowed) {
    try {
      const current = await sendBackgroundMessage({ type: 'VELOCITY_STATUS_TAB', tabId: activeTab.id });
      if (current?.ok) show(current.status, Boolean(current.error));
      await refreshDiagnostics();
    } catch (_) {
      show('Not started');
    }
  }
}

$('#match-mode').addEventListener('change', () => applyMatchMode($('#match-mode').value));
for (const id of ['depth', 'move-time', 'min-delay', 'max-delay', 'drag-min', 'drag-max']) {
  $(`#${id}`).addEventListener('input', () => {
    if ($('#match-mode').value !== 'custom') {
      $('#match-mode').value = 'custom';
      $('#mode-note').textContent = 'Uses the advanced values below.';
    }
  });
}

$('#start').addEventListener('click', async () => {
  if (!activeTab?.id || !allowed) return;
  try {
    show('Injecting the local engine…');
    const response = await injectPlayer(activeTab.id, activeTab.url, settings());
    const sideText = $('#side').value === 'auto'
      ? 'Auto-detecting your side'
      : `Playing as ${$('#side').selectedOptions[0].textContent.split('·')[0].trim()}`;
    const skillText = $('#skill-level').selectedOptions[0].textContent.split('·')[0].trim();
    show(response?.ok ? `${sideText} · ${skillText} · ${$('#match-mode').value}.` : response?.error || 'Unable to start.', !response?.ok);
  } catch (error) {
    show(error instanceof Error ? error.message : String(error), true);
  }
});

$('#stop').addEventListener('click', async () => {
  if (!activeTab?.id) return;
  try {
    const response = await sendBackgroundMessage({ type: 'VELOCITY_STOP_TAB', tabId: activeTab.id });
    show(response?.ok ? 'Stopped.' : response?.error || 'Unable to stop.', !response?.ok);
  } catch (error) {
    show(error instanceof Error ? error.message : String(error), true);
  }
});


async function ensureRecorder(tabId, url) {
  if (!Number.isInteger(tabId) || !(await isAllowed(url))) throw new Error('Mouse recording is only available on authorized pages');
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['mouse-recorder.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['humanize.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['skill-levels.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['content-script.js'] });
}

$('#record-mouse').addEventListener('click', async () => {
  if (!activeTab?.id || !allowed) return;
  try {
    await ensureRecorder(activeTab.id, activeTab.url);
    const response = await invokeController(activeTab.id, 'startMouseRecording');
    show(response?.ok ? 'Recording real mouse strokes—make at least 3 natural drags, then press Save recording.' : response?.error || 'Unable to record.', !response?.ok);
  } catch (error) { show(error instanceof Error ? error.message : String(error), true); }
});

$('#save-mouse').addEventListener('click', async () => {
  if (!activeTab?.id || !allowed) return;
  try {
    const response = await invokeController(activeTab.id, 'stopMouseRecording');
    show(response?.ok ? `Saved ${response.sampleCount} normalized mouse strokes.` : `Need at least 3 usable drags; captured ${response?.sampleCount || 0}.`, !response?.ok);
    await refreshDiagnostics();
  } catch (error) { show(error instanceof Error ? error.message : String(error), true); }
});

$('#clear-mouse').addEventListener('click', async () => {
  if (!activeTab?.id || !allowed) return;
  try {
    await ensureRecorder(activeTab.id, activeTab.url);
    const response = await invokeController(activeTab.id, 'clearMouseRecording');
    show(response?.ok ? 'Recorded mouse profile cleared.' : response?.error || 'Unable to clear profile.', !response?.ok);
    await refreshDiagnostics();
  } catch (error) { show(error instanceof Error ? error.message : String(error), true); }
});

$('#refresh-diagnostics').addEventListener('click', () => void refreshDiagnostics());
$('#copy-diagnostics').addEventListener('click', async () => {
  if (!lastDiagnostics) await refreshDiagnostics();
  const safe = {
    running: lastDiagnostics?.running, status: lastDiagnostics?.status, error: lastDiagnostics?.error,
    recoveries: lastDiagnostics?.totalRecoveries, proactiveRecoveries: lastDiagnostics?.proactiveRecoveries,
    lastStateReadAt: lastDiagnostics?.lastStateReadAt, lastPositionChangeAt: lastDiagnostics?.lastPositionChangeAt,
    recorder: { savedSamples: lastDiagnostics?.recorder?.savedSamples, quality: lastDiagnostics?.recorder?.quality },
    lastMove: lastDiagnostics?.lastMove
  };
  await navigator.clipboard.writeText(JSON.stringify(safe, null, 2));
  show('Diagnostics copied.');
});

applyMatchMode('5m');
void refresh().catch((error) => show(error instanceof Error ? error.message : String(error), true));
