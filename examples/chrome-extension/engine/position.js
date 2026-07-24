import {
  WHITE, BLACK, EMPTY, WP, WK, BP, BK, WR, BR,
  CASTLE_WK, CASTLE_WQ, CASTLE_BK, CASTLE_BQ,
  NO_SQUARE, PIECE_FROM_CHAR, MAX_PLY
} from './constants.js';
import { moveFrom, moveTo, movePiece, moveCaptured, movePromotion, moveFlags } from './move.js';
import { FLAG_EP, FLAG_CASTLE, FLAG_PROMOTION } from './constants.js';
import { KNIGHT_TARGETS, KNIGHT_COUNTS, RAY_TARGETS, RAY_COUNTS } from './attacks.js';
import {
  computeHash, PIECE_KEYS_LO, PIECE_KEYS_HI, CASTLE_KEYS_LO, CASTLE_KEYS_HI,
  EP_KEYS_LO, EP_KEYS_HI, SIDE_KEY_LO, SIDE_KEY_HI
} from './hash.js';

export class Position {
  constructor(fen) {
    this.board = new Int8Array(64);
    this.side = WHITE;
    this.castling = 0;
    this.epSquare = NO_SQUARE;
    this.halfmove = 0;
    this.fullmove = 1;
    this.kingSquare = new Int8Array(2);
    this.pieceCount = new Uint8Array(13);
    this.pieceSquares = new Int8Array(13 * 16);
    this.squareListIndex = new Int8Array(64);
    this.occLo = new Uint32Array(2);
    this.occHi = new Uint32Array(2);
    this.hashLo = 0;
    this.hashHi = 0;
    this.ply = 0;
    this.undoMove = new Uint32Array(MAX_PLY);
    this.undoCastling = new Uint8Array(MAX_PLY);
    this.undoEp = new Int8Array(MAX_PLY);
    this.undoHalfmove = new Uint16Array(MAX_PLY);
    this.undoFullmove = new Uint16Array(MAX_PLY);
    this.undoKingW = new Int8Array(MAX_PLY);
    this.undoKingB = new Int8Array(MAX_PLY);
    this.undoEpCaptured = new Int8Array(MAX_PLY);
    this.undoHashLo = new Uint32Array(MAX_PLY);
    this.undoHashHi = new Uint32Array(MAX_PLY);
    if (fen) this.loadFen(fen);
  }

  _addPiece(square, piece) {
    this.board[square] = piece;
    const slot = this.pieceCount[piece]++;
    this.pieceSquares[piece * 16 + slot] = square;
    this.squareListIndex[square] = slot;
    const side = piece <= 6 ? WHITE : BLACK;
    if (square < 32) this.occLo[side] = (this.occLo[side] | (1 << square)) >>> 0;
    else this.occHi[side] = (this.occHi[side] | (1 << (square - 32))) >>> 0;
  }

  _removePiece(square, piece) {
    const slot = this.squareListIndex[square];
    const last = --this.pieceCount[piece];
    const lastSq = this.pieceSquares[piece * 16 + last];
    this.pieceSquares[piece * 16 + slot] = lastSq;
    this.squareListIndex[lastSq] = slot;
    this.board[square] = EMPTY;
    const side = piece <= 6 ? WHITE : BLACK;
    if (square < 32) this.occLo[side] = (this.occLo[side] & ~(1 << square)) >>> 0;
    else this.occHi[side] = (this.occHi[side] & ~(1 << (square - 32))) >>> 0;
  }

  _movePiece(from, to, piece) {
    const slot = this.squareListIndex[from];
    this.pieceSquares[piece * 16 + slot] = to;
    this.squareListIndex[to] = slot;
    this.board[from] = EMPTY; this.board[to] = piece;
    const side = piece <= 6 ? WHITE : BLACK;
    if (from < 32) this.occLo[side] = (this.occLo[side] & ~(1 << from)) >>> 0;
    else this.occHi[side] = (this.occHi[side] & ~(1 << (from - 32))) >>> 0;
    if (to < 32) this.occLo[side] = (this.occLo[side] | (1 << to)) >>> 0;
    else this.occHi[side] = (this.occHi[side] | (1 << (to - 32))) >>> 0;
  }

  loadFen(fen) {
    this.board.fill(EMPTY);
    this.pieceCount.fill(0); this.pieceSquares.fill(0); this.squareListIndex.fill(0);
    this.occLo.fill(0); this.occHi.fill(0);
    const parts = fen.trim().split(/\s+/);
    if (parts.length < 4) throw new Error(`Invalid FEN: ${fen}`);
    let rank = 7, file = 0;
    for (const ch of parts[0]) {
      if (ch === '/') { rank--; file = 0; continue; }
      if (ch >= '1' && ch <= '8') { file += ch.charCodeAt(0) - 48; continue; }
      const piece = PIECE_FROM_CHAR[ch];
      if (!piece || rank < 0 || file > 7) throw new Error(`Invalid FEN board: ${fen}`);
      const sq = rank * 8 + file++;
      this._addPiece(sq, piece);
      if (piece === WK) this.kingSquare[WHITE] = sq;
      else if (piece === BK) this.kingSquare[BLACK] = sq;
    }
    this.side = parts[1] === 'b' ? BLACK : WHITE;
    this.castling = 0;
    if (parts[2].includes('K')) this.castling |= CASTLE_WK;
    if (parts[2].includes('Q')) this.castling |= CASTLE_WQ;
    if (parts[2].includes('k')) this.castling |= CASTLE_BK;
    if (parts[2].includes('q')) this.castling |= CASTLE_BQ;
    this.epSquare = parts[3] === '-' ? NO_SQUARE : ((parts[3].charCodeAt(0) - 97) + (parts[3].charCodeAt(1) - 49) * 8);
    this.halfmove = Number(parts[4] ?? 0) | 0;
    this.fullmove = Number(parts[5] ?? 1) | 0;
    this.ply = 0;
    computeHash(this);
  }

  makeMove(move) {
    const ply = this.ply;
    if (ply >= MAX_PLY) return false;
    const from = moveFrom(move), to = moveTo(move), piece = movePiece(move);
    const captured = moveCaptured(move), promotion = movePromotion(move), flags = moveFlags(move);
    const oldSide = this.side;

    this.undoMove[ply] = move;
    this.undoCastling[ply] = this.castling;
    this.undoEp[ply] = this.epSquare;
    this.undoHalfmove[ply] = this.halfmove;
    this.undoFullmove[ply] = this.fullmove;
    this.undoKingW[ply] = this.kingSquare[WHITE];
    this.undoKingB[ply] = this.kingSquare[BLACK];
    this.undoEpCaptured[ply] = EMPTY;
    this.undoHashLo[ply] = this.hashLo;
    this.undoHashHi[ply] = this.hashHi;
    this.ply = ply + 1;

    let lo = this.hashLo ^ CASTLE_KEYS_LO[this.castling];
    let hi = this.hashHi ^ CASTLE_KEYS_HI[this.castling];
    if (this.epSquare >= 0) {
      const epFile = this.epSquare & 7;
      lo ^= EP_KEYS_LO[epFile]; hi ^= EP_KEYS_HI[epFile];
    }
    let index = piece * 64 + from;
    lo ^= PIECE_KEYS_LO[index]; hi ^= PIECE_KEYS_HI[index];
    if (captured && !(flags & FLAG_EP)) {
      index = captured * 64 + to;
      lo ^= PIECE_KEYS_LO[index]; hi ^= PIECE_KEYS_HI[index];
    }

    if (captured && !(flags & FLAG_EP)) this._removePiece(to, captured);
    const placed = (flags & FLAG_PROMOTION) ? promotion : piece;
    if (flags & FLAG_PROMOTION) { this._removePiece(from, piece); this._addPiece(to, placed); }
    else this._movePiece(from, to, piece);
    index = placed * 64 + to;
    lo ^= PIECE_KEYS_LO[index]; hi ^= PIECE_KEYS_HI[index];

    if (flags & FLAG_EP) {
      const capSq = oldSide === WHITE ? to - 8 : to + 8;
      const epCaptured = this.board[capSq];
      this.undoEpCaptured[ply] = epCaptured;
      this._removePiece(capSq, epCaptured);
      index = epCaptured * 64 + capSq;
      lo ^= PIECE_KEYS_LO[index]; hi ^= PIECE_KEYS_HI[index];
    }

    if (flags & FLAG_CASTLE) {
      let rookFrom, rookTo, rook;
      if (to === 6) { rookFrom = 7; rookTo = 5; rook = WR; }
      else if (to === 2) { rookFrom = 0; rookTo = 3; rook = WR; }
      else if (to === 62) { rookFrom = 63; rookTo = 61; rook = BR; }
      else { rookFrom = 56; rookTo = 59; rook = BR; }
      this._movePiece(rookFrom, rookTo, rook);
      index = rook * 64 + rookFrom; lo ^= PIECE_KEYS_LO[index]; hi ^= PIECE_KEYS_HI[index];
      index = rook * 64 + rookTo; lo ^= PIECE_KEYS_LO[index]; hi ^= PIECE_KEYS_HI[index];
    }

    if (piece === WK) this.kingSquare[WHITE] = to;
    else if (piece === BK) this.kingSquare[BLACK] = to;

    if (piece === WK) this.castling &= ~(CASTLE_WK | CASTLE_WQ);
    else if (piece === BK) this.castling &= ~(CASTLE_BK | CASTLE_BQ);
    if (from === 0 || to === 0) this.castling &= ~CASTLE_WQ;
    if (from === 7 || to === 7) this.castling &= ~CASTLE_WK;
    if (from === 56 || to === 56) this.castling &= ~CASTLE_BQ;
    if (from === 63 || to === 63) this.castling &= ~CASTLE_BK;

    this.epSquare = NO_SQUARE;
    if (piece === WP && to - from === 16) this.epSquare = from + 8;
    else if (piece === BP && from - to === 16) this.epSquare = from - 8;

    lo ^= CASTLE_KEYS_LO[this.castling]; hi ^= CASTLE_KEYS_HI[this.castling];
    if (this.epSquare >= 0) {
      const epFile = this.epSquare & 7;
      lo ^= EP_KEYS_LO[epFile]; hi ^= EP_KEYS_HI[epFile];
    }
    lo ^= SIDE_KEY_LO; hi ^= SIDE_KEY_HI;
    this.hashLo = lo >>> 0; this.hashHi = hi >>> 0;

    this.halfmove = (piece === WP || piece === BP || captured || (flags & FLAG_EP)) ? 0 : this.halfmove + 1;
    if (oldSide === BLACK) this.fullmove++;
    this.side ^= 1;

    if (this.isSquareAttacked(this.kingSquare[oldSide], this.side)) {
      this.unmakeMove();
      return false;
    }
    return true;
  }


  makeNullMove() {
    const ply = this.ply;
    if (ply >= MAX_PLY) return false;
    this.undoMove[ply] = 0;
    this.undoCastling[ply] = this.castling;
    this.undoEp[ply] = this.epSquare;
    this.undoHalfmove[ply] = this.halfmove;
    this.undoFullmove[ply] = this.fullmove;
    this.undoKingW[ply] = this.kingSquare[WHITE];
    this.undoKingB[ply] = this.kingSquare[BLACK];
    this.undoEpCaptured[ply] = EMPTY;
    this.undoHashLo[ply] = this.hashLo;
    this.undoHashHi[ply] = this.hashHi;
    this.ply = ply + 1;

    let lo = this.hashLo, hi = this.hashHi;
    if (this.epSquare >= 0) {
      const epFile = this.epSquare & 7;
      lo ^= EP_KEYS_LO[epFile]; hi ^= EP_KEYS_HI[epFile];
    }
    lo ^= SIDE_KEY_LO; hi ^= SIDE_KEY_HI;
    this.hashLo = lo >>> 0; this.hashHi = hi >>> 0;
    this.epSquare = NO_SQUARE;
    this.halfmove++;
    if (this.side === BLACK) this.fullmove++;
    this.side ^= 1;
    return true;
  }

  unmakeMove() {
    const ply = this.ply - 1;
    if (ply < 0) throw new Error('Unmake with empty history');
    this.ply = ply;
    const move = this.undoMove[ply];
    this.side ^= 1;
    if (move === 0) {
      this.castling = this.undoCastling[ply];
      this.epSquare = this.undoEp[ply];
      this.halfmove = this.undoHalfmove[ply];
      this.fullmove = this.undoFullmove[ply];
      this.kingSquare[WHITE] = this.undoKingW[ply];
      this.kingSquare[BLACK] = this.undoKingB[ply];
      this.hashLo = this.undoHashLo[ply];
      this.hashHi = this.undoHashHi[ply];
      return;
    }
    const from = moveFrom(move), to = moveTo(move), piece = movePiece(move);
    const captured = moveCaptured(move), flags = moveFlags(move);
    this.castling = this.undoCastling[ply];
    this.epSquare = this.undoEp[ply];
    this.halfmove = this.undoHalfmove[ply];
    this.fullmove = this.undoFullmove[ply];
    this.kingSquare[WHITE] = this.undoKingW[ply];
    this.kingSquare[BLACK] = this.undoKingB[ply];
    this.hashLo = this.undoHashLo[ply];
    this.hashHi = this.undoHashHi[ply];

    if (flags & FLAG_CASTLE) {
      if (to === 6) this._movePiece(5, 7, WR);
      else if (to === 2) this._movePiece(3, 0, WR);
      else if (to === 62) this._movePiece(61, 63, BR);
      else if (to === 58) this._movePiece(59, 56, BR);
    }
    if (flags & FLAG_PROMOTION) { this._removePiece(to, movePromotion(move)); this._addPiece(from, piece); }
    else this._movePiece(to, from, piece);
    if (flags & FLAG_EP) this._addPiece(this.side === WHITE ? to - 8 : to + 8, this.undoEpCaptured[ply]);
    else if (captured) this._addPiece(to, captured);
  }

  isSquareAttacked(square, bySide) {
    const b = this.board;
    const file = square & 7;
    if (bySide === WHITE) {
      if (file > 0 && square >= 9 && b[square - 9] === WP) return true;
      if (file < 7 && square >= 7 && b[square - 7] === WP) return true;
    } else {
      if (file > 0 && square <= 55 && b[square + 7] === BP) return true;
      if (file < 7 && square <= 54 && b[square + 9] === BP) return true;
    }

    const knight = bySide === WHITE ? 2 : 8;
    const base = square * 8;
    for (let i = 0, count = KNIGHT_COUNTS[square]; i < count; i++) if (b[KNIGHT_TARGETS[base + i]] === knight) return true;

    const bishop = bySide === WHITE ? 3 : 9, rook = bySide === WHITE ? 4 : 10;
    const queen = bySide === WHITE ? 5 : 11, king = bySide === WHITE ? 6 : 12;
    for (let d = 0; d < 8; d++) {
      const rayIndex = square * 8 + d, base = rayIndex * 7, n = RAY_COUNTS[rayIndex];
      for (let step = 0; step < n; step++) {
        const p = b[RAY_TARGETS[base + step]];
        if (!p) continue;
        if (step === 0 && p === king) return true;
        if (d < 4 ? (p === bishop || p === queen) : (p === rook || p === queen)) return true;
        break;
      }
    }
    return false;
  }

  inCheck(side = this.side) { return this.isSquareAttacked(this.kingSquare[side], side ^ 1); }
}
