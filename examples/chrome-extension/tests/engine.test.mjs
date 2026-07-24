import test from 'node:test';
import assert from 'node:assert/strict';
import { analyzeFen } from '../engine/runtime.js';

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

test('Velocity returns a legal-looking move from the initial position', () => {
  const result = analyzeFen(START, { maxDepth: 2, moveTimeMs: 250, hashBits: 16 });
  assert.match(result.move, /^[a-h][1-8][a-h][1-8][qrbn]?$/);
  assert.equal(result.turn, 'w');
  assert.equal(result.legalMoves, 20);
  assert.ok(result.nodes > 0);
});

test('Velocity reports no move in a checkmated position', () => {
  const result = analyzeFen('7k/6Q1/6K1/8/8/8/8/8 b - - 0 1', { maxDepth: 2, moveTimeMs: 100 });
  assert.equal(result.move, null);
  assert.equal(result.legalMoves, 0);
});
