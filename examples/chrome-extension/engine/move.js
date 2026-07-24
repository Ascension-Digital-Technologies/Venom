import { FLAG_PROMOTION } from './constants.js';

export function encodeMove(from, to, piece, captured = 0, promotion = 0, flags = 0) {
  return (from | (to << 6) | (piece << 12) | (captured << 16) | (promotion << 20) | (flags << 24)) >>> 0;
}
export const moveFrom = move => move & 63;
export const moveTo = move => (move >>> 6) & 63;
export const movePiece = move => (move >>> 12) & 15;
export const moveCaptured = move => (move >>> 16) & 15;
export const movePromotion = move => (move >>> 20) & 15;
export const moveFlags = move => (move >>> 24) & 31;

export function squareName(square) {
  return String.fromCharCode(97 + (square & 7)) + String((square >>> 3) + 1);
}

export function moveToUci(move) {
  let text = squareName(moveFrom(move)) + squareName(moveTo(move));
  if (moveFlags(move) & FLAG_PROMOTION) {
    const p = movePromotion(move);
    text += p === 2 || p === 8 ? 'n' : p === 3 || p === 9 ? 'b' : p === 4 || p === 10 ? 'r' : 'q';
  }
  return text;
}
