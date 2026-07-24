import { generatePseudoLegal, createMoveBuffer } from './movegen.js';
import { evaluate } from './evaluate.js';
import { moveCaptured, movePiece, moveFlags, moveFrom, moveTo, moveToUci } from './move.js';
import { FLAG_CAPTURE, FLAG_PROMOTION, MAX_MOVES, MAX_PLY, WP, BP, WN, WB, WR, WQ, BN, BB, BR, BQ } from './constants.js';
import { TranspositionTable, TT_EXACT, TT_ALPHA, TT_BETA } from './transposition.js';

const INF = 32000, MATE = 30000;
const PIECE_VALUE = new Int16Array([0,100,325,335,500,950,20000,100,325,335,500,950,20000]);

export class Search {
  constructor(hashBits = 20) {
    this.nodes = 0; this.stop = false; this.deadline = Infinity; this.bestMove = 0;
    this.moves = createMoveBuffer(MAX_PLY);
    this.scores = new Int32Array(MAX_PLY * MAX_MOVES);
    this.killers = new Uint32Array(MAX_PLY * 2);
    this.history = new Int32Array(2 * 64 * 64);
    this.tt = new TranspositionTable(hashBits);
  }

  shouldStop() {
    if (this.stop) return true;
    if ((this.nodes & 2047) === 0 && performance.now() >= this.deadline) this.stop = true;
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
      if (score >= beta) return beta;
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
      if (score >= beta) return beta;
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
          if (quiet) this.recordQuietCutoff(position, move, depth, ply);
          break;
        }
      }
    }
    if (!legal) return inCheck ? -MATE + ply : 0;

    const flag = best <= originalAlpha ? TT_ALPHA : best >= beta ? TT_BETA : TT_EXACT;
    this.tt.store(position.hashLo, position.hashHi, depth, best, flag, bestMove);
    return best;
  }

  rankRootMoves(position, maxDepth = 4, movetime = 0, limit = 8) {
    this.nodes = 0; this.stop = false; this.bestMove = 0;
    this.killers.fill(0); this.history.fill(0);
    this.deadline = movetime > 0 ? performance.now() + movetime : Infinity;
    this.tt.nextGeneration();

    const legalMoves = [];
    const pseudoCount = generatePseudoLegal(position, this.moves, 0);
    for (let i = 0; i < pseudoCount; i++) {
      const move = this.moves[i];
      if (!position.makeMove(move)) continue;
      const capture = Boolean(moveFlags(move) & FLAG_CAPTURE);
      const promotion = Boolean(moveFlags(move) & FLAG_PROMOTION);
      const check = position.inCheck();
      position.unmakeMove();
      legalMoves.push({ move, capture, promotion, check });
    }
    if (!legalMoves.length) return { candidates: [], depth: 0, nodes: this.nodes };

    let completedDepth = 0;
    let stable = legalMoves.map((entry) => ({ ...entry, score: 0 }));
    const targetDepth = Math.max(1, Math.min(maxDepth, 6));
    for (let depth = 1; depth <= targetDepth; depth++) {
      const current = [];
      for (const entry of legalMoves) {
        if (this.shouldStop()) break;
        if (!position.makeMove(entry.move)) continue;
        const score = -this.negamax(position, depth - 1, -INF, INF, 1, true);
        position.unmakeMove();
        if (this.stop) break;
        current.push({ ...entry, score });
      }
      if (this.stop || current.length !== legalMoves.length) break;
      current.sort((a, b) => b.score - a.score);
      stable = current;
      completedDepth = depth;
    }
    return { candidates: stable.slice(0, Math.max(1, Math.min(12, Number(limit) || 8))), depth: completedDepth, nodes: this.nodes };
  }

  iterative(position, maxDepth = 6, movetime = 0, info = console.log) {
    this.nodes = 0; this.stop = false; this.bestMove = 0;
    this.killers.fill(0); this.history.fill(0);
    this.deadline = movetime > 0 ? performance.now() + movetime : Infinity;
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
    return { move: this.bestMove || stableMove, score, depth: completed, nodes: this.nodes };
  }
}
