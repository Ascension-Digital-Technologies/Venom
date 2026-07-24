import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';

const root = resolve(import.meta.dirname, '..');

test('content controller is replaceable after an extension reload', async () => {
  const source = await readFile(resolve(root, 'content-script.js'), 'utf8');
  assert.match(source, /previousController\.dispose\(\)/);
  assert.match(source, /globalThis\[CONTROLLER_KEY\] = controller/);
  assert.doesNotMatch(source, /__VELOCITY_CHESS_CONTENT_INSTALLED_V040__/);
});

test('popup delegates engine lifecycle to the protected background broker', async () => {
  const source = await readFile(resolve(root, 'popup.js'), 'utf8');
  assert.match(source, /VELOCITY_START_ACTIVE_TAB/);
  assert.match(source, /VELOCITY_STATUS_TAB/);
  assert.match(source, /VELOCITY_STOP_TAB/);
  assert.match(source, /chrome\.runtime\.sendMessage/);
});
