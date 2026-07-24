export const WHITE = 0;
export const BLACK = 1;

export const EMPTY = 0;
export const WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6;
export const BP = 7, BN = 8, BB = 9, BR = 10, BQ = 11, BK = 12;

export const FLAG_NONE = 0;
export const FLAG_CAPTURE = 1;
export const FLAG_DOUBLE = 2;
export const FLAG_EP = 4;
export const FLAG_CASTLE = 8;
export const FLAG_PROMOTION = 16;

export const CASTLE_WK = 1;
export const CASTLE_WQ = 2;
export const CASTLE_BK = 4;
export const CASTLE_BQ = 8;

export const MAX_PLY = 256;
export const MAX_MOVES = 256;
export const NO_SQUARE = -1;

export const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

export const PIECE_FROM_CHAR = Object.freeze({
  P: WP, N: WN, B: WB, R: WR, Q: WQ, K: WK,
  p: BP, n: BN, b: BB, r: BR, q: BQ, k: BK
});

export const CHAR_FROM_PIECE = Object.freeze([
  '.', 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'
]);

export function colorOf(piece) {
  if (piece >= WP && piece <= WK) return WHITE;
  if (piece >= BP && piece <= BK) return BLACK;
  return -1;
}

export function typeOf(piece) {
  return piece > 6 ? piece - 6 : piece;
}
