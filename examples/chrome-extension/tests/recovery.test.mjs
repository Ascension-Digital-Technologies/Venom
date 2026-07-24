import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';

const root = resolve(import.meta.dirname, '..');

test('every start performs a hard visual tracker reset before resuming', async () => {
  const controller = await readFile(resolve(root, 'content-script.js'), 'utf8');
  const bridge = await readFile(resolve(root, 'page-bridge.js'), 'utf8');
  assert.match(controller, /await hardResetVisualState\(\)/);
  assert.match(controller, /bridge\('RESET_STATE'/);
  assert.match(bridge, /tracker\.reset\(\)/);
  assert.match(bridge, /request\.type === 'RESET_STATE'/);
});

test('transient visual failures use bounded self recovery', async () => {
  const source = await readFile(resolve(root, 'content-script.js'), 'utf8');
  assert.match(source, /session\.recoveryCount >= 5/);
  assert.match(source, /recoverFromTransientError/);
  assert.match(source, /Rebuilding state from the live board/);
});

test('recovery release uses isolated versioned controller keys', async () => {
  const popup = await readFile(resolve(root, 'popup.js'), 'utf8');
  const controller = await readFile(resolve(root, 'content-script.js'), 'utf8');
  const bridge = await readFile(resolve(root, 'page-bridge.js'), 'utf8');
  assert.match(popup, /V0110/);
  assert.match(controller, /V0110/);
  assert.match(bridge, /V0110/);
});

test('successful reads reset the consecutive recovery budget', async () => {
  const source = await readFile(resolve(root, 'content-script.js'), 'utf8');
  assert.match(source, /const state = await readState\(\);\s*session\.recoveryCount = 0;/);
  assert.match(source, /totalRecoveries/);
});

test('startup retries a live-board rebuild instead of failing once', async () => {
  const source = await readFile(resolve(root, 'content-script.js'), 'utf8');
  assert.match(source, /attempt <= 6/);
  assert.match(source, /Rebuilding live position/);
});

test('page bridge automatically reseeds after skipped or complex visual transitions', async () => {
  const bridge = await readFile(resolve(root, 'page-bridge.js'), 'utf8');
  const visual = await readFile(resolve(root, 'visual-board.js'), 'utf8');
  assert.match(bridge, /tracker\.resync/);
  assert.match(bridge, /automatic-live-board-resync/);
  assert.match(visual, /resync\(snapshot, hints/);
  assert.match(visual, /visual-dom-resync/);
});

test('controller proactively refreshes a frozen valid-looking board', async () => {
  const source = await readFile(resolve(root, 'content-script.js'), 'utf8');
  assert.match(source, /shouldProactivelyRecover/);
  assert.match(source, /position stopped updating/);
  assert.match(source, /proactiveRecoveries/);
  assert.match(source, /staleProbeCount >= 36/);
});

test('visual reader ranks all viable board roots and preserves active-root affinity', async () => {
  const visual = await readFile(resolve(root, 'visual-board.js'), 'utf8');
  const bridge = await readFile(resolve(root, 'page-bridge.js'), 'utf8');
  assert.match(visual, /bestSnapshotScore/);
  assert.match(visual, /root === preferredRoot/);
  assert.match(bridge, /visual\.scan\(document, latestSnapshot\?\.root \|\| null\)/);
  assert.match(bridge, /boardHealth/);
});
