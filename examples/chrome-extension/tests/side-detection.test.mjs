import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const visualSource = await readFile(new URL('../visual-board.js', import.meta.url), 'utf8');
const contentSource = await readFile(new URL('../content-script.js', import.meta.url), 'utf8');
const popupHtml = await readFile(new URL('../popup.html', import.meta.url), 'utf8');
const context = vm.createContext({ console });
vm.runInContext(visualSource, context, { filename: 'visual-board.js' });
const visual = context.VelocityVisualBoardV090;

function emptyDocument(routes = new Map()) {
  return {
    querySelectorAll(selector) {
      return routes.get(selector) || [];
    }
  };
}

function element(attributes = {}, textContent = '', className = '') {
  return {
    className,
    textContent,
    getAttribute(name) { return attributes[name] ?? null; }
  };
}

test('recognizes explicit player-side language without confusing turn language', () => {
  assert.equal(visual.playerSideFromText('You are playing as Black'), 'b');
  assert.equal(visual.playerSideFromText("You're playing as White"), 'w');
  assert.equal(visual.playerSideFromText('White player (you)'), 'w');
  assert.equal(visual.playerSideFromText('Black to move'), null);
  assert.equal(visual.playerSideFromText('black', true), 'b');
});

test('prefers explicit page player color over board orientation', () => {
  const player = element({ 'data-player-color': 'black' });
  const doc = emptyDocument(new Map([['[data-player-color]', [player]]]));
  const snapshot = visual.makeSnapshot({}, { orientation: 'white' });
  const detected = visual.readPlayerSide(doc, snapshot);
  assert.equal(detected.side, 'b');
  assert.equal(detected.source, '[data-player-color]');
  assert.equal(detected.confidence, 1);
});

test('uses interactive piece ownership before orientation fallback', () => {
  const whitePiece = element({ draggable: 'true' });
  whitePiece.draggable = true;
  const whitePieceTwo = element({ draggable: 'true' });
  whitePieceTwo.draggable = true;
  const snapshot = visual.makeSnapshot({ e2: 'P', d2: 'P' }, {
    orientation: 'black',
    elements: new Map([['e2', whitePiece], ['d2', whitePieceTwo]])
  });
  const detected = visual.readPlayerSide(emptyDocument(), snapshot);
  assert.equal(detected.side, 'w');
  assert.equal(detected.source, 'interactive-white-pieces');
});

test('falls back to the side shown at the bottom of the board', () => {
  const black = visual.readPlayerSide(emptyDocument(), visual.makeSnapshot({}, { orientation: 'black' }));
  const white = visual.readPlayerSide(emptyDocument(), visual.makeSnapshot({}, { orientation: 'white' }));
  assert.equal(black.side, 'b');
  assert.equal(black.source, 'board-orientation');
  assert.equal(white.side, 'w');
});

test('auto detect is the popup default and content script locks the detected side', () => {
  assert.match(popupHtml, /<option value="auto" selected>/);
  assert.match(contentSource, /side: \['auto', 'w', 'b', 'both'\]/);
  assert.match(contentSource, /session\.detectedSide = state\.playerSide/);
  assert.match(contentSource, /if \(session\.detectedSide === 'w' \|\| session\.detectedSide === 'b'\)/);
});
