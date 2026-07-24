const PIECE_KEYS_LO = new Uint32Array(13 * 64);
const PIECE_KEYS_HI = new Uint32Array(13 * 64);
const CASTLE_KEYS_LO = new Uint32Array(16);
const CASTLE_KEYS_HI = new Uint32Array(16);
const EP_KEYS_LO = new Uint32Array(8);
const EP_KEYS_HI = new Uint32Array(8);
let SIDE_KEY_LO = 0;
let SIDE_KEY_HI = 0;

let seed = 0x9e3779b9 >>> 0;
function random32() {
  seed ^= seed << 13;
  seed ^= seed >>> 17;
  seed ^= seed << 5;
  return seed >>> 0;
}

for (let i = 0; i < PIECE_KEYS_LO.length; i++) {
  PIECE_KEYS_LO[i] = random32();
  PIECE_KEYS_HI[i] = random32();
}
for (let i = 0; i < 16; i++) {
  CASTLE_KEYS_LO[i] = random32();
  CASTLE_KEYS_HI[i] = random32();
}
for (let i = 0; i < 8; i++) {
  EP_KEYS_LO[i] = random32();
  EP_KEYS_HI[i] = random32();
}
SIDE_KEY_LO = random32();
SIDE_KEY_HI = random32();

export function pieceKeyIndex(piece, square) { return piece * 64 + square; }
export { PIECE_KEYS_LO, PIECE_KEYS_HI, CASTLE_KEYS_LO, CASTLE_KEYS_HI, EP_KEYS_LO, EP_KEYS_HI, SIDE_KEY_LO, SIDE_KEY_HI };

export function computeHash(position) {
  let lo = 0, hi = 0;
  const board = position.board;
  for (let square = 0; square < 64; square++) {
    const piece = board[square];
    if (!piece) continue;
    const index = piece * 64 + square;
    lo ^= PIECE_KEYS_LO[index];
    hi ^= PIECE_KEYS_HI[index];
  }
  lo ^= CASTLE_KEYS_LO[position.castling];
  hi ^= CASTLE_KEYS_HI[position.castling];
  if (position.epSquare >= 0) {
    const file = position.epSquare & 7;
    lo ^= EP_KEYS_LO[file];
    hi ^= EP_KEYS_HI[file];
  }
  if (position.side) {
    lo ^= SIDE_KEY_LO;
    hi ^= SIDE_KEY_HI;
  }
  position.hashLo = lo >>> 0;
  position.hashHi = hi >>> 0;
}
