import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { readFileSync } from 'node:fs';

function loadModel() {
  const context = vm.createContext({ globalThis: {}, Math, Date, Map });
  context.globalThis = context;
  vm.runInContext(readFileSync(new URL('../skill-levels.js', import.meta.url), 'utf8'), context);
  return context.VelocitySkillModel;
}

test('human state accumulates fatigue and tracks evaluation shock', () => {
  const model = loadModel();
  const state = model.createHumanState();
  model.updateHumanState(state, { positionKey: 'a', move: 'e2e4', score: 80, complexity: 0.8 });
  model.updateHumanState(state, { positionKey: 'b', move: 'g1f3', score: -180, complexity: 0.9 });
  assert.equal(state.moveNumber, 2);
  assert.ok(state.fatigue > 0);
  assert.ok(state.tilt > 0);
  assert.ok(state.confidence < 0);
});

test('familiar positions reuse the learned move', () => {
  const model = loadModel();
  const human = model.createHumanState();
  human.familiarPositions.set('repeat', 'd2d4');
  const personality = model.createPersonality('familiar');
  const result = { legalMoves: 3, candidates: [
    { move: 'e2e4', score: 80 }, { move: 'd2d4', score: 70 }, { move: 'g1f3', score: 60 }
  ] };
  const decision = model.chooseMove(result, { positionKey: 'repeat' }, { skillLevel: 'club', playStyle: 'balanced' }, personality, human);
  assert.equal(decision.move, 'd2d4');
  assert.equal(decision.context.familiar, true);
});

test('maximum mode remains strongest-move only with human state present', () => {
  const model = loadModel();
  const result = { legalMoves: 2, candidates: [{ move: 'e2e4', score: 100 }, { move: 'd2d4', score: 20 }] };
  const decision = model.chooseMove(result, {}, { skillLevel: 'maximum', playStyle: 'tactical' }, model.createPersonality('max'), model.createHumanState());
  assert.equal(decision.move, 'e2e4');
});


test('complexity model distinguishes forced and volatile positions', () => {
  const model = loadModel();
  const forced = model.contextFromResult({ pieceCount: 22 }, { legalMoves: 1, candidates: [{ move: 'a1a2', score: 0 }] });
  const volatile = model.contextFromResult({ pieceCount: 22 }, { legalMoves: 30, candidates: [
    { move: 'a1a2', score: 40, check: true },
    { move: 'b1b2', score: 35, capture: true },
    { move: 'c1c2', score: 30 },
    { move: 'd1d2', score: -220, capture: true }
  ] });
  assert.equal(forced.forcedMove, true);
  assert.ok(forced.complexity < volatile.complexity);
  assert.ok(volatile.tacticalVolatility > 0);
});


test('move selection keeps inaccuracies inside a skill-scaled plausibility band', () => {
  const model = loadModel();
  const result = { legalMoves: 4, candidates: [
    { move: 'a2a3', score: 100 },
    { move: 'b2b3', score: 70 },
    { move: 'c2c3', score: -40 },
    { move: 'd2d3', score: -900 }
  ] };
  const personality = model.createPersonality('bounded-loss');
  const decision = model.chooseMove(result, { pieceCount: 24 }, { skillLevel: 'club', playStyle: 'balanced' }, personality, model.createHumanState());
  assert.ok(decision.lossLimit > 0);
  assert.notEqual(decision.move, 'd2d3');
  assert.ok(decision.scoreLoss <= decision.lossLimit * 1.35);
});

test('time pressure increases human error pressure without affecting maximum mode', () => {
  const model = loadModel();
  const result = { legalMoves: 20, candidates: [{ move: 'a2a3', score: 40 }, { move: 'b2b3', score: 35 }] };
  const calm = model.contextFromResult({ pieceCount: 20, remainingTimeMs: 120000 }, result);
  const rushed = model.contextFromResult({ pieceCount: 20, remainingTimeMs: 3500 }, result);
  assert.ok(rushed.timePressure > calm.timePressure);
  const maximum = model.chooseMove(result, { pieceCount: 20, remainingTimeMs: 3500 }, { skillLevel: 'maximum', playStyle: 'balanced' }, model.createPersonality('max-pressure'), model.createHumanState());
  assert.equal(maximum.move, 'a2a3');
});


test('calm forced moves allow fatigue and tilt to recover', () => {
  const model = loadModel();
  const state = model.createHumanState();
  state.fatigue = 0.55;
  state.tilt = 0.6;
  for (let i = 0; i < 5; i++) model.updateHumanState(state, { complexity: 0.05, forcedMove: true, score: 0 });
  assert.ok(state.fatigue < 0.55);
  assert.ok(state.tilt < 0.2);
  assert.ok(state.calmMoves >= 5);
});

test('recent mistakes trigger a bounded self-correction period', () => {
  const model = loadModel();
  const human = model.createHumanState();
  model.updateHumanState(human, { complexity: 0.8, mistakeType: 'plan-error', selectedRank: 3, score: -100 });
  assert.ok(human.recentMistakes > 0);
  model.updateHumanState(human, { complexity: 0.1, forcedMove: true, mistakeType: 'none', selectedRank: 1, score: -95 });
  assert.ok(human.recentMistakes < 2);
});
