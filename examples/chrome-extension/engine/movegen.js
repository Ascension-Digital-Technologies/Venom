import {
  WHITE, EMPTY, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK,
  FLAG_NONE, FLAG_CAPTURE, FLAG_EP, FLAG_CASTLE, FLAG_PROMOTION,
  CASTLE_WK, CASTLE_WQ, CASTLE_BK, CASTLE_BQ, MAX_MOVES, colorOf
} from './constants.js';
import { encodeMove } from './move.js';
import { KNIGHT_TARGETS, KNIGHT_COUNTS, KING_TARGETS, KING_COUNTS, RAY_TARGETS, RAY_COUNTS } from './attacks.js';

export function createMoveBuffer(plies = 1) { return new Uint32Array(plies * MAX_MOVES); }

function addPromotions(out, count, from, to, piece, captured, side, baseFlags) {
  if (side === WHITE) {
    out[count++] = encodeMove(from, to, piece, captured, WQ, baseFlags | FLAG_PROMOTION);
    out[count++] = encodeMove(from, to, piece, captured, WR, baseFlags | FLAG_PROMOTION);
    out[count++] = encodeMove(from, to, piece, captured, WB, baseFlags | FLAG_PROMOTION);
    out[count++] = encodeMove(from, to, piece, captured, WN, baseFlags | FLAG_PROMOTION);
  } else {
    out[count++] = encodeMove(from, to, piece, captured, BQ, baseFlags | FLAG_PROMOTION);
    out[count++] = encodeMove(from, to, piece, captured, BR, baseFlags | FLAG_PROMOTION);
    out[count++] = encodeMove(from, to, piece, captured, BB, baseFlags | FLAG_PROMOTION);
    out[count++] = encodeMove(from, to, piece, captured, BN, baseFlags | FLAG_PROMOTION);
  }
  return count;
}

export function generatePseudoLegal(position, out, offset = 0, capturesOnly = false) {
  const b = position.board, side = position.side, lists = position.pieceSquares, counts = position.pieceCount;
  let count = offset;
  const first = side === WHITE ? WP : BP, last = side === WHITE ? WK : BK;

  for (let piece = first; piece <= last; piece++) {
    const listBase = piece * 16, pieceCount = counts[piece];
    for (let li = 0; li < pieceCount; li++) {
      const from = lists[listBase + li], file = from & 7, rank = from >>> 3;

      if (piece === WP || piece === BP) {
        const dir = side === WHITE ? 8 : -8;
        const startRank = side === WHITE ? 1 : 6;
        const promoRank = side === WHITE ? 6 : 1;
        const one = from + dir;
        if (!capturesOnly && b[one] === EMPTY) {
          if (rank === promoRank) count = addPromotions(out, count, from, one, piece, 0, side, FLAG_NONE);
          else {
            out[count++] = encodeMove(from, one, piece);
            const two = from + dir * 2;
            if (rank === startRank && b[two] === EMPTY) out[count++] = encodeMove(from, two, piece);
          }
        }
        const capA = side === WHITE ? 7 : -9, capB = side === WHITE ? 9 : -7;
        let to = from + capA;
        if (to >= 0 && to < 64 && Math.abs((to & 7) - file) === 1) {
          if (to === position.epSquare) out[count++] = encodeMove(from, to, piece, side === WHITE ? BP : WP, 0, FLAG_CAPTURE | FLAG_EP);
          else {
            const target = b[to];
            if (target && colorOf(target) !== side) count = rank === promoRank ? addPromotions(out, count, from, to, piece, target, side, FLAG_CAPTURE) : (out[count] = encodeMove(from, to, piece, target, 0, FLAG_CAPTURE), count + 1);
          }
        }
        to = from + capB;
        if (to >= 0 && to < 64 && Math.abs((to & 7) - file) === 1) {
          if (to === position.epSquare) out[count++] = encodeMove(from, to, piece, side === WHITE ? BP : WP, 0, FLAG_CAPTURE | FLAG_EP);
          else {
            const target = b[to];
            if (target && colorOf(target) !== side) count = rank === promoRank ? addPromotions(out, count, from, to, piece, target, side, FLAG_CAPTURE) : (out[count] = encodeMove(from, to, piece, target, 0, FLAG_CAPTURE), count + 1);
          }
        }
        continue;
      }

      if (piece === WN || piece === BN) {
        const base = from * 8;
        for (let i = 0, n = KNIGHT_COUNTS[from]; i < n; i++) {
          const to = KNIGHT_TARGETS[base + i], target = b[to];
          if (!target) { if (!capturesOnly) out[count++] = encodeMove(from, to, piece); }
          else if (colorOf(target) !== side) out[count++] = encodeMove(from, to, piece, target, 0, FLAG_CAPTURE);
        }
        continue;
      }

      if (piece === WK || piece === BK) {
        const base = from * 8;
        for (let i = 0, n = KING_COUNTS[from]; i < n; i++) {
          const to = KING_TARGETS[base + i], target = b[to];
          if (!target) { if (!capturesOnly) out[count++] = encodeMove(from, to, piece); }
          else if (colorOf(target) !== side) out[count++] = encodeMove(from, to, piece, target, 0, FLAG_CAPTURE);
        }
        if (capturesOnly) continue;
        if (side === WHITE && from === 4 && !position.inCheck(WHITE)) {
          if ((position.castling & CASTLE_WK) && !b[5] && !b[6] && b[7] === WR && !position.isSquareAttacked(5, 1) && !position.isSquareAttacked(6, 1)) out[count++] = encodeMove(4,6,WK,0,0,FLAG_CASTLE);
          if ((position.castling & CASTLE_WQ) && !b[1] && !b[2] && !b[3] && b[0] === WR && !position.isSquareAttacked(3, 1) && !position.isSquareAttacked(2, 1)) out[count++] = encodeMove(4,2,WK,0,0,FLAG_CASTLE);
        } else if (side !== WHITE && from === 60 && !position.inCheck(1)) {
          if ((position.castling & CASTLE_BK) && !b[61] && !b[62] && b[63] === BR && !position.isSquareAttacked(61, 0) && !position.isSquareAttacked(62, 0)) out[count++] = encodeMove(60,62,BK,0,0,FLAG_CASTLE);
          if ((position.castling & CASTLE_BQ) && !b[57] && !b[58] && !b[59] && b[56] === BR && !position.isSquareAttacked(59, 0) && !position.isSquareAttacked(58, 0)) out[count++] = encodeMove(60,58,BK,0,0,FLAG_CASTLE);
        }
        continue;
      }

      const startDir = (piece === WB || piece === BB) ? 0 : (piece === WR || piece === BR) ? 4 : 0;
      const endDir = (piece === WB || piece === BB) ? 4 : (piece === WR || piece === BR) ? 8 : 8;
      for (let d = startDir; d < endDir; d++) {
        const rayIndex = from * 8 + d, base = rayIndex * 7, n = RAY_COUNTS[rayIndex];
        for (let i = 0; i < n; i++) {
          const to = RAY_TARGETS[base + i], target = b[to];
          if (!target) { if (!capturesOnly) out[count++] = encodeMove(from, to, piece); }
          else { if (colorOf(target) !== side) out[count++] = encodeMove(from, to, piece, target, 0, FLAG_CAPTURE); break; }
        }
      }
    }
  }
  return count - offset;
}

const legalScratch = new Uint32Array(MAX_MOVES);
export function generateLegal(position) {
  const count = generatePseudoLegal(position, legalScratch);
  const result = [];
  for (let i = 0; i < count; i++) {
    const move = legalScratch[i];
    if (position.makeMove(move)) { position.unmakeMove(); result.push(move); }
  }
  return result;
}
