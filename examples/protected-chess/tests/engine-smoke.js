'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const enginePath = path.resolve(__dirname, '..', 'js', 'ai-engine.js');
vm.runInThisContext(fs.readFileSync(enginePath, 'utf8') + '\nglobalThis.__venomVelocityChessTestEntry = runChessEngine;', { filename: enginePath });
const run = globalThis.__venomVelocityChessTestEntry;

function test(name, fn) {
  const started = Date.now();
  fn();
  console.log(`[PASS] ${name} (${Date.now() - started} ms)`);
}

test('identity advertises the protected Velocity 0.5.0 pipeline', () => {
  const identity = run({ action: 'identity' });
  assert.strictEqual(identity.name, 'Velocity Chess');
  assert.strictEqual(identity.version, '0.5.0');
  assert.strictEqual(identity.protected, true);
  assert(identity.capabilities.includes('packed-32-bit-moves'));
  assert(identity.capabilities.includes('incremental-zobrist-hashing'));
  assert(identity.capabilities.includes('null-move-pruning'));
});

test('starting-position perft matches canonical depths 1-5', () => {
  const expected = { 1: 20, 2: 400, 3: 8902, 4: 197281, 5: 4865609 };
  for (const [depth, nodes] of Object.entries(expected)) assert.strictEqual(run({ action: 'perft', depth: Number(depth) }).nodes, nodes);
});

test('state and legal move bridge preserve FEN and expose 20 legal moves', () => {
  const state = run({ action: 'state' }).state;
  assert.strictEqual(state.fen, 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1');
  const moves = run({ action: 'moves', fen: state.fen }).moves;
  assert.strictEqual(moves.length, 20);
  assert(moves.some(move => move.uci === 'e2e4'));
});

test('protected move applies e4 and returns SAN plus authoritative state', () => {
  const result = run({ action: 'move', move: { from: 'e2', to: 'e4', promotion: 'q' }, history: [] });
  assert.strictEqual(result.move.san, 'e4');
  assert.strictEqual(result.state.turn, 'b');
  assert.strictEqual(result.state.history[0], 'e4');
  assert.strictEqual(result.state.fen, 'rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1');
});

test('full-board evaluation is stable after FEN reconstruction', () => {
  const moved = run({ action: 'move', move: 'e2e4', history: [] });
  const reevaluated = run({ action: 'evaluate', fen: moved.state.fen, history: moved.state.history, repetition: moved.state.repetition });
  assert.strictEqual(reevaluated.value, reevaluated.state.evaluation);
  assert(Number.isFinite(reevaluated.value));
});

test('deterministic search reuses the protected transposition table', () => {
  const request = { action: 'search', maxDepth: 4, timeMs: 5000 };
  const first = run(request);
  const second = run(request);
  assert(first.move && second.move);
  assert.strictEqual(first.move.uci, second.move.uci);
  assert.strictEqual(first.depth, 4);
  assert.strictEqual(second.depth, 4);
  assert(second.ttHits > 0);
  assert(Array.isArray(second.pv));
});

test('search-and-play returns one authoritative updated snapshot', () => {
  const result = run({ action: 'search', maxDepth: 3, timeMs: 5000, play: true, history: [] });
  assert(result.move && result.state);
  assert.notStrictEqual(result.state.fen, 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1');
  assert.strictEqual(result.state.history.length, 1);
  assert.strictEqual(result.state.evaluation, result.value);
});

test('mate-in-one is found, formatted, and applied', () => {
  const result = run({ action: 'search', fen: '7k/5Q2/6K1/8/8/8/8/8 w - - 0 1', maxDepth: 3, timeMs: 5000, play: true, history: [] });
  assert(result.move);
  assert(result.value > 990000);
  assert(result.state.checkmate);
  assert(result.move.san.endsWith('#'));
});

test('threefold repetition is detected from protected position history', () => {
  let state = run({ action: 'state' }).state;
  for (const move of ['g1f3', 'g8f6', 'f3g1', 'f6g8', 'g1f3', 'g8f6', 'f3g1', 'f6g8']) {
    const result = run({ action: 'move', fen: state.fen, move, history: state.history, repetition: state.repetition });
    state = result.state;
  }
  assert.strictEqual(state.threefoldRepetition, true);
  assert.strictEqual(state.gameOver, true);
});

test('time-limited search returns a legal move and restores the root', () => {
  const result = run({ action: 'search', maxDepth: 16, timeMs: 25 });
  assert(result.move);
  assert.strictEqual(result.rootTurn, 'w');
  assert(result.depth >= 0);
  assert(result.elapsedMs < 1000);
  const state = run({ action: 'state' }).state;
  assert.strictEqual(state.fen, 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1');
});

assert.throws(() => run({ action: '__invalid_runtime_probe__' }), /unsupported chess engine action/);
console.log('\nProtected Velocity Chess 0.5.0 smoke suite passed.');
