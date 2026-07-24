import { Position } from './position.js';
import { Search } from './search.js';
import { generateLegal } from './movegen.js';
import { moveToUci } from './move.js';

let sharedSearch = null;
let sharedBits = 18;

function getSearch(hashBits = 18) {
  const bits = Math.max(14, Math.min(20, Number(hashBits) || 18));
  if (!sharedSearch || sharedBits !== bits) {
    sharedBits = bits;
    sharedSearch = new Search(bits);
  }
  return sharedSearch;
}

export function analyzeFen(fen, options = {}) {
  if (typeof fen !== 'string' || !fen.trim()) throw new Error('A valid FEN string is required');

  const position = new Position(fen.trim());
  const legalMoves = generateLegal(position);
  const started = performance.now();
  if (!legalMoves.length) {
    return {
      move: null,
      score: 0,
      depth: 0,
      nodes: 0,
      legalMoves: 0,
      elapsedMs: performance.now() - started,
      turn: position.side === 0 ? 'w' : 'b'
    };
  }

  const maxDepth = Math.max(1, Math.min(12, Number(options.maxDepth) || 4));
  const moveTimeMs = Math.max(25, Math.min(15000, Number(options.moveTimeMs) || 1000));
  const search = getSearch(options.hashBits);
  const candidateCount = Math.max(1, Math.min(12, Number(options.candidateCount) || 1));
  let result;
  let candidates = [];
  if (candidateCount > 1) {
    const ranked = search.rankRootMoves(position, maxDepth, moveTimeMs, candidateCount);
    candidates = ranked.candidates.map((entry) => ({
      move: moveToUci(entry.move),
      score: Number(entry.score) || 0,
      capture: Boolean(entry.capture),
      promotion: Boolean(entry.promotion),
      check: Boolean(entry.check)
    }));
    result = {
      move: ranked.candidates[0]?.move || legalMoves[0],
      score: ranked.candidates[0]?.score || 0,
      depth: ranked.depth,
      nodes: ranked.nodes
    };
  } else {
    result = search.iterative(position, maxDepth, moveTimeMs, () => {});
  }
  const move = result.move ? moveToUci(result.move) : moveToUci(legalMoves[0]);
  if (!candidates.length) candidates = [{ move, score: Number(result.score) || 0, capture: false, promotion: false, check: false }];

  return {
    move,
    candidates,
    score: Number(result.score) || 0,
    depth: Number(result.depth) || 0,
    nodes: Number(result.nodes ?? search.nodes) || 0,
    legalMoves: legalMoves.length,
    elapsedMs: performance.now() - started,
    turn: position.side === 0 ? 'w' : 'b'
  };
}
