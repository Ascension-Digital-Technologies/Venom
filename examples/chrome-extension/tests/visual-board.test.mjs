import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../visual-board.js', import.meta.url), 'utf8');
const context = vm.createContext({ console });
vm.runInContext(source, context, { filename: 'visual-board.js' });
const visual = context.VelocityVisualBoardV090;

function boardFromPlacement(placement) {
  const board = {};
  const ranks = placement.split('/');
  for (let row = 0; row < 8; row++) {
    let file = 0;
    for (const token of ranks[row]) {
      if (/\d/.test(token)) file += Number(token);
      else {
        board[String.fromCharCode(97 + file) + String(8 - row)] = token;
        file++;
      }
    }
  }
  return board;
}

function move(board, from, to, promotion = null) {
  const next = { ...board };
  const piece = next[from];
  delete next[from];
  next[to] = promotion || piece;
  return next;
}

test('recognizes common visual piece labels', () => {
  assert.equal(visual.pieceFromRaw('wP'), 'P');
  assert.equal(visual.pieceFromRaw('piece black-knight square-63'), 'n');
  assert.equal(visual.pieceFromRaw('white queen.svg'), 'Q');
  assert.equal(visual.pieceFromRaw('♚'), 'k');
  assert.equal(visual.pieceFromRaw('not-a-piece'), null);
});

test('recognizes algebraic and numeric square classes', () => {
  assert.equal(visual.normalizeSquare('square-e4 piece'), 'e4');
  assert.equal(visual.normalizeSquare('piece square-52 wp'), 'e2');
  assert.equal(visual.normalizeSquare('data a8'), 'a8');
});

test('reconstructs the initial board and full visual state', () => {
  const board = boardFromPlacement(visual.START_PLACEMENT);
  const snapshot = visual.makeSnapshot(board);
  const tracker = visual.createTracker();
  const state = tracker.update(snapshot, {});
  assert.equal(snapshot.pieceCount, 32);
  assert.equal(state.fen, `${visual.START_PLACEMENT} w KQkq - 0 1`);
  assert.equal(state.source, 'visual-dom');
  assert.equal(state.turnSource, 'initial-position');
});

test('tracks visual move deltas, side to move, and en passant', () => {
  const tracker = visual.createTracker();
  let board = boardFromPlacement(visual.START_PLACEMENT);
  tracker.update(visual.makeSnapshot(board), {});

  board = move(board, 'e2', 'e4');
  let state = tracker.update(visual.makeSnapshot(board), { turn: 'w' }); // stale UI hint is ignored after a visual move.
  assert.equal(state.turn, 'b');
  assert.equal(state.enPassant, 'e3');
  assert.equal(state.fullmove, 1);

  board = move(board, 'e7', 'e5');
  state = tracker.update(visual.makeSnapshot(board), {});
  assert.equal(state.turn, 'w');
  assert.equal(state.enPassant, 'e6');
  assert.equal(state.fullmove, 2);
});

test('removes castling rights permanently after visual king movement', () => {
  const tracker = visual.createTracker();
  let board = boardFromPlacement(visual.START_PLACEMENT);
  tracker.update(visual.makeSnapshot(board), {});

  board = move(board, 'e2', 'e4');
  tracker.update(visual.makeSnapshot(board), {});
  board = move(board, 'a7', 'a6');
  tracker.update(visual.makeSnapshot(board), {});
  board = move(board, 'e1', 'e2');
  const state = tracker.update(visual.makeSnapshot(board), {});
  assert.equal(state.castling, 'kq');
});

test('maps squares correctly in both board orientations', () => {
  const rect = { left: 100, top: 50, width: 800, height: 800 };
  assert.deepEqual({ ...visual.pointForSquare(rect, 'a8', 'white') }, { x: 150, y: 100 });
  assert.deepEqual({ ...visual.pointForSquare(rect, 'a8', 'black') }, { x: 850, y: 800 });
});


test('detects a new initial board as a fresh game', () => {
  const tracker = visual.createTracker();
  let board = boardFromPlacement(visual.START_PLACEMENT);
  tracker.update(visual.makeSnapshot(board), {});
  board = move(board, 'e2', 'e4');
  tracker.update(visual.makeSnapshot(board), {});
  const reset = tracker.update(visual.makeSnapshot(boardFromPlacement(visual.START_PLACEMENT)), {});
  assert.equal(reset.turn, 'w');
  assert.equal(reset.castling, 'KQkq');
  assert.equal(reset.turnSource, 'new-game-reset');
});


test('bootstraps Black to move after a visible White opening move without a turn label', () => {
  const tracker = visual.createTracker();
  let board = boardFromPlacement(visual.START_PLACEMENT);
  board = move(board, 'e2', 'e4');
  const state = tracker.update(visual.makeSnapshot(board, { orientation: 'black' }), { playerSide: 'b' });
  assert.equal(state.turn, 'b');
  assert.equal(state.turnSource, 'opening-activity-parity');
  assert.ok(state.turnConfidence >= 0.8);
});

test('keeps White to move on an untouched board even when the local player is Black', () => {
  const tracker = visual.createTracker();
  const board = boardFromPlacement(visual.START_PLACEMENT);
  const state = tracker.update(visual.makeSnapshot(board, { orientation: 'black' }), { playerSide: 'b' });
  assert.equal(state.turn, 'w');
  assert.equal(state.turnSource, 'initial-position');
});

test('uses detected player side as a nonfatal fallback for an unknown midgame turn', () => {
  const tracker = visual.createTracker();
  const board = boardFromPlacement('4k3/8/8/8/8/8/4P3/4K3');
  const state = tracker.update(visual.makeSnapshot(board, { orientation: 'black' }), { playerSide: 'b' });
  assert.equal(state.turn, 'b');
  assert.equal(state.turnSource, 'player-side-bootstrap');
});

test('visual move deltas replace the low-confidence player-side bootstrap', () => {
  const tracker = visual.createTracker();
  let board = boardFromPlacement('4k3/8/8/8/8/8/4P3/4K3');
  let state = tracker.update(visual.makeSnapshot(board, { orientation: 'black' }), { playerSide: 'b' });
  assert.equal(state.turnSource, 'player-side-bootstrap');
  board = move(board, 'e2', 'e3');
  state = tracker.update(visual.makeSnapshot(board, { orientation: 'black' }), { playerSide: 'b' });
  assert.equal(state.turn, 'b');
  assert.equal(state.turnSource, 'visual-move-delta');
  assert.equal(state.turnConfidence, 1);
});

test('scans a DOM-rendered visual board without any FEN source', () => {
  class FakeElement {
    constructor({ attrs = {}, className = '', rect, tagName = 'DIV', textContent = '' } = {}) {
      this.attrs = attrs;
      this.className = className;
      this.rect = rect;
      this.tagName = tagName;
      this.textContent = textContent;
      this.style = { backgroundImage: '' };
      this.children = [];
      this.parentElement = null;
      this.draggable = false;
    }
    getAttribute(name) { return this.attrs[name] ?? null; }
    getBoundingClientRect() { return this.rect; }
    matches(selector) {
      return selector.includes('.piece') ? this.className.split(/\s+/).includes('piece') : false;
    }
    querySelectorAll() { return []; }
    querySelector() { return null; }
  }

  const boardRect = { left: 0, top: 0, right: 800, bottom: 800, width: 800, height: 800 };
  const root = new FakeElement({ attrs: { 'data-orientation': 'white' }, className: 'chessboard', rect: boardRect });
  const pieces = [];
  const initial = boardFromPlacement('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR');
  for (const [square, piece] of Object.entries(initial)) {
    const file = square.charCodeAt(0) - 97;
    const rank = Number(square[1]);
    const row = 8 - rank;
    const rect = {
      left: file * 100 + 10,
      top: row * 100 + 10,
      right: file * 100 + 90,
      bottom: row * 100 + 90,
      width: 80,
      height: 80
    };
    const token = piece === piece.toUpperCase() ? `w${piece}` : `b${piece.toUpperCase()}`;
    const element = new FakeElement({ attrs: { 'data-piece': token }, className: `piece square-${square}`, rect, tagName: 'IMG' });
    element.parentElement = root;
    pieces.push(element);
  }
  root.children = pieces;
  root.querySelectorAll = (selector) => {
    if (selector.includes('[data-piece]') || selector.includes('.piece') || selector.includes('[class*="piece-"]') || selector.includes('[class*="square-"]')) return pieces;
    return [];
  };
  root.querySelector = (selector) => pieces.find((piece) => selector.includes(`square-${visual.normalizeSquare(piece.className)}`)) || null;

  const fakeDocument = {
    body: { className: '' },
    querySelectorAll(selector) {
      if (selector === '.chessboard' || selector === '[class*="chessboard"]') return [root];
      if (selector.includes('[data-piece]') || selector.includes('.piece') || selector.includes('[class*="piece-"]')) return pieces;
      return [];
    }
  };

  const scanContext = vm.createContext({
    console,
    document: fakeDocument,
    innerWidth: 1200,
    innerHeight: 1000,
    getComputedStyle: () => ({ display: 'block', visibility: 'visible', opacity: '1' })
  });
  vm.runInContext(source, scanContext, { filename: 'visual-board.js' });
  const snapshot = scanContext.VelocityVisualBoardV090.scan(fakeDocument);
  assert.equal(snapshot.placement, visual.START_PLACEMENT);
  assert.equal(snapshot.pieceCount, 32);
  assert.equal(snapshot.orientation, 'white');
});

test('resync rebuilds a complex skipped position from the live board', () => {
  const tracker = visual.createTracker();
  tracker.update(visual.makeSnapshot(boardFromPlacement(visual.START_PLACEMENT)), { turn: 'w' });
  const complex = boardFromPlacement(visual.START_PLACEMENT);
  delete complex.e2;
  delete complex.e7;
  delete complex.g1;
  delete complex.b8;
  complex.e4 = 'P';
  complex.e5 = 'p';
  complex.f3 = 'N';
  complex.c6 = 'n';
  const snapshot = visual.makeSnapshot(complex, { orientation: 'black' });
  const state = tracker.resync(snapshot, { playerSide: 'b' }, 'test-live-resync');
  assert.equal(state.turn, 'b');
  assert.equal(state.source, 'visual-dom-resync');
  assert.match(state.fen, / b - - /);
});
