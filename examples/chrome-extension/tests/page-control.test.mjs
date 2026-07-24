import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../page-bridge.js', import.meta.url), 'utf8');

function createBridgeContext() {
  const listeners = new Map();
  class CustomEvent { constructor(type, init = {}) { this.type = type; this.detail = init.detail; } }
  const tracker = { resetCalls: 0, reset() { this.resetCalls++; }, update() { throw new Error('not used'); } };
  const contextObject = {
    console, setTimeout, clearTimeout, CustomEvent,
    VelocityVisualBoardV090: { createTracker() { return tracker; } },
    VelocityHumanizer: { buildPath() { return []; } },
    document: {
      createElement() { return { style: {}, appendChild() {}, remove() {}, isConnected: false }; },
      createElementNS() { return { setAttribute() {}, appendChild() {} }; },
      documentElement: { appendChild() {} }, body: { appendChild() {} }
    },
    addEventListener(type, listener) { const group = listeners.get(type) || []; group.push(listener); listeners.set(type, group); },
    dispatchEvent(event) { for (const listener of listeners.get(event.type) || []) listener(event); return true; }
  };
  contextObject.window = contextObject;
  contextObject.globalThis = contextObject;
  contextObject.__VELOCITY_CHESS_EXTENSION_CHANNEL_V0110__ = 'test-channel';
  const context = vm.createContext(contextObject);
  vm.runInContext(source, context, { filename: 'page-bridge.js' });
  return { context, tracker };
}

async function request(context, type, payload = {}) {
  const response = new Promise((resolve) => {
    context.addEventListener('VELOCITY_CHESS_EXTENSION_RESPONSE_V0110', (event) => {
      const data = JSON.parse(event.detail);
      if (data.id === 'request-1') resolve(data);
    });
  });
  context.dispatchEvent(new context.CustomEvent('VELOCITY_CHESS_EXTENSION_REQUEST_V0110', {
    detail: JSON.stringify({ channel: 'test-channel', id: 'request-1', type, payload })
  }));
  return response;
}

test('page bridge exposes the current reset contract', async () => {
  const { context, tracker } = createBridgeContext();
  const response = await request(context, 'RESET_STATE');
  assert.equal(response.ok, true);
  assert.equal(response.result.reset, true);
  assert.equal(tracker.resetCalls, 1);
});

test('page bridge fails closed for removed legacy session commands', async () => {
  const { context } = createBridgeContext();
  const response = await request(context, 'SESSION_START', { settings: { side: 'both' } });
  assert.equal(response.ok, false);
  assert.match(response.error, /Unknown bridge request/);
});
