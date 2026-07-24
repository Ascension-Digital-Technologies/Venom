// @venom: browser
import { matchAllowedSite, parseAllowedSites } from './lib/allowlist.js';

let allowlistPromise = null;
const CONTROLLER_KEY = '__VELOCITY_CHESS_CONTROLLER_V0110__';
const CHANNEL_KEY = '__VELOCITY_CHESS_EXTENSION_CHANNEL_V0110__';

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
        if (tolerateMissing) return { ok: true, alreadyStopped: true };
        throw new Error('Velocity page controller was not installed');
      }
      return controller[requestedMethod](...(Array.isArray(requestedArgs) ? requestedArgs : []));
    },
    args: [CONTROLLER_KEY, method, args, missingOk]
  });
  const value = results?.[0]?.result;
  if (value && typeof value === 'object') return value;
  return { ok: true };
}

async function injectPlayer(tabId, url, settings) {
  if (!Number.isInteger(tabId)) throw new Error('No active browser tab was found');
  if (!(await isAllowed(url))) throw new Error('This page is not listed in allowed_sites.txt');

  if (!globalThis.VenomExtensionRPC || typeof globalThis.VenomExtensionRPC.ready !== 'function') {
    throw new Error('Protected chess engine RPC is unavailable');
  }
  await globalThis.VenomExtensionRPC.ready();

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
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['mouse-recorder.js'] });
  await chrome.scripting.executeScript({ target: { tabId }, world: 'ISOLATED', files: ['content-script.js'] });

  const started = await invokeController(tabId, 'start', [channel, settings || {}]);
  return { ok: started?.ok !== false, channel, ...started };
}

chrome.runtime.onMessage.addListener((message, _sender, sendResponse) => {
  if (!message || typeof message.type !== 'string') return undefined;

  if (message.type === 'VELOCITY_CHECK_ALLOWED') {
    isAllowed(message.url)
      .then((allowed) => sendResponse({ ok: true, allowed }))
      .catch((error) => sendResponse({ ok: false, error: error.message }));
    return true;
  }

  if (message.type === 'VELOCITY_START_ACTIVE_TAB') {
    injectPlayer(message.tabId, message.url, message.settings || {})
      .then(sendResponse)
      .catch((error) => sendResponse({ ok: false, error: error.message }));
    return true;
  }

  if (message.type === 'VELOCITY_STATUS_TAB' && Number.isInteger(message.tabId)) {
    invokeController(message.tabId, 'status', [], { missingOk: true })
      .then(sendResponse)
      .catch(() => sendResponse({ ok: true, running: false, status: 'Not started' }));
    return true;
  }

  if (message.type === 'VELOCITY_ANALYZE') {
    try {
      globalThis.VenomExtensionRPC.call('analyzeVelocity', { fen: message.fen, options: message.options || {} })
        .then((result) => sendResponse({ ok: true, result }))
        .catch((error) => sendResponse({ ok: false, error: error instanceof Error ? error.message : String(error) }));
      return true;
    } catch (error) {
      sendResponse({ ok: false, error: error instanceof Error ? error.message : String(error) });
      return false;
    }
  }

  if (message.type === 'VELOCITY_STOP_TAB' && Number.isInteger(message.tabId)) {
    invokeController(message.tabId, 'stop', [], { missingOk: true })
      .then(sendResponse)
      .catch((error) => sendResponse({ ok: false, error: error instanceof Error ? error.message : String(error) }));
    return true;
  }

  return undefined;
});
