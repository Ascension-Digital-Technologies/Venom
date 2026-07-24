import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

const pageBridge = await readFile(new URL('../page-bridge.js', import.meta.url), 'utf8');
const content = await readFile(new URL('../content-script.js', import.meta.url), 'utf8');
const popup = await readFile(new URL('../popup.js', import.meta.url), 'utf8');

test('black-side webpage takeover code is absent', () => {
  for (const source of [pageBridge, content, popup]) {
    assert.equal(source.includes('makeBestMove'), false);
    assert.equal(source.includes('suppressPageAutoReply'), false);
    assert.equal(source.includes('SESSION_START'), false);
    assert.equal(source.includes('SESSION_STOP'), false);
  }
});


test('detected Black side is passed into visual turn bootstrap', () => {
  assert.match(pageBridge, /playerSide:\s*detectedSide\.side/);
  assert.match(pageBridge, /tracker\.update\(snapshot/);
});
