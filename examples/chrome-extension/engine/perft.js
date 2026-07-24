import { generatePseudoLegal, createMoveBuffer } from './movegen.js';
import { MAX_MOVES, MAX_PLY } from './constants.js';

const moveBuffer = createMoveBuffer(MAX_PLY);

export function perft(position, depth, ply = 0) {
  if (depth === 0) return 1;
  const offset = ply * MAX_MOVES;
  const count = generatePseudoLegal(position, moveBuffer, offset);
  let nodes = 0;
  for (let i = 0; i < count; i++) {
    const move = moveBuffer[offset + i];
    if (!position.makeMove(move)) continue;
    nodes += depth === 1 ? 1 : perft(position, depth - 1, ply + 1);
    position.unmakeMove();
  }
  return nodes;
}
