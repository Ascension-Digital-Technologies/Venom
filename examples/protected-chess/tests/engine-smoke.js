'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const enginePath = path.resolve(__dirname, '..', 'js', 'ai-engine.js');
const source = fs.readFileSync(enginePath, 'utf8') + '\nglobalThis.__venomChessTestEntry = runChessEngine;';
vm.runInThisContext(source, { filename: enginePath });
const run = globalThis.__venomChessTestEntry;

function test(name, fn) {
  const started = Date.now();
  fn();
  console.log(`[PASS] ${name} (${Date.now() - started} ms)`);
}

test('identity advertises Engine 2.0 search features', () => {
  const identity = run({ action: 'identity' });
  assert.strictEqual(identity.version, '2.0.0');
  assert.strictEqual(identity.scoreConvention, 'positive-white-centipawns');
  assert(identity.capabilities.includes('iterative-deepening'));
  assert(identity.capabilities.includes('quiescence-search'));
  assert(identity.capabilities.includes('transposition-table'));
});

test('starting-position perft matches known legal-move counts', () => {
  const expected = { 1: 20, 2: 400, 3: 8902, 4: 197281 };
  for (const [depth, nodes] of Object.entries(expected)) {
    assert.strictEqual(run({ action: 'perft', depth: Number(depth) }).nodes, nodes);
  }
});

test('full-board evaluation is stable across evaluate calls', () => {
  const start = run({ action: 'evaluate' });
  assert(Number.isFinite(start.value));
  assert.strictEqual(start.value, start.state.evaluation);
  const afterMove = run({
    action: 'move',
    fen: start.state.fen,
    history: [],
    move: { from: 'e2', to: 'e4', promotion: 'q' }
  });
  const reevaluated = run({ action: 'evaluate', fen: afterMove.state.fen, history: afterMove.state.history });
  assert.strictEqual(afterMove.value, reevaluated.value);
});

test('deterministic search returns the same principal move', () => {
  const request = { action: 'search', maxDepth: 3, timeMs: 5000 };
  const first = run(request);
  const second = run(request);
  assert(first.move);
  assert(second.move);
  assert.deepStrictEqual(
    { from: first.move.from, to: first.move.to, promotion: first.move.promotion },
    { from: second.move.from, to: second.move.to, promotion: second.move.promotion }
  );
  assert.strictEqual(first.depth, 3);
  assert.strictEqual(second.depth, 3);
  assert(second.ttHits > 0);
});

test('search-and-play returns one authoritative updated snapshot', () => {
  const result = run({ action: 'search', maxDepth: 3, timeMs: 5000, play: true, history: [] });
  assert(result.move);
  assert(result.state);
  assert.notStrictEqual(result.state.fen, 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1');
  assert.strictEqual(result.state.history.length, 1);
  assert.strictEqual(result.state.evaluation, result.value);
});

test('mate-in-one is found and applied', () => {
  const result = run({
    action: 'search',
    fen: '7k/5Q2/6K1/8/8/8/8/8 w - - 0 1',
    maxDepth: 3,
    timeMs: 5000,
    play: true,
    history: []
  });
  assert(result.move);
  assert(result.value > 990000);
  assert(result.state.checkmate);
  assert.strictEqual(result.state.history[0], 'Qg7#');
});

test('time-limited interruption preserves the root position', () => {
  const result = run({ action: 'search', maxDepth: 12, timeMs: 50 });
  assert(result.move);
  assert.strictEqual(result.rootTurn, 'w');
  assert(result.depth >= 1);
  assert(result.elapsedMs < 750);
});

console.log('\nProtected Chess Engine 2.0 smoke suite passed.');
