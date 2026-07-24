import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';

const listeners = [];
const injections = [];
const controllerCalls = [];
const root = resolve(import.meta.dirname, '..');

globalThis.chrome = {
  runtime: {
    getURL(path) { return pathToFileURL(resolve(root, path)).href; },
    onMessage: { addListener(listener) { listeners.push(listener); } }
  },
  scripting: {
    executeScript: async (request) => {
      injections.push(request);
      if (typeof request.func === 'function' && request.world === 'ISOLATED') {
        const [, method, args, missingOk] = request.args || [];
        controllerCalls.push({ method, args, missingOk });
        if (method === 'status') return [{ result: { ok: true, running: true, status: 'Auto-detected Black' } }];
        if (method === 'stop') return [{ result: { ok: true } }];
        if (method === 'start') return [{ result: { ok: true } }];
      }
      return [];
    }
  }
};


globalThis.VenomExtensionRPC = Object.freeze({
  async ready() { return true; },
  async call(name, payload) {
    assert.equal(name, 'analyzeVelocity');
    assert.equal(typeof payload?.fen, 'string');
    return { move: 'e2e4', legalMoves: 20, score: 0, depth: 2, nodes: 1 };
  }
});

globalThis.fetch = async (url) => {
  const text = await readFile(new URL(url), 'utf8');
  return { ok: true, status: 200, text: async () => text };
};

await import('../background.js');
const listener = listeners[0];

function message(payload) {
  return new Promise((resolvePromise, reject) => {
    let settled = false;
    const sendResponse = (value) => {
      settled = true;
      resolvePromise(value);
    };
    try {
      const asyncResponse = listener(payload, {}, sendResponse);
      if (asyncResponse !== true && !settled) resolvePromise(undefined);
      if (asyncResponse === true) setTimeout(() => {
        if (!settled) reject(new Error('Background response timed out'));
      }, 2500);
    } catch (error) {
      reject(error);
    }
  });
}

test('background performs allowlist checks', async () => {
  const allowed = await message({ type: 'VELOCITY_CHECK_ALLOWED', url: 'http://localhost:8080/game' });
  const blocked = await message({ type: 'VELOCITY_CHECK_ALLOWED', url: 'https://blocked.example/game' });
  assert.deepEqual(allowed, { ok: true, allowed: true });
  assert.deepEqual(blocked, { ok: true, allowed: false });
});

test('background runs the packaged Velocity engine', async () => {
  const reply = await message({
    type: 'VELOCITY_ANALYZE',
    fen: 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
    options: { maxDepth: 2, moveTimeMs: 100, hashBits: 16 }
  });
  assert.equal(reply.ok, true);
  assert.match(reply.result.move, /^[a-h][1-8][a-h][1-8][qrbn]?$/);
  assert.equal(reply.result.legalMoves, 20);
});

test('background starts through a direct isolated-world controller call', async () => {
  injections.length = 0;
  controllerCalls.length = 0;
  const reply = await message({
    type: 'VELOCITY_START_ACTIVE_TAB',
    tabId: 42,
    url: 'http://localhost:8080/game',
    settings: { side: 'auto', matchMode: '5m' }
  });
  assert.equal(reply.ok, true);
  assert.deepEqual(injections.filter((request) => request.files).map((request) => request.files), [
    ['visual-board.js'],
    ['humanize.js'],
    ['page-bridge.js'],
    ['humanize.js'],
    ['skill-levels.js'],
    ['mouse-recorder.js'],
    ['content-script.js']
  ]);
  assert.equal(controllerCalls.at(-1).method, 'start');
  assert.equal(controllerCalls.at(-1).args[1].matchMode, '5m');
});

test('status and stop also use the direct controller instead of tabs messaging', async () => {
  controllerCalls.length = 0;
  const status = await message({ type: 'VELOCITY_STATUS_TAB', tabId: 42 });
  const stopped = await message({ type: 'VELOCITY_STOP_TAB', tabId: 42 });
  assert.equal(status.ok, true);
  assert.equal(status.running, true);
  assert.equal(stopped.ok, true);
  assert.deepEqual(controllerCalls.map((call) => call.method), ['status', 'stop']);
});

test('background contains no tabs.sendMessage startup dependency', async () => {
  const source = await readFile(resolve(root, 'background.js'), 'utf8');
  assert.doesNotMatch(source, /tabs\.sendMessage/);
  assert.match(source, /__VELOCITY_CHESS_CONTROLLER_V0110__/);
});
