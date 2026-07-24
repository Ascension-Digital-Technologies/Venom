// @venom: browser
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

  function estimateMovesRemaining(state = {}) {
    const pieceCount = Number(state.pieceCount) || 32;
    const fullmove = Number(state.fullmove) || 1;
    if (pieceCount <= 10) return Math.max(10, 30 - Math.floor(fullmove / 8));
    if (pieceCount <= 20) return Math.max(14, 36 - Math.floor(fullmove / 10));
    return Math.max(20, 42 - Math.floor(fullmove / 12));
  }

  function timingMultiplier(state = {}, result = {}, context = {}) {
    const legal = Number(result.legalMoves) || 0;
    const fullmove = Number(state.fullmove) || 1;
    const score = Math.abs(Number(result.score) || 0);
    const complexity = Number.isFinite(Number(context.complexity)) ? Number(context.complexity) : Math.min(1, legal / 35);
    let multiplier = 0.72 + complexity * 0.82;
    if (context.forcedMove || legal <= 1) multiplier *= 0.34;
    else if (legal <= 4) multiplier *= 0.68;
    if (context.inCheck) multiplier *= context.forcedMove ? 0.82 : 1.12;
    if (context.tacticalVolatility > 0.65) multiplier *= 1.18;
    if (context.candidateCloseness > 0.8) multiplier *= 1.12;
    if (fullmove <= 6 && context.gamePhase === 'opening') multiplier *= 0.62;
    else if (fullmove >= 28) multiplier *= 1.06;
    if (score > 700) multiplier *= 0.78;
    return multiplier;
  }

  function clockBudget(settings = {}, state = {}, context = {}) {
    const remaining = Number(state.remainingTimeMs ?? state.clockMs ?? settings.remainingTimeMs);
    const increment = Math.max(0, Number(state.incrementMs ?? settings.incrementMs) || 0);
    if (!(remaining > 0)) return null;
    const reserve = Math.max(1200, remaining * (remaining < 20000 ? 0.28 : 0.12));
    const usable = Math.max(100, remaining - reserve);
    const perMove = usable / estimateMovesRemaining(state) + increment * 0.72;
    const criticalBoost = 0.62 + (Number(context.complexity) || 0) * 1.15;
    const cap = Math.max(120, Math.min(remaining * 0.16, perMove * criticalBoost));
    return { remaining, reserve, perMove, cap };
  }

  function chooseMoveDelay(state, result, settings, memory = {}, random = Math.random, context = {}) {
    const min = clamp(Number(settings.minDelayMs) || 450, 100, 10000);
    const max = clamp(Number(settings.maxDelayMs) || 1800, min, 15000);
    const varied = settings.varyMoveTiming !== false;
    const midpoint = (min + max) / 2;
    let target;
    if (varied) {
      target = min + bellRandom(random) * (max - min);
      target *= timingMultiplier(state, result, context);
      const budget = clockBudget(settings, state, context);
      if (budget) target = Math.min(target, budget.cap);
      if (context.familiar) target *= 0.42;
      if (context.surprise > 0) target *= 1 + Math.min(0.5, context.surprise * 0.45);
      const timeDebt = Math.max(0, Number(memory.timeDebtMs) || 0);
      if (timeDebt > 0 && !context.forcedMove) {
        const repayment = Math.min(target * 0.34, timeDebt * (0.16 + (1 - (Number(context.complexity) || 0)) * 0.22));
        target -= repayment;
        memory.timeDebtMs = Math.max(0, timeDebt - repayment);
      }
      target = clamp(target, Math.min(min * 0.34, max), max * 1.55);
      target = chooseDistinct(target, memory.lastTargetDelayMs, Math.min(min * 0.34, max), max * 1.55, random, Math.min(240, Math.max(75, (max - min) * 0.14)));
    } else {
      target = midpoint;
    }
    const roundedTarget = Math.max(0, Math.round(target));
    const baseline = min + (max - min) * (0.34 + (Number(context.complexity) || 0) * 0.34);
    if (roundedTarget > baseline * 1.12) memory.timeDebtMs = Math.min(max * 2.2, (Number(memory.timeDebtMs) || 0) + (roundedTarget - baseline) * 0.42);
    else if (context.forcedMove) memory.timeDebtMs = Math.max(0, (Number(memory.timeDebtMs) || 0) - min * 0.18);
    memory.lastTargetDelayMs = roundedTarget;
    const output = {
      targetMs: roundedTarget,
      waitMs: Math.max(0, roundedTarget - (Number(result.elapsedMs) || 0))
    };
    const budget = clockBudget(settings, state, context);
    if (budget) output.budget = budget;
    return output;
  }


  function sampleTemplateAt(template, t) {
    const points = Array.isArray(template?.points) ? template.points : [];
    if (!points.length) return { p: t, n: 0, t };
    if (t <= Number(points[0].t || 0)) return { p: Number(points[0].p) || 0, n: Number(points[0].n) || 0, t };
    for (let index = 1; index < points.length; index++) {
      const right = points[index];
      const left = points[index - 1];
      const rt = clamp(Number(right.t) || 0, 0, 1);
      if (t <= rt || index === points.length - 1) {
        const lt = clamp(Number(left.t) || 0, 0, 1);
        const span = Math.max(0.0001, rt - lt);
        const mix = clamp((t - lt) / span, 0, 1);
        return {
          p: (Number(left.p) || 0) + ((Number(right.p) || 0) - (Number(left.p) || 0)) * mix,
          n: (Number(left.n) || 0) + ((Number(right.n) || 0) - (Number(left.n) || 0)) * mix,
          t
        };
      }
    }
    const last = points[points.length - 1];
    return { p: Number(last.p) || 1, n: Number(last.n) || 0, t };
  }


  function assessRecordedProfile(profile, targetDistance = null) {
    const templates = Array.isArray(profile?.templates) ? profile.templates.filter((item) => Array.isArray(item?.points) && item.points.length >= 4) : [];
    if (!templates.length) return { score: 0, confidence: 0, label: 'None', usable: false };
    const distances = templates.map((item) => Math.max(1, Number(item.distance) || Number(profile.meanDistance) || 160));
    const countScore = clamp(templates.length / 24, 0, 1);
    const minDistance = Math.min(...distances);
    const maxDistance = Math.max(...distances);
    const coverage = clamp(Math.log2(Math.max(1, maxDistance / minDistance)) / 2.6 + Math.min(0.35, templates.length / 80), 0, 1);
    let distanceConfidence = 1;
    if (Number.isFinite(Number(targetDistance))) {
      const target = Math.max(1, Number(targetDistance));
      const nearest = Math.min(...distances.map((value) => Math.abs(Math.log(target / value))));
      distanceConfidence = clamp(1 - nearest / 1.15, 0, 1);
    }
    const confidence = clamp(countScore * 0.45 + coverage * 0.25 + distanceConfidence * 0.30, 0, 1);
    const score = Math.round(confidence * 100);
    return { score, confidence, label: score >= 82 ? 'Excellent' : score >= 68 ? 'Good' : score >= 48 ? 'Fair' : score >= 25 ? 'Weak' : 'Insufficient', usable: confidence >= 0.34, minDistance, maxDistance };
  }

  function calculateRecordedTemplate(profile, targetDistance, random = Math.random) {
    const quality = assessRecordedProfile(profile, targetDistance);
    if (!quality.usable) return null;
    const templates = Array.isArray(profile?.templates) ? profile.templates.filter((item) => Array.isArray(item?.points) && item.points.length >= 4) : [];
    if (!templates.length) return null;
    const distance = Math.max(1, Number(targetDistance) || Number(profile.meanDistance) || 160);
    const ranked = templates.map((template) => {
      const sourceDistance = Math.max(1, Number(template.distance) || Number(profile.meanDistance) || distance);
      const ratioError = Math.abs(Math.log(distance / sourceDistance));
      return { template, sourceDistance, weight: 1 / (0.12 + ratioError * ratioError) };
    }).sort((a, b) => b.weight - a.weight).slice(0, Math.min(7, templates.length));
    const totalWeight = ranked.reduce((sum, item) => sum + item.weight, 0) || 1;
    const pointCount = clamp(Math.round(18 + Math.min(28, distance / 8)), 18, 46);
    const points = [];
    for (let index = 0; index < pointCount; index++) {
      const t = index / Math.max(1, pointCount - 1);
      const samples = ranked.map((item) => ({ ...sampleTemplateAt(item.template, t), weight: item.weight }));
      const meanP = samples.reduce((sum, item) => sum + item.p * item.weight, 0) / totalWeight;
      const meanN = samples.reduce((sum, item) => sum + item.n * item.weight, 0) / totalWeight;
      const varianceN = samples.reduce((sum, item) => sum + (item.n - meanN) ** 2 * item.weight, 0) / totalWeight;
      const taper = Math.sin(Math.PI * t);
      const naturalVariation = (random() - 0.5) * Math.sqrt(Math.max(0, varianceN)) * 0.55 * taper;
      points.push({
        p: clamp(meanP, -0.12, 1.12),
        n: clamp(meanN + naturalVariation, -0.5, 0.5),
        t
      });
    }
    points[0] = { p: 0, n: 0, t: 0 };
    points[points.length - 1] = { p: 1, n: 0, t: 1 };
    const durationNumerator = ranked.reduce((sum, item) => {
      const base = Math.max(1, Number(item.template.durationMs) || Number(profile.meanDurationMs) || 420);
      const scaled = base * Math.pow(distance / item.sourceDistance, 0.58);
      return sum + scaled * item.weight;
    }, 0);
    return {
      distance,
      durationMs: Math.max(40, Math.round((durationNumerator / totalWeight) * randomBetween(0.94, 1.07, random))),
      points,
      calculated: true,
      sourceCount: ranked.length,
      confidence: quality.confidence,
      qualityScore: quality.score,
      qualityLabel: quality.label
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

    const profile = settings.recordedMouseProfile;
    if (profile && Number(profile.meanDurationMs) > 0) {
      duration = Math.round(clamp(Number(profile.meanDurationMs) * randomBetween(0.94, 1.07, random), minDrag, maxDrag));
    }

    return {
      humanize: true,
      showCursor: true,
      durationMs: duration,
      recordedMouseProfile: profile || null,
      recordedTemplate: null,
      dragMinMs: minDrag,
      dragMaxMs: maxDrag,
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
    const recorded = options.recordedTemplate || calculateRecordedTemplate(options.recordedMouseProfile, distance, random);
    if (recorded && Array.isArray(recorded.points) && recorded.points.length >= 4) {
      const dx = end.x - start.x;
      const dy = end.y - start.y;
      const length = Math.max(1, distance);
      const normal = { x: -dy / length, y: dx / length };
      const scale = clamp(distance / Math.max(20, Number(recorded.distance) || distance), 0.35, 3.5);
      const points = recorded.points.map((point, index) => {
        const progress = clamp(Number(point.p) || 0, -0.18, 1.18);
        const lateral = clamp(Number(point.n) || 0, -0.6, 0.6) * length * Math.min(1.25, scale);
        return {
          x: start.x + dx * progress + normal.x * lateral,
          y: start.y + dy * progress + normal.y * lateral,
          t: clamp(Number(point.t) || index / Math.max(1, recorded.points.length - 1), 0, 1)
        };
      });
      points[0] = { x: start.x, y: start.y, t: 0 };
      points[points.length - 1] = { x: end.x, y: end.y, t: 1 };
      return points;
    }
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
    buildPath,
    calculateRecordedTemplate,
    assessRecordedProfile
  });
})();
