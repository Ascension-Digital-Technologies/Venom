import { WHITE, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK } from './constants.js';
import { KNIGHT_TARGETS, KNIGHT_COUNTS, KING_TARGETS, KING_COUNTS, RAY_TARGETS, RAY_COUNTS } from './attacks.js';

const MG_VALUE = new Int16Array([0, 100, 325, 335, 500, 950, 0, -100, -325, -335, -500, -950, 0]);
const EG_VALUE = new Int16Array([0, 120, 310, 330, 520, 930, 0, -120, -310, -330, -520, -930, 0]);
const PHASE = new Uint8Array([0, 0, 1, 1, 2, 4, 0, 0, 1, 1, 2, 4, 0]);

function centerBonus(square) {
  const f = square & 7, r = square >>> 3;
  return 7 - (Math.abs(f * 2 - 7) + Math.abs(r * 2 - 7)) / 2;
}

const MG_PST = Array.from({ length: 13 }, () => new Int16Array(64));
const EG_PST = Array.from({ length: 13 }, () => new Int16Array(64));
for (let p = 1; p <= 12; p++) {
  const white = p <= 6, type = white ? p : p - 6;
  for (let sq = 0; sq < 64; sq++) {
    const view = white ? sq : (sq ^ 56);
    const rank = view >>> 3, file = view & 7, center = centerBonus(view);
    let mg = 0, eg = 0;
    if (type === 1) { mg = rank * 8 - Math.abs(file * 2 - 7) * 2; eg = rank * 12; }
    else if (type === 2) { mg = center * 8 - (rank === 0 ? 12 : 0); eg = center * 5; }
    else if (type === 3) { mg = center * 5 + rank * 2; eg = center * 4; }
    else if (type === 4) { mg = rank * 2 + (file === 0 || file === 7 ? -2 : 2); eg = rank * 3; }
    else if (type === 5) { mg = center * 2; eg = center * 2; }
    else if (type === 6) { mg = -center * 7 - rank * 2; eg = center * 8; }
    MG_PST[p][sq] = white ? mg : -mg;
    EG_PST[p][sq] = white ? eg : -eg;
  }
}

function mobility(position, square, piece) {
  const board = position.board, side = piece <= WK ? 0 : 1;
  if (piece === WN || piece === BN) {
    let n = 0, base = square * 8;
    for (let i = 0; i < KNIGHT_COUNTS[square]; i++) {
      const target = board[KNIGHT_TARGETS[base + i]];
      if (!target || (target <= WK ? 0 : 1) !== side) n++;
    }
    return n * 3;
  }
  if (piece === WK || piece === BK) {
    let n = 0, base = square * 8;
    for (let i = 0; i < KING_COUNTS[square]; i++) {
      const target = board[KING_TARGETS[base + i]];
      if (!target || (target <= WK ? 0 : 1) !== side) n++;
    }
    return n;
  }
  if (piece === WB || piece === BB || piece === WR || piece === BR || piece === WQ || piece === BQ) {
    const start = (piece === WB || piece === BB) ? 0 : (piece === WR || piece === BR) ? 4 : 0;
    const end = (piece === WB || piece === BB) ? 4 : (piece === WR || piece === BR) ? 8 : 8;
    let n = 0;
    for (let d = start; d < end; d++) {
      const rayIndex = square * 8 + d, base = rayIndex * 7, count = RAY_COUNTS[rayIndex];
      for (let i = 0; i < count; i++) {
        const target = board[RAY_TARGETS[base + i]];
        if (!target) n++;
        else { if ((target <= WK ? 0 : 1) !== side) n++; break; }
      }
    }
    return n * (piece === WQ || piece === BQ ? 1 : 2);
  }
  return 0;
}

export function evaluate(position) {
  let mg = 0, eg = 0, phase = 0, whiteBishops = 0, blackBishops = 0;
  const lists = position.pieceSquares, counts = position.pieceCount;
  for (let piece = 1; piece <= 12; piece++) {
    const base = piece * 16, count = counts[piece];
    for (let i = 0; i < count; i++) {
      const sq = lists[base + i];
      mg += MG_VALUE[piece] + MG_PST[piece][sq];
      eg += EG_VALUE[piece] + EG_PST[piece][sq];
      phase += PHASE[piece];
      const mob = mobility(position, sq, piece);
      if (piece <= WK) { mg += mob; eg += mob; } else { mg -= mob; eg -= mob; }
    }
    if (piece === WB) whiteBishops = count;
    else if (piece === BB) blackBishops = count;
  }
  if (whiteBishops >= 2) { mg += 28; eg += 38; }
  if (blackBishops >= 2) { mg -= 28; eg -= 38; }
  if (phase > 24) phase = 24;
  const score = ((mg * phase + eg * (24 - phase)) / 24) | 0;
  return position.side === WHITE ? score : -score;
}
