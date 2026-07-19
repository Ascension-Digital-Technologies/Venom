// @venom: browser
"use strict";

// @venom: protected isolated
function runChessEngine(request) {
  /*
   * Velocity Chess v0.5.0 integration for Venom Protected Chess.
   * All rules, validation, move generation, evaluation, hashing, search,
   * game-state decisions, and move notation remain inside this protected export.
   */
  const VELOCITY_ENGINE_VERSION = "0.5.0";
  const VELOCITY_BRIDGE_VERSION = "3.0.0";

  function velocityNowMs() {
    if (typeof performance !== "undefined" && performance && typeof performance.now === "function") {
      return performance.now();
    }
    return Date.now();
  }

  const WHITE = 0;
  const BLACK = 1;

  const EMPTY = 0;
  const WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6;
  const BP = 7, BN = 8, BB = 9, BR = 10, BQ = 11, BK = 12;

  const FLAG_NONE = 0;
  const FLAG_CAPTURE = 1;
  const FLAG_DOUBLE = 2;
  const FLAG_EP = 4;
  const FLAG_CASTLE = 8;
  const FLAG_PROMOTION = 16;

  const CASTLE_WK = 1;
  const CASTLE_WQ = 2;
  const CASTLE_BK = 4;
  const CASTLE_BQ = 8;

  const MAX_PLY = 256;
  const MAX_MOVES = 256;
  const NO_SQUARE = -1;

  const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

  const PIECE_FROM_CHAR = Object.freeze({
    P: WP, N: WN, B: WB, R: WR, Q: WQ, K: WK,
    p: BP, n: BN, b: BB, r: BR, q: BQ, k: BK
  });

  const CHAR_FROM_PIECE = Object.freeze([
    '.', 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'
  ]);

  function colorOf(piece) {
    if (piece >= WP && piece <= WK) return WHITE;
    if (piece >= BP && piece <= BK) return BLACK;
    return -1;
  }

  function typeOf(piece) {
    return piece > 6 ? piece - 6 : piece;
  }


  function encodeMove(from, to, piece, captured = 0, promotion = 0, flags = 0) {
    return (from | (to << 6) | (piece << 12) | (captured << 16) | (promotion << 20) | (flags << 24)) >>> 0;
  }
  const moveFrom = move => move & 63;
  const moveTo = move => (move >>> 6) & 63;
  const movePiece = move => (move >>> 12) & 15;
  const moveCaptured = move => (move >>> 16) & 15;
  const movePromotion = move => (move >>> 20) & 15;
  const moveFlags = move => (move >>> 24) & 31;

  function squareName(square) {
    return String.fromCharCode(97 + (square & 7)) + String((square >>> 3) + 1);
  }

  function moveToUci(move) {
    let text = squareName(moveFrom(move)) + squareName(moveTo(move));
    if (moveFlags(move) & FLAG_PROMOTION) {
      const p = movePromotion(move);
      text += p === 2 || p === 8 ? 'n' : p === 3 || p === 9 ? 'b' : p === 4 || p === 10 ? 'r' : 'q';
    }
    return text;
  }


  const KNIGHT_DF = new Int8Array([-2,-1,1,2,2,1,-1,-2]);
  const KNIGHT_DR = new Int8Array([1,2,2,1,-1,-2,-2,-1]);
  const KING_DF = new Int8Array([-1,0,1,-1,1,-1,0,1]);
  const KING_DR = new Int8Array([-1,-1,-1,0,0,1,1,1]);

  const KNIGHT_TARGETS = new Int8Array(64 * 8);
  const KNIGHT_COUNTS = new Uint8Array(64);
  const KING_TARGETS = new Int8Array(64 * 8);
  const KING_COUNTS = new Uint8Array(64);

  function initLeapers(targets, counts, dfs, drs) {
    for (let square = 0; square < 64; square++) {
      const file = square & 7, rank = square >>> 3;
      let count = 0;
      for (let i = 0; i < dfs.length; i++) {
        const nf = file + dfs[i], nr = rank + drs[i];
        if ((nf & ~7) !== 0 || (nr & ~7) !== 0) continue;
        targets[square * 8 + count++] = nr * 8 + nf;
      }
      counts[square] = count;
    }
  }

  initLeapers(KNIGHT_TARGETS, KNIGHT_COUNTS, KNIGHT_DF, KNIGHT_DR);
  initLeapers(KING_TARGETS, KING_COUNTS, KING_DF, KING_DR);

  const DIR_DF = new Int8Array([1,-1,1,-1,1,-1,0,0]);
  const DIR_DR = new Int8Array([1,1,-1,-1,0,0,1,-1]);

  // Dense ray table: [square][direction][step]. A count array avoids sentinels.
  const RAY_TARGETS = new Int8Array(64 * 8 * 7);
  const RAY_COUNTS = new Uint8Array(64 * 8);
  for (let square = 0; square < 64; square++) {
    const file = square & 7, rank = square >>> 3;
    for (let d = 0; d < 8; d++) {
      let f = file + DIR_DF[d], r = rank + DIR_DR[d], n = 0;
      const base = (square * 8 + d) * 7;
      while ((f & ~7) === 0 && (r & ~7) === 0) {
        RAY_TARGETS[base + n++] = r * 8 + f;
        f += DIR_DF[d]; r += DIR_DR[d];
      }
      RAY_COUNTS[square * 8 + d] = n;
    }
  }


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

  function pieceKeyIndex(piece, square) { return piece * 64 + square; }
  { PIECE_KEYS_LO, PIECE_KEYS_HI, CASTLE_KEYS_LO, CASTLE_KEYS_HI, EP_KEYS_LO, EP_KEYS_HI, SIDE_KEY_LO, SIDE_KEY_HI };

  function computeHash(position) {
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


  const TT_EXACT = 0;
  const TT_ALPHA = 1;
  const TT_BETA = 2;

  class TranspositionTable {
    constructor(bits = 20) {
      this.size = 1 << bits;
      this.mask = this.size - 1;
      this.keyLo = new Uint32Array(this.size);
      this.keyHi = new Uint32Array(this.size);
      this.move = new Uint32Array(this.size);
      this.score = new Int16Array(this.size);
      this.depth = new Int8Array(this.size);
      this.flag = new Uint8Array(this.size);
      this.age = new Uint8Array(this.size);
      this.generation = 1;
    }

    clear() {
      this.keyLo.fill(0); this.keyHi.fill(0); this.move.fill(0);
      this.score.fill(0); this.depth.fill(0); this.flag.fill(0); this.age.fill(0);
    }

    nextGeneration() { this.generation = (this.generation + 1) & 255 || 1; }

    probe(lo, hi) {
      const index = lo & this.mask;
      return this.keyLo[index] === lo && this.keyHi[index] === hi ? index : -1;
    }

    store(lo, hi, depth, score, flag, move) {
      const index = lo & this.mask;
      const replace = this.keyLo[index] !== lo || this.keyHi[index] !== hi || depth >= this.depth[index] || this.age[index] !== this.generation;
      if (!replace) return;
      this.keyLo[index] = lo;
      this.keyHi[index] = hi;
      this.depth[index] = depth;
      this.score[index] = score;
      this.flag[index] = flag;
      this.move[index] = move >>> 0;
      this.age[index] = this.generation;
    }
  }


  class Position {
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


  function createMoveBuffer(plies = 1) { return new Uint32Array(plies * MAX_MOVES); }

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

  function generatePseudoLegal(position, out, offset = 0, capturesOnly = false) {
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
  function generateLegal(position) {
    const count = generatePseudoLegal(position, legalScratch);
    const result = [];
    for (let i = 0; i < count; i++) {
      const move = legalScratch[i];
      if (position.makeMove(move)) { position.unmakeMove(); result.push(move); }
    }
    return result;
  }


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

  function evaluate(position) {
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


  const moveBuffer = createMoveBuffer(MAX_PLY);

  function perft(position, depth, ply = 0) {
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


  const INF = 32000, MATE = 30000;
  const PIECE_VALUE = new Int16Array([0,100,325,335,500,950,20000,100,325,335,500,950,20000]);

  class Search {
    constructor(hashBits = 20) {
      this.nodes = 0; this.qnodes = 0; this.ttHits = 0; this.ttStores = 0; this.cutoffs = 0; this.stop = false; this.deadline = Infinity; this.bestMove = 0;
      this.moves = createMoveBuffer(MAX_PLY);
      this.scores = new Int32Array(MAX_PLY * MAX_MOVES);
      this.killers = new Uint32Array(MAX_PLY * 2);
      this.history = new Int32Array(2 * 64 * 64);
      this.tt = new TranspositionTable(hashBits);
    }

    shouldStop() {
      if (this.stop) return true;
      if ((this.nodes & 2047) === 0 && velocityNowMs() >= this.deadline) this.stop = true;
      return this.stop;
    }

    hasNonPawnMaterial(position) {
      const b = position.board, side = position.side;
      const min = side === 0 ? WN : BN, max = side === 0 ? WQ : BQ;
      for (let sq = 0; sq < 64; sq++) if (b[sq] >= min && b[sq] <= max) return true;
      return false;
    }

    scoreAndSort(position, offset, count, ttMove, ply, capturesOnly = false) {
      const scores = this.scores, sideBase = position.side * 4096;
      const killer0 = this.killers[ply * 2], killer1 = this.killers[ply * 2 + 1];
      for (let i = 0; i < count; i++) {
        const move = this.moves[offset + i];
        let score = move === ttMove ? 2_000_000_000 : 0;
        if (moveFlags(move) & FLAG_CAPTURE) score += 1_000_000 + PIECE_VALUE[moveCaptured(move)] * 32 - PIECE_VALUE[movePiece(move)];
        else if (capturesOnly) score = -1;
        else if (move === killer0) score += 900_000;
        else if (move === killer1) score += 800_000;
        else score += this.history[sideBase + moveFrom(move) * 64 + moveTo(move)];
        scores[offset + i] = score;
      }
      for (let i = 1; i < count; i++) {
        const move = this.moves[offset + i], score = scores[offset + i];
        let j = i - 1;
        while (j >= 0 && scores[offset + j] < score) {
          this.moves[offset + j + 1] = this.moves[offset + j];
          scores[offset + j + 1] = scores[offset + j];
          j--;
        }
        this.moves[offset + j + 1] = move;
        scores[offset + j + 1] = score;
      }
    }

    recordQuietCutoff(position, move, depth, ply) {
      const k = ply * 2;
      if (this.killers[k] !== move) { this.killers[k + 1] = this.killers[k]; this.killers[k] = move; }
      const index = position.side * 4096 + moveFrom(move) * 64 + moveTo(move);
      let value = this.history[index] + depth * depth;
      if (value > 1_000_000) { for (let i = 0; i < this.history.length; i++) this.history[i] >>= 1; value >>= 1; }
      this.history[index] = value;
    }

    captureGain(move) {
      return PIECE_VALUE[moveCaptured(move)] + ((moveFlags(move) & FLAG_PROMOTION) ? PIECE_VALUE[movePiece(move) > 6 ? 11 : 5] - 100 : 0) - PIECE_VALUE[movePiece(move)];
    }

    quiescence(position, alpha, beta, ply) {
      if (this.shouldStop()) return 0;
      this.nodes++;
      this.qnodes++;
      if (ply >= MAX_PLY - 1) return evaluate(position);
      const inCheck = position.inCheck();
      if (!inCheck) {
        const stand = evaluate(position);
        if (stand >= beta) return beta;
        if (stand > alpha) alpha = stand;
      }
      const offset = ply * MAX_MOVES;
      const count = generatePseudoLegal(position, this.moves, offset, !inCheck);
      this.scoreAndSort(position, offset, count, 0, ply, !inCheck);
      let legal = 0;
      for (let i = 0; i < count; i++) {
        const move = this.moves[offset + i];
        if (!inCheck && (moveFlags(move) & FLAG_CAPTURE) && this.captureGain(move) < -120) continue;
        if (!position.makeMove(move)) continue;
        legal++;
        const score = -this.quiescence(position, -beta, -alpha, ply + 1);
        position.unmakeMove();
        if (this.stop) return 0;
        if (score >= beta) { this.cutoffs++; return beta; }
        if (score > alpha) alpha = score;
      }
      if (inCheck && legal === 0) return -MATE + ply;
      return alpha;
    }

    negamax(position, depth, alpha, beta, ply, allowNull = true) {
      if (this.shouldStop()) return 0;
      this.nodes++;
      if (depth <= 0) return this.quiescence(position, alpha, beta, ply);
      if (position.halfmove >= 100) return 0;
      if (ply >= MAX_PLY - 1) return evaluate(position);

      const originalAlpha = alpha;
      const pvNode = beta - alpha > 1;
      const inCheck = position.inCheck();
      if (inCheck) depth++;

      const ttIndex = this.tt.probe(position.hashLo, position.hashHi);
      let ttMove = 0;
      if (ttIndex >= 0) {
        this.ttHits++;
        ttMove = this.tt.move[ttIndex];
        if (!pvNode && this.tt.depth[ttIndex] >= depth) {
          const ttScore = this.tt.score[ttIndex], flag = this.tt.flag[ttIndex];
          if (flag === TT_EXACT) return ttScore;
          if (flag === TT_ALPHA && ttScore <= alpha) return ttScore;
          if (flag === TT_BETA && ttScore >= beta) return ttScore;
        }
      }

      if (allowNull && !pvNode && !inCheck && depth >= 3 && this.hasNonPawnMaterial(position)) {
        const reduction = depth >= 7 ? 3 : 2;
        position.makeNullMove();
        const score = -this.negamax(position, depth - 1 - reduction, -beta, -beta + 1, ply + 1, false);
        position.unmakeMove();
        if (this.stop) return 0;
        if (score >= beta) { this.cutoffs++; return beta; }
      }

      const offset = ply * MAX_MOVES;
      const count = generatePseudoLegal(position, this.moves, offset);
      this.scoreAndSort(position, offset, count, ttMove, ply);
      let legal = 0, best = -INF, bestMove = 0;
      for (let i = 0; i < count; i++) {
        const move = this.moves[offset + i];
        if (!pvNode && depth <= 3 && (moveFlags(move) & FLAG_CAPTURE) && this.captureGain(move) < -180 * depth) continue;
        if (!position.makeMove(move)) continue;
        legal++;

        let score;
        const quiet = !(moveFlags(move) & (FLAG_CAPTURE | FLAG_PROMOTION));
        let reduction = 0;
        if (depth >= 3 && legal > 3 && quiet && !inCheck) reduction = 1 + (depth >= 6 && legal > 8 ? 1 : 0);

        if (legal === 1) {
          score = -this.negamax(position, depth - 1, -beta, -alpha, ply + 1, true);
        } else {
          score = -this.negamax(position, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, true);
          if (score > alpha && reduction) score = -this.negamax(position, depth - 1, -alpha - 1, -alpha, ply + 1, true);
          if (score > alpha && score < beta) score = -this.negamax(position, depth - 1, -beta, -alpha, ply + 1, true);
        }
        position.unmakeMove();
        if (this.stop) return 0;
        if (score > best) { best = score; bestMove = move; }
        if (score > alpha) {
          alpha = score;
          if (ply === 0) this.bestMove = move;
          if (alpha >= beta) {
            this.cutoffs++;
            if (quiet) this.recordQuietCutoff(position, move, depth, ply);
            break;
          }
        }
      }
      if (!legal) return inCheck ? -MATE + ply : 0;

      const flag = best <= originalAlpha ? TT_ALPHA : best >= beta ? TT_BETA : TT_EXACT;
      this.tt.store(position.hashLo, position.hashHi, depth, best, flag, bestMove);
      this.ttStores++;
      return best;
    }

    iterative(position, maxDepth = 6, movetime = 0, info = console.log) {
      this.nodes = 0; this.qnodes = 0; this.ttHits = 0; this.ttStores = 0; this.cutoffs = 0; this.stop = false; this.bestMove = 0;
      this.killers.fill(0); this.history.fill(0);
      this.deadline = movetime > 0 ? velocityNowMs() + movetime : Infinity;
      this.tt.nextGeneration();
      let score = 0, completed = 0, stableMove = 0;
      for (let depth = 1; depth <= maxDepth; depth++) {
        let window = depth >= 4 ? 30 : INF;
        let alpha = window === INF ? -INF : score - window;
        let beta = window === INF ? INF : score + window;
        for (;;) {
          const candidate = this.negamax(position, depth, alpha, beta, 0, true);
          if (this.stop) break;
          score = candidate;
          if (score <= alpha) { window *= 2; alpha = Math.max(-INF, score - window); continue; }
          if (score >= beta) { window *= 2; beta = Math.min(INF, score + window); continue; }
          break;
        }
        if (this.stop) { this.bestMove = stableMove; break; }
        completed = depth; stableMove = this.bestMove;
        info(`info depth ${depth} score cp ${score} nodes ${this.nodes} pv ${this.bestMove ? moveToUci(this.bestMove) : ''}`);
      }
      return { move: this.bestMove || stableMove, score, depth: completed, nodes: this.nodes, qnodes: this.qnodes, ttHits: this.ttHits, ttStores: this.ttStores, cutoffs: this.cutoffs, timedOut: this.stop };
    }
  }

  function createPosition(fen) {
    const text = typeof fen === 'string' && fen.trim() ? fen.trim() : START_FEN;
    const position = new Position(text);
    if (position.pieceCount[WK] !== 1 || position.pieceCount[BK] !== 1) throw new Error('invalid chess position: both kings are required');
    return position;
  }

  function positionToFen(position) {
    const ranks = [];
    for (let rank = 7; rank >= 0; rank--) {
      let empty = 0, row = '';
      for (let file = 0; file < 8; file++) {
        const piece = position.board[rank * 8 + file];
        if (!piece) { empty++; continue; }
        if (empty) { row += String(empty); empty = 0; }
        row += CHAR_FROM_PIECE[piece];
      }
      if (empty) row += String(empty);
      ranks.push(row);
    }
    let castle = '';
    if (position.castling & CASTLE_WK) castle += 'K';
    if (position.castling & CASTLE_WQ) castle += 'Q';
    if (position.castling & CASTLE_BK) castle += 'k';
    if (position.castling & CASTLE_BQ) castle += 'q';
    if (!castle) castle = '-';
    const ep = position.epSquare >= 0 ? squareName(position.epSquare) : '-';
    return ranks.join('/') + ' ' + (position.side === WHITE ? 'w' : 'b') + ' ' + castle + ' ' + ep + ' ' + position.halfmove + ' ' + position.fullmove;
  }

  function repetitionKey(position) {
    return positionToFen(position).split(/\s+/).slice(0, 4).join(' ');
  }

  function normalizeRepetition(input, position) {
    const values = Array.isArray(input) ? input.filter(function (value) { return typeof value === 'string'; }).slice(-255) : [];
    const current = repetitionKey(position);
    if (!values.length || values[values.length - 1] !== current) values.push(current);
    return values;
  }

  function isThreefold(position, repetitions) {
    const key = repetitionKey(position);
    let count = 0;
    for (let i = 0; i < repetitions.length; i++) if (repetitions[i] === key && ++count >= 3) return true;
    return false;
  }

  function insufficientMaterial(position) {
    if (position.pieceCount[WP] || position.pieceCount[BP] || position.pieceCount[WR] || position.pieceCount[BR] || position.pieceCount[WQ] || position.pieceCount[BQ]) return false;
    const knights = position.pieceCount[WN] + position.pieceCount[BN];
    const bishops = position.pieceCount[WB] + position.pieceCount[BB];
    if (knights + bishops <= 1) return true;
    if (knights) return false;
    let squareColor = -1;
    const bishopPieces = [WB, BB];
    for (let p = 0; p < bishopPieces.length; p++) {
      const piece = bishopPieces[p], base = piece * 16, count = position.pieceCount[piece];
      for (let i = 0; i < count; i++) {
        const square = position.pieceSquares[base + i];
        const color = ((square & 7) + (square >>> 3)) & 1;
        if (squareColor < 0) squareColor = color;
        else if (squareColor !== color) return false;
      }
    }
    return true;
  }

  function whiteStaticEvaluation(position) {
    const score = evaluate(position);
    return position.side === WHITE ? score : -score;
  }

  function normalizeMateScore(score) {
    if (score >= 29000) return 1000000 - (30000 - score);
    if (score <= -29000) return -1000000 + (30000 + score);
    return score;
  }

  function whiteSearchScore(position, score) {
    const normalized = normalizeMateScore(score);
    return position.side === WHITE ? normalized : -normalized;
  }

  function squareIndex(name) {
    if (typeof name !== 'string' || !/^[a-h][1-8]$/.test(name)) return -1;
    return (name.charCodeAt(0) - 97) + (name.charCodeAt(1) - 49) * 8;
  }

  function promotionCode(side, value) {
    const ch = String(value || 'q').toLowerCase();
    if (side === WHITE) return ch === 'n' ? WN : ch === 'b' ? WB : ch === 'r' ? WR : WQ;
    return ch === 'n' ? BN : ch === 'b' ? BB : ch === 'r' ? BR : BQ;
  }

  function requestedMoveParts(value) {
    if (typeof value === 'string') {
      const text = value.trim().toLowerCase();
      if (!/^[a-h][1-8][a-h][1-8][qrbn]?$/.test(text)) return null;
      return { from: text.slice(0, 2), to: text.slice(2, 4), promotion: text.slice(4) || 'q' };
    }
    if (!value || typeof value !== 'object') return null;
    return { from: String(value.from || '').toLowerCase(), to: String(value.to || '').toLowerCase(), promotion: String(value.promotion || 'q').toLowerCase() };
  }

  function findRequestedLegalMove(position, value) {
    const parts = requestedMoveParts(value);
    if (!parts) return 0;
    const from = squareIndex(parts.from), to = squareIndex(parts.to);
    if (from < 0 || to < 0) return 0;
    const legal = generateLegal(position);
    for (let i = 0; i < legal.length; i++) {
      const move = legal[i];
      if (moveFrom(move) !== from || moveTo(move) !== to) continue;
      if (moveFlags(move) & FLAG_PROMOTION) {
        if (movePromotion(move) !== promotionCode(position.side, parts.promotion)) continue;
      }
      return move;
    }
    return 0;
  }

  function samePackedMove(a, b) {
    return (a >>> 0) === (b >>> 0);
  }

  function moveSan(position, move) {
    const piece = movePiece(move), type = typeOf(piece), from = moveFrom(move), to = moveTo(move), flags = moveFlags(move);
    let text = '';
    if (flags & FLAG_CASTLE) {
      text = to > from ? 'O-O' : 'O-O-O';
    } else {
      const pieceLetter = type === 2 ? 'N' : type === 3 ? 'B' : type === 4 ? 'R' : type === 5 ? 'Q' : type === 6 ? 'K' : '';
      text += pieceLetter;
      if (type !== 1) {
        const legal = generateLegal(position);
        let sameFile = false, sameRank = false, ambiguous = false;
        for (let i = 0; i < legal.length; i++) {
          const other = legal[i];
          if (samePackedMove(other, move) || moveTo(other) !== to || typeOf(movePiece(other)) !== type) continue;
          ambiguous = true;
          if ((moveFrom(other) & 7) === (from & 7)) sameFile = true;
          if ((moveFrom(other) >>> 3) === (from >>> 3)) sameRank = true;
        }
        if (ambiguous) {
          if (!sameFile) text += String.fromCharCode(97 + (from & 7));
          else if (!sameRank) text += String((from >>> 3) + 1);
          else text += squareName(from);
        }
      } else if (flags & FLAG_CAPTURE) {
        text += String.fromCharCode(97 + (from & 7));
      }
      if (flags & FLAG_CAPTURE) text += 'x';
      text += squareName(to);
      if (flags & FLAG_PROMOTION) {
        const promoted = typeOf(movePromotion(move));
        text += '=' + (promoted === 2 ? 'N' : promoted === 3 ? 'B' : promoted === 4 ? 'R' : 'Q');
      }
    }
    if (!position.makeMove(move)) throw new Error('cannot format illegal move');
    const check = position.inCheck();
    const replies = generateLegal(position);
    position.unmakeMove();
    if (check) text += replies.length ? '+' : '#';
    return text;
  }

  function publicMove(position, move, san) {
    if (!move) return null;
    const promotion = moveFlags(move) & FLAG_PROMOTION ? moveToUci(move).slice(4) : null;
    return {
      from: squareName(moveFrom(move)),
      to: squareName(moveTo(move)),
      promotion: promotion,
      san: san || moveSan(position, move),
      uci: moveToUci(move)
    };
  }

  function snapshot(position, history, repetitions, forcedEvaluation) {
    const legal = generateLegal(position);
    const inCheck = position.inCheck();
    const checkmate = inCheck && legal.length === 0;
    const stalemate = !inCheck && legal.length === 0;
    const insufficient = insufficientMaterial(position);
    const threefold = isThreefold(position, repetitions);
    const fiftyMove = position.halfmove >= 100;
    const draw = stalemate || insufficient || threefold || fiftyMove;
    let evaluation = typeof forcedEvaluation === 'number' ? forcedEvaluation : whiteStaticEvaluation(position);
    if (checkmate) evaluation = position.side === WHITE ? -1000000 : 1000000;
    return {
      fen: positionToFen(position),
      turn: position.side === WHITE ? 'w' : 'b',
      gameOver: checkmate || draw,
      inCheck: inCheck,
      checkmate: checkmate,
      stalemate: stalemate,
      draw: draw,
      fiftyMoveRule: fiftyMove,
      insufficientMaterial: insufficient,
      threefoldRepetition: threefold,
      evaluation: evaluation,
      history: Array.isArray(history) ? history.slice(-512) : [],
      repetition: repetitions.slice(-256)
    };
  }

  function getSearchEngine() {
    const root = typeof globalThis !== 'undefined' ? globalThis : this;
    const key = '__venomVelocityChessSearch050';
    const cached = root[key];
    if (cached && cached.version === VELOCITY_ENGINE_VERSION && cached.search) return cached.search;
    // 17 bits keeps a persistent typed-array TT inside Venom's default 8 MiB QuickJS heap.
    const search = new Search(17);
    root[key] = { version: VELOCITY_ENGINE_VERSION, search: search };
    return search;
  }

  function buildPrincipalVariation(fen, search, maxLength) {
    const line = createPosition(fen);
    const pv = [];
    for (let ply = 0; ply < maxLength; ply++) {
      const index = search.tt.probe(line.hashLo, line.hashHi);
      if (index < 0) break;
      const stored = search.tt.move[index];
      const legal = generateLegal(line);
      let chosen = 0;
      for (let i = 0; i < legal.length; i++) if (samePackedMove(legal[i], stored)) { chosen = legal[i]; break; }
      if (!chosen) break;
      const san = moveSan(line, chosen);
      pv.push(san);
      if (!line.makeMove(chosen)) break;
      if (generateLegal(line).length === 0) break;
    }
    return pv;
  }

  function runProtectedSearch(position, requestValue) {
    const maxDepthRaw = Number(requestValue.maxDepth || requestValue.depth || 4);
    const timeRaw = Number(requestValue.timeMs || requestValue.movetime || 1500);
    const maxDepth = Math.max(1, Math.min(16, Number.isFinite(maxDepthRaw) ? Math.floor(maxDepthRaw) : 4));
    const timeMs = Math.max(25, Math.min(30000, Number.isFinite(timeRaw) ? Math.floor(timeRaw) : 1500));
    const search = getSearchEngine();
    const rootFen = positionToFen(position);
    const rootTurn = position.side;
    const started = velocityNowMs();
    const result = search.iterative(position, maxDepth, timeMs, function () {});
    const elapsedMs = Math.max(0, velocityNowMs() - started);
    let best = result.move;
    if (!best) {
      const legal = generateLegal(position);
      best = legal.length ? legal[0] : 0;
    }
    const value = whiteSearchScore(position, result.score);
    return {
      packedMove: best,
      move: best ? publicMove(position, best) : null,
      value: value,
      rootScore: normalizeMateScore(result.score),
      rootTurn: rootTurn === WHITE ? 'w' : 'b',
      depth: result.depth,
      completedDepth: result.depth,
      requestedDepth: maxDepth,
      timedOut: Boolean(result.timedOut),
      elapsedMs: Math.round(elapsedMs * 1000) / 1000,
      positions: result.nodes,
      qnodes: result.qnodes || 0,
      ttHits: result.ttHits || 0,
      ttStores: result.ttStores || 0,
      cutoffs: result.cutoffs || 0,
      positionsPerSecond: elapsedMs > 0 ? Math.round(result.nodes * 1000 / elapsedMs) : result.nodes,
      pv: buildPrincipalVariation(rootFen, search, Math.max(1, result.depth))
    };
  }

  const action = request && request.action;
  if (action === 'identity') {
    return {
      name: 'Velocity Chess',
      version: VELOCITY_ENGINE_VERSION,
      bridgeVersion: VELOCITY_BRIDGE_VERSION,
      runtime: 'QuickJS / WASM',
      isolation: 'worker',
      bridge: 'json-value-v1',
      protected: true,
      scoreConvention: 'positive-white-centipawns',
      capabilities: [
        'packed-32-bit-moves', 'incremental-piece-lists', 'incremental-occupancy',
        'incremental-zobrist-hashing', 'legal-move-generation', 'full-board-evaluation',
        'tapered-evaluation', 'iterative-deepening', 'aspiration-windows',
        'alpha-beta', 'principal-variation-search', 'quiescence-search',
        'null-move-pruning', 'late-move-reductions', 'check-extensions',
        'transposition-table', 'killer-history-ordering', 'time-management',
        'principal-variation', 'san-notation', 'draw-detection', 'perft'
      ]
    };
  }

  if (!action) throw new Error('chess engine action is required');
  const position = createPosition(request && request.fen);
  const history = Array.isArray(request && request.history) ? request.history.slice(-511) : [];
  const repetitions = normalizeRepetition(request && request.repetition, position);

  if (action === 'state') return { state: snapshot(position, history, repetitions) };

  if (action === 'moves') {
    const square = squareIndex(String(request.square || '').toLowerCase());
    const legal = generateLegal(position);
    const moves = [];
    for (let i = 0; i < legal.length; i++) {
      const move = legal[i];
      if (square >= 0 && moveFrom(move) !== square) continue;
      moves.push(publicMove(position, move));
    }
    return { moves: moves };
  }

  if (action === 'perft') {
    const depthRaw = Number(request.depth || 0);
    const depth = Math.max(0, Math.min(6, Number.isFinite(depthRaw) ? Math.floor(depthRaw) : 0));
    const started = velocityNowMs();
    const nodes = perft(position, depth);
    return { depth: depth, nodes: nodes, elapsedMs: Math.max(0, velocityNowMs() - started) };
  }

  if (action === 'evaluate') {
    const value = whiteStaticEvaluation(position);
    return { value: value, state: snapshot(position, history, repetitions, value) };
  }

  if (action === 'move') {
    const move = findRequestedLegalMove(position, request.move);
    if (!move) throw new Error('engine received an illegal move');
    const san = moveSan(position, move);
    const visibleMove = publicMove(position, move, san);
    if (!position.makeMove(move)) throw new Error('engine failed to apply a legal move');
    history.push(san);
    repetitions.push(repetitionKey(position));
    const state = snapshot(position, history, repetitions);
    return { move: visibleMove, value: state.evaluation, state: state };
  }

  if (action !== 'search') throw new Error('unsupported chess engine action');
  const searchResult = runProtectedSearch(position, request || {});
  if (request && request.play && searchResult.packedMove) {
    const san = searchResult.move.san;
    if (!position.makeMove(searchResult.packedMove)) throw new Error('search returned an illegal move');
    history.push(san);
    repetitions.push(repetitionKey(position));
    searchResult.state = snapshot(position, history, repetitions, searchResult.value);
  }
  delete searchResult.packedMove;
  return searchResult;
}
