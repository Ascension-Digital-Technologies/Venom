'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const enginePath = path.resolve(__dirname, '..', 'js', 'ai-engine.js');
vm.runInThisContext(
  fs.readFileSync(enginePath, 'utf8') + '\nglobalThis.__venomChessBenchmarkEntry = runChessEngine;',
  { filename: enginePath }
);
const run = globalThis.__venomChessBenchmarkEntry;

const positions = [
  ['Starting position', 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'],
  ['Kiwipete', 'r3k2r/p1ppqpb1/bn2pnp1/2pP4/1p2P3/2N2N2/PPQBBPPP/R3K2R w KQkq - 0 1'],
  ['Middlegame', '2r2rk1/pp1bqppp/2n1pn2/2pp4/3P4/2PBPN2/PPQ2PPP/R1B2RK1 w - - 0 12'],
  ['Endgame', '8/5pk1/6p1/3P4/4P3/5K2/8/8 w - - 0 40']
];

const maxDepth = Number(process.argv[2] || 8);
const timeMs = Number(process.argv[3] || 1500);
console.log(`Protected Chess Engine 2.0 benchmark · max depth ${maxDepth} · ${timeMs} ms budget\n`);

for (const [name, fen] of positions) {
  const result = run({ action: 'search', fen, maxDepth, timeMs });
  const move = result.move ? `${result.move.san || result.move.from + result.move.to}` : 'none';
  console.log(name);
  console.log(`  move: ${move}`);
  console.log(`  depth: ${result.depth}/${result.requestedDepth}${result.timedOut ? ' (time-limited)' : ''}`);
  console.log(`  evaluation: ${(result.value / 100).toFixed(2)} pawns (White-positive)`);
  console.log(`  nodes: ${result.positions.toLocaleString()} (${result.qnodes.toLocaleString()} quiescence)`);
  console.log(`  TT hits: ${result.ttHits.toLocaleString()} · cutoffs: ${result.cutoffs.toLocaleString()}`);
  console.log(`  elapsed: ${result.elapsedMs} ms · NPS: ${result.positionsPerSecond.toLocaleString()}`);
  console.log(`  PV: ${result.pv.join(' ') || '—'}\n`);
}
