import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../visual-board.js', import.meta.url), 'utf8');
const context = vm.createContext({
  console,
  innerWidth: 1200,
  innerHeight: 1000,
  getComputedStyle: () => ({ display: 'block', visibility: 'visible', opacity: '1', backgroundColor: 'rgb(186, 202, 68)' })
});
vm.runInContext(source, context, { filename: 'visual-board.js' });
const visual = context.VelocityVisualBoardV090;

function element(className, rect) {
  return {
    className,
    parentElement: null,
    getAttribute() { return null; },
    getBoundingClientRect() { return rect; }
  };
}

test('flipped board uses highlighted White last move to infer Black to move', () => {
  const from = element('square-f6 highlight1-32417', { left: 200, top: 500, right: 300, bottom: 600, width: 100, height: 100 });
  const to = element('square-g7 highlight2-9c5d2', { left: 100, top: 600, right: 200, bottom: 700, width: 100, height: 100 });
  const root = {
    querySelectorAll(selector) {
      if (selector.includes('highlight1')) return [from];
      if (selector.includes('highlight2')) return [to];
      return [];
    },
    querySelector() { return null; }
  };
  from.parentElement = root;
  to.parentElement = root;
  const snapshot = visual.makeSnapshot({ e1: 'K', e8: 'k', g7: 'P', b4: 'n' }, {
    orientation: 'black', root, rootRect: { left: 0, top: 0, right: 800, bottom: 800, width: 800, height: 800 }
  });
  const hint = visual.lastMoverFromHighlights({}, snapshot);
  assert.equal(hint.lastMover, 'w');
  assert.deepEqual(Array.from(hint.lastMoveSquares).sort(), ['f6', 'g7']);
  const inferred = visual.inferInitialTurn(snapshot, { playerSide: 'b', ...hint });
  assert.equal(inferred.turn, 'b');
  assert.equal(inferred.source, 'last-move-highlight-class');
  assert.ok(inferred.confidence >= 0.95);
});
