import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { readFileSync } from 'node:fs';
import { analyzeFen } from '../engine/runtime.js';

function loadModel() {
  const context = vm.createContext({ globalThis: {}, Math, Date });
  context.globalThis = context;
  vm.runInContext(readFileSync(new URL('../skill-levels.js', import.meta.url), 'utf8'), context);
  return context.VelocitySkillModel;
}

const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

test('engine exposes ranked legal candidates for skill simulation', () => {
  const result = analyzeFen(START, { maxDepth: 3, moveTimeMs: 250, candidateCount: 6, hashBits: 16 });
  assert.ok(result.candidates.length >= 3);
  assert.equal(result.move, result.candidates[0].move);
  assert.ok(result.candidates.every((candidate) => /^[a-h][1-8][a-h][1-8][qrbn]?$/.test(candidate.move)));
  for (let i = 1; i < result.candidates.length; i++) {
    assert.ok(result.candidates[i - 1].score >= result.candidates[i].score);
  }
});

test('skill model defines seven levels and five styles', () => {
  const model = loadModel();
  assert.deepEqual(Object.keys(model.SKILLS), ['beginner', 'casual', 'intermediate', 'club', 'advanced', 'expert', 'maximum']);
  assert.deepEqual(Object.keys(model.STYLES), ['balanced', 'aggressive', 'tactical', 'positional', 'defensive']);
});

test('maximum mode always selects the strongest candidate', () => {
  const model = loadModel();
  const personality = model.createPersonality('maximum-test');
  const result = { legalMoves: 3, score: 80, candidates: [
    { move: 'e2e4', score: 80 },
    { move: 'd2d4', score: 50 },
    { move: 'g1f3', score: 30 }
  ] };
  const decision = model.chooseMove(result, {}, { skillLevel: 'maximum', playStyle: 'aggressive' }, personality);
  assert.equal(decision.move, 'e2e4');
});

test('per-game personality is deterministic for the same seed', () => {
  const model = loadModel();
  const first = model.createPersonality('same-game');
  const second = model.createPersonality('same-game');
  assert.equal(first.speedBias, second.speedBias);
  assert.equal(first.hesitationBias, second.hesitationBias);
  assert.equal(first.random(), second.random());
});

test('lower skill receives smaller search budgets than maximum', () => {
  const model = loadModel();
  const settings = { maxDepth: 12, moveTimeMs: 1000 };
  const beginner = model.searchOptions({ ...settings, skillLevel: 'beginner' });
  const maximum = model.searchOptions({ ...settings, skillLevel: 'maximum' });
  assert.ok(beginner.maxDepth < maximum.maxDepth);
  assert.ok(beginner.moveTimeMs < maximum.moveTimeMs);
  assert.ok(beginner.candidateCount > maximum.candidateCount);
});
