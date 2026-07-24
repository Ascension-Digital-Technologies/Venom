import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../match-modes.js', import.meta.url), 'utf8');
const context = vm.createContext({ console });
vm.runInContext(source, context, { filename: 'match-modes.js' });
const modes = context.VelocityMatchModes;

test('provides 1m, 3m, 5m, and 10m match modes', () => {
  assert.deepEqual(Object.keys(modes.PRESETS), ['1m', '3m', '5m', '10m']);
});

test('longer match modes receive larger search and timing budgets', () => {
  const one = modes.getPreset('1m');
  const three = modes.getPreset('3m');
  const five = modes.getPreset('5m');
  const ten = modes.getPreset('10m');
  assert.ok(one.moveTimeMs < three.moveTimeMs);
  assert.ok(three.moveTimeMs < five.moveTimeMs);
  assert.ok(five.moveTimeMs < ten.moveTimeMs);
  assert.ok(one.maxDelayMs < ten.maxDelayMs);
  assert.ok(one.dragMaxMs < ten.dragMaxMs);
});

test('unknown modes fail closed', () => {
  assert.equal(modes.getPreset('30m'), null);
});
