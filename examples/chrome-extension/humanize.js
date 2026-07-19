(() => {
  'use strict';
  if (globalThis.VelocityHumanizer) return;

  const clamp = (value, min, max) => Math.max(min, Math.min(max, value));
  const randomBetween = (min, max, random = Math.random) => min + random() * Math.max(0, max - min);
  const smoothstep = (t) => t * t * (3 - 2 * t);

  function bellRandom(random = Math.random) {
    return (random() + random() + random() + random()) / 4;
  }

  function chooseDistinct(value, previous, min, max, random = Math.random, gap = 120) {
    if (!Number.isFinite(previous) || max - min <= gap * 1.5 || Math.abs(value - previous) >= gap) return value;
    const lowerRoom = previous - min;
    const upperRoom = max - previous;
    const direction = upperRoom >= lowerRoom ? 1 : -1;
    const shifted = previous + direction * randomBetween(gap, Math.max(gap, Math.min(max - min, gap * 2.8)), random);
    return clamp(shifted, min, max);
  }

  function timingMultiplier(state = {}, result = {}) {
    const legal = Number(result.legalMoves) || 0;
    const fullmove = Number(state.fullmove) || 1;
    const score = Math.abs(Number(result.score) || 0);
    let multiplier = 1;
    if (legal <= 1) multiplier *= 0.42;
    else if (legal <= 5) multiplier *= 0.78;
    else if (legal >= 30) multiplier *= 1.22;
    if (fullmove <= 6) multiplier *= 0.72;
    else if (fullmove >= 28) multiplier *= 1.08;
    if (score < 35) multiplier *= 1.08;
    else if (score > 500) multiplier *= 0.84;
    return multiplier;
  }

  function chooseMoveDelay(state, result, settings, memory = {}, random = Math.random) {
    const min = clamp(Number(settings.minDelayMs) || 450, 100, 10000);
    const max = clamp(Number(settings.maxDelayMs) || 1800, min, 15000);
    const varied = settings.varyMoveTiming !== false;
    const midpoint = (min + max) / 2;
    let target;
    if (varied) {
      // Bell-shaped choices make middle-speed moves common while keeping fast/slow outliers.
      target = min + bellRandom(random) * (max - min);
      target *= timingMultiplier(state, result);
      target = clamp(target, min * 0.45, max * 1.35);
      target = chooseDistinct(target, memory.lastTargetDelayMs, min * 0.45, max * 1.35, random, Math.min(240, Math.max(75, (max - min) * 0.14)));
    } else {
      target = midpoint;
    }
    const roundedTarget = Math.max(0, Math.round(target));
    memory.lastTargetDelayMs = roundedTarget;
    return {
      targetMs: roundedTarget,
      waitMs: Math.max(0, roundedTarget - (Number(result.elapsedMs) || 0))
    };
  }

  function createMoveBehavior(settings, memory = {}, random = Math.random) {
    const humanize = settings.humanize !== false;
    const varied = settings.varyMoveTiming !== false;
    const minDrag = clamp(Number(settings.dragMinMs) || 260, 120, 1800);
    const maxDrag = clamp(Number(settings.dragMaxMs) || 620, minDrag, 2200);
    if (!humanize) {
      return {
        humanize: false,
        showCursor: false,
        durationMs: Math.round((minDrag + maxDrag) / 2),
        approachMs: 0,
        hoverMs: 0,
        holdMs: 22,
        destinationPauseMs: 0,
        releaseMs: 25,
        microCorrections: 0,
        pathJitterPx: 0,
        overshootPx: 0
      };
    }

    const pick = (min, max, fixed) => varied ? Math.round(randomBetween(min, max, random)) : fixed;
    let duration = pick(minDrag, maxDrag, Math.round((minDrag + maxDrag) / 2));
    if (varied) duration = Math.round(chooseDistinct(duration, memory.lastDragDurationMs, minDrag, maxDrag, random, 45));
    memory.lastDragDurationMs = duration;

    return {
      humanize: true,
      showCursor: true,
      durationMs: duration,
      approachMs: pick(160, 520, 300),
      hoverMs: pick(55, 230, 120),
      holdMs: pick(45, 150, 82),
      destinationPauseMs: pick(20, 145, 58),
      releaseMs: pick(45, 180, 90),
      microCorrections: varied ? (random() < 0.42 ? 1 : 0) : 1,
      pathJitterPx: varied ? randomBetween(0.45, 1.9, random) : 0.9,
      overshootPx: varied && random() < 0.35 ? randomBetween(2, 9, random) : 3,
      bendRatio: varied ? randomBetween(-0.24, 0.24, random) : 0.12,
      pauseChance: varied ? randomBetween(0.05, 0.2, random) : 0.1
    };
  }

  function buildPath(start, end, options = {}, random = Math.random) {
    const distance = Math.hypot(end.x - start.x, end.y - start.y);
    const duration = clamp(Number(options.durationMs) || 420, 40, 3000);
    const steps = clamp(Math.round(duration / 17), 8, 80);
    const bendRatio = Number.isFinite(Number(options.bendRatio)) ? Number(options.bendRatio) : randomBetween(-0.2, 0.2, random);
    const bend = distance * bendRatio;
    const dx = end.x - start.x;
    const dy = end.y - start.y;
    const length = Math.max(1, distance);
    const normal = { x: -dy / length, y: dx / length };
    const jitter = Math.max(0, Number(options.pathJitterPx) || 0);
    const points = [];

    for (let index = 1; index <= steps; index++) {
      const t = index / steps;
      const eased = smoothstep(t);
      const arc = Math.sin(Math.PI * t) * bend;
      const taper = Math.sin(Math.PI * t);
      const noise = index === steps ? 0 : (random() - 0.5) * jitter * taper;
      points.push({
        x: start.x + dx * eased + normal.x * arc + noise,
        y: start.y + dy * eased + normal.y * arc + noise,
        t
      });
    }
    points[points.length - 1] = { x: end.x, y: end.y, t: 1 };
    return points;
  }

  globalThis.VelocityHumanizer = Object.freeze({
    clamp,
    randomBetween,
    chooseMoveDelay,
    createMoveBehavior,
    buildPath
  });
})();
