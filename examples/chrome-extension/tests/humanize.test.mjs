import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../humanize.js', import.meta.url), 'utf8');
const context = vm.createContext({ console, Math });
vm.runInContext(source, context, { filename: 'humanize.js' });
const humanizer = context.VelocityHumanizer;

function sequence(values) {
  let index = 0;
  return () => values[index++ % values.length];
}

test('different-time mode avoids a repeated move cadence', () => {
  const settings = { minDelayMs: 500, maxDelayMs: 1800, varyMoveTiming: true };
  const memory = {};
  const random = sequence([0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.9]);
  const first = humanizer.chooseMoveDelay({ fullmove: 15 }, { legalMoves: 20, elapsedMs: 0, score: 100 }, settings, memory, random);
  const second = humanizer.chooseMoveDelay({ fullmove: 15 }, { legalMoves: 20, elapsedMs: 0, score: 100 }, settings, memory, random);
  assert.notEqual(second.targetMs, first.targetMs);
  assert.ok(Math.abs(second.targetMs - first.targetMs) >= 75);
});

test('fixed-time mode returns a stable midpoint cadence', () => {
  const settings = { minDelayMs: 400, maxDelayMs: 1600, varyMoveTiming: false };
  const memory = {};
  const first = humanizer.chooseMoveDelay({}, { elapsedMs: 100 }, settings, memory, Math.random);
  const second = humanizer.chooseMoveDelay({}, { elapsedMs: 100 }, settings, memory, Math.random);
  assert.deepEqual({ ...first }, { targetMs: 1000, waitMs: 900 });
  assert.deepEqual({ ...second }, { targetMs: 1000, waitMs: 900 });
});

test('humanized behavior adds approach, hover, correction, and release phases', () => {
  const behavior = humanizer.createMoveBehavior({
    humanize: true,
    varyMoveTiming: false,
    dragMinMs: 300,
    dragMaxMs: 700
  }, {});
  assert.equal(behavior.humanize, true);
  assert.equal(behavior.showCursor, true);
  assert.ok(behavior.approachMs > 0);
  assert.ok(behavior.hoverMs > 0);
  assert.ok(behavior.holdMs > 0);
  assert.ok(behavior.releaseMs > 0);
  assert.equal(behavior.microCorrections, 1);
});

test('generated mouse path ends exactly on the destination', () => {
  const path = humanizer.buildPath({ x: 10, y: 20 }, { x: 300, y: 240 }, {
    durationMs: 500,
    bendRatio: 0.2,
    pathJitterPx: 2
  }, sequence([0.25, 0.75]));
  assert.ok(path.length >= 8);
  assert.deepEqual({ x: path.at(-1).x, y: path.at(-1).y }, { x: 300, y: 240 });
  assert.ok(path.some((point) => Math.abs(point.y - (20 + (240 - 20) * point.t)) > 1));
});


test('forced moves receive much shorter thinking time than complex choices', () => {
  const settings = { minDelayMs: 500, maxDelayMs: 2400, varyMoveTiming: true };
  const random = sequence([0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5]);
  const forced = humanizer.chooseMoveDelay({ fullmove: 20, pieceCount: 24 }, { legalMoves: 1, elapsedMs: 0, score: 20 }, settings, {}, random, { complexity: 0.06, forcedMove: true, gamePhase: 'middlegame' });
  const complex = humanizer.chooseMoveDelay({ fullmove: 20, pieceCount: 24 }, { legalMoves: 32, elapsedMs: 0, score: 20 }, settings, {}, random, { complexity: 0.92, candidateCloseness: 0.95, tacticalVolatility: 0.8, gamePhase: 'middlegame' });
  assert.ok(forced.targetMs < complex.targetMs);
});

test('clock budget caps thought time while preserving a reserve', () => {
  const settings = { minDelayMs: 500, maxDelayMs: 5000, varyMoveTiming: true };
  const timing = humanizer.chooseMoveDelay({ fullmove: 28, pieceCount: 18, remainingTimeMs: 10000, incrementMs: 0 }, { legalMoves: 28, elapsedMs: 0, score: 0 }, settings, {}, sequence([0.9, 0.9, 0.9, 0.9]), { complexity: 1, gamePhase: 'middlegame' });
  assert.ok(timing.budget);
  assert.ok(timing.targetMs <= Math.ceil(timing.budget.cap));
  assert.ok(timing.budget.reserve >= 1200);
});


test('long thinks create time debt that shortens a later simple decision', () => {
  const settings = { minDelayMs: 500, maxDelayMs: 3000, varyMoveTiming: true };
  const memory = {};
  const random = sequence([0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95, 0.95]);
  humanizer.chooseMoveDelay({ fullmove: 22 }, { legalMoves: 30, elapsedMs: 0, score: 0 }, settings, memory, random, { complexity: 1, candidateCloseness: 1, tacticalVolatility: 1, gamePhase: 'middlegame' });
  assert.ok(memory.timeDebtMs > 0);
  const debtBefore = memory.timeDebtMs;
  const simple = humanizer.chooseMoveDelay({ fullmove: 23 }, { legalMoves: 8, elapsedMs: 0, score: 0 }, settings, memory, sequence([0.5, 0.5, 0.5, 0.5]), { complexity: 0.2, gamePhase: 'middlegame' });
  assert.ok(simple.targetMs < 1800);
  assert.ok(memory.timeDebtMs < debtBefore || memory.timeDebtMs > 0);
});

test('recorded normalized mouse paths rescale and land exactly on the destination', () => {

  const path = humanizer.buildPath(
    { x: 20, y: 30 },
    { x: 220, y: 130 },
    {
      recordedTemplate: {
        distance: 100,
        points: [
          { p: 0, n: 0, t: 0 },
          { p: 0.2, n: 0.08, t: 0.1 },
          { p: 0.55, n: -0.04, t: 0.42 },
          { p: 0.88, n: 0.02, t: 0.82 },
          { p: 1, n: 0, t: 1 }
        ]
      }
    },
    () => 0.5
  );
  assert.equal(path.length, 5);
  assert.deepEqual({ x: path[0].x, y: path[0].y, t: path[0].t }, { x: 20, y: 30, t: 0 });
  assert.deepEqual({ x: path.at(-1).x, y: path.at(-1).y, t: path.at(-1).t }, { x: 220, y: 130, t: 1 });
  assert.notEqual(path[1].y, 50);
});

test('recorded profile calculates a fresh blended trajectory for the requested distance', () => {
  const profile = {
    meanDurationMs: 400,
    meanDistance: 100,
    templates: [
      { durationMs: 360, distance: 80, points: [
        { p: 0, n: 0, t: 0 }, { p: 0.3, n: 0.08, t: 0.25 }, { p: 0.72, n: 0.03, t: 0.7 }, { p: 1, n: 0, t: 1 }
      ] },
      { durationMs: 460, distance: 140, points: [
        { p: 0, n: 0, t: 0 }, { p: 0.22, n: -0.04, t: 0.2 }, { p: 0.68, n: 0.01, t: 0.66 }, { p: 1, n: 0, t: 1 }
      ] }
    ]
  };
  const learned = humanizer.calculateRecordedTemplate(profile, 120, () => 0.5);
  assert.equal(learned.calculated, true);
  assert.equal(learned.sourceCount, 2);
  assert.equal(learned.distance, 120);
  assert.ok(learned.points.length >= 18);
  assert.deepEqual({ ...learned.points[0] }, { p: 0, n: 0, t: 0 });
  assert.deepEqual({ ...learned.points.at(-1) }, { p: 1, n: 0, t: 1 });
  assert.ok(learned.durationMs > 360 && learned.durationMs < 520);
});

test('move behavior carries the recorded profile so page-distance calculation happens at execution time', () => {
  const profile = { meanDurationMs: 430, templates: [{ durationMs: 430, distance: 100, points: [
    { p: 0, n: 0, t: 0 }, { p: 0.3, n: 0.05, t: 0.3 }, { p: 0.7, n: 0.02, t: 0.7 }, { p: 1, n: 0, t: 1 }
  ] }] };
  const behavior = humanizer.createMoveBehavior({ humanize: true, varyMoveTiming: false, dragMinMs: 200, dragMaxMs: 700, recordedMouseProfile: profile }, {});
  assert.equal(behavior.recordedMouseProfile, profile);
  assert.equal(behavior.recordedTemplate, null);
  assert.equal(behavior.dragMinMs, 200);
  assert.equal(behavior.dragMaxMs, 700);
});


test('recorded profile confidence rejects poorly matched movement distances', () => {
  const profile = { meanDistance: 30, templates: Array.from({ length: 4 }, (_, i) => ({
    distance: 28 + i, durationMs: 120, points: [
      { p: 0, n: 0, t: 0 }, { p: 0.3, n: 0.02, t: 0.3 },
      { p: 0.7, n: -0.01, t: 0.7 }, { p: 1, n: 0, t: 1 }
    ]
  })) };
  const quality = humanizer.assessRecordedProfile(profile, 900);
  assert.equal(quality.usable, false);
  assert.equal(humanizer.calculateRecordedTemplate(profile, 900, () => 0.5), null);
});

test('recorded profile confidence accepts well-covered movement distances', () => {
  const distances = [30, 55, 90, 140, 220, 320, 450, 620, 780, 900, 180, 380];
  const profile = { meanDistance: 300, templates: distances.map((distance) => ({
    distance, durationMs: 120 + distance * 0.7, points: [
      { p: 0, n: 0, t: 0 }, { p: 0.28, n: 0.03, t: 0.24 },
      { p: 0.72, n: -0.02, t: 0.74 }, { p: 1, n: 0, t: 1 }
    ]
  })) };
  const quality = humanizer.assessRecordedProfile(profile, 500);
  assert.equal(quality.usable, true);
  assert.ok(quality.score >= 45);
  assert.ok(humanizer.calculateRecordedTemplate(profile, 500, () => 0.5));
});
