import { matchAllowedSite, parseAllowedSites } from './lib/allowlist.js';

const $ = (selector) => document.querySelector(selector);
const modes = globalThis.VelocityMatchModes;
const CONTROLLER_KEY = '__VELOCITY_CHESS_CONTROLLER_V045__';
const CHANNEL_KEY = '__VELOCITY_CHESS_EXTENSION_CHANNEL_V045__';
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

  const channel = randomChannel();
  await chrome.scripting.executeScript({
    target: { tabId },
    world: 'MAIN',
    func: (key, value) => { globalThis[key] = value; },
    args: [CHANNEL_KEY, channel]
  });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'MAIN', files: ['visual-board.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'MAIN', files: ['humanize.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'MAIN', files: ['page-bridge.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['humanize.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['skill-levels.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['content-script.js'] });
  return invokeController(tabId, 'start', [channel, inputSettings || {}]);
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
      const current = await invokeController(activeTab.id, 'status', [], { missingOk: true });
      if (current?.ok) show(current.status, Boolean(current.error));
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
    const response = await invokeController(activeTab.id, 'stop', [], { missingOk: true });
    show(response?.ok ? 'Stopped.' : response?.error || 'Unable to stop.', !response?.ok);
  } catch (error) {
    show(error instanceof Error ? error.message : String(error), true);
  }
});

applyMatchMode('5m');
void refresh().catch((error) => show(error instanceof Error ? error.message : String(error), true));
