import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const read = (name) => fs.readFileSync(path.join(root, name), 'utf8');

test('manifest declares the generated RPC host prerequisites', () => {
  const manifest = JSON.parse(read('manifest.json'));
  assert.equal(manifest.version, '0.11.0');
  assert.deepEqual(manifest.background, { service_worker: 'background.js', type: 'module' });
  assert.ok(manifest.permissions.includes('offscreen'));
  assert.match(manifest.content_security_policy.extension_pages, /wasm-unsafe-eval/);
});

test('extension control path uses the bounded protected RPC bridge', () => {
  const background = read('background.js');
  assert.match(background, /VenomExtensionRPC\.call\('analyzeVelocity'/);
  assert.match(background, /runtime\.onMessage/);
  assert.doesNotMatch(background, /VelocityChessEngine|analyzeFen\(/);
});

test('engine implementation remains behind the protected export', () => {
  const protectedEngine = read('protected/velocity-engine.js');
  assert.match(protectedEngine, /^\/\/ @venom: protected module/m);
  assert.match(protectedEngine, /export function analyzeVelocity/);
  assert.match(protectedEngine, /VelocityChessEngine/);
});

test('popup injects only Chrome and DOM adapters', () => {
  const popup = read('popup.js');
  assert.doesNotMatch(popup, /engine-bundle\.js/);
  assert.match(popup, /chrome\.scripting\.executeScript/);
  assert.match(popup, /__VELOCITY_CHESS_CONTROLLER_V0110__/);
});
