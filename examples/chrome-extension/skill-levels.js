// @venom: browser
(() => {
  'use strict';
  if (globalThis.VelocitySkillModel) return;

  const SKILLS = Object.freeze({
    beginner: Object.freeze({ id: 'beginner', label: 'Beginner · 700', rating: 700, maxDepth: 3, timeScale: 0.32, candidateCount: 8, temperature: 175, blunderRate: 0.11, tacticalVision: 0.35, cursorDirectness: 0.62 }),
    casual: Object.freeze({ id: 'casual', label: 'Casual · 1000', rating: 1000, maxDepth: 4, timeScale: 0.45, candidateCount: 7, temperature: 120, blunderRate: 0.065, tacticalVision: 0.52, cursorDirectness: 0.68 }),
    intermediate: Object.freeze({ id: 'intermediate', label: 'Intermediate · 1300', rating: 1300, maxDepth: 5, timeScale: 0.62, candidateCount: 6, temperature: 78, blunderRate: 0.032, tacticalVision: 0.70, cursorDirectness: 0.74 }),
    club: Object.freeze({ id: 'club', label: 'Club · 1600', rating: 1600, maxDepth: 7, timeScale: 0.78, candidateCount: 5, temperature: 47, blunderRate: 0.014, tacticalVision: 0.86, cursorDirectness: 0.81 }),
    advanced: Object.freeze({ id: 'advanced', label: 'Advanced · 1900', rating: 1900, maxDepth: 9, timeScale: 0.90, candidateCount: 4, temperature: 25, blunderRate: 0.005, tacticalVision: 0.95, cursorDirectness: 0.87 }),
    expert: Object.freeze({ id: 'expert', label: 'Expert · 2200', rating: 2200, maxDepth: 11, timeScale: 1.0, candidateCount: 3, temperature: 12, blunderRate: 0.0015, tacticalVision: 0.985, cursorDirectness: 0.91 }),
    maximum: Object.freeze({ id: 'maximum', label: 'Maximum · Full strength', rating: 2600, maxDepth: 12, timeScale: 1.0, candidateCount: 1, temperature: 0, blunderRate: 0, tacticalVision: 1, cursorDirectness: 0.94 })
  });

  const STYLES = Object.freeze({
    balanced: Object.freeze({ id: 'balanced', label: 'Balanced', captureBonus: 0, checkBonus: 0, promotionBonus: 0, volatility: 1 }),
    aggressive: Object.freeze({ id: 'aggressive', label: 'Aggressive', captureBonus: 16, checkBonus: 22, promotionBonus: 14, volatility: 1.18 }),
    tactical: Object.freeze({ id: 'tactical', label: 'Tactical', captureBonus: 20, checkBonus: 28, promotionBonus: 18, volatility: 1.12 }),
    positional: Object.freeze({ id: 'positional', label: 'Positional', captureBonus: -5, checkBonus: 2, promotionBonus: 8, volatility: 0.82 }),
    defensive: Object.freeze({ id: 'defensive', label: 'Defensive', captureBonus: -2, checkBonus: -3, promotionBonus: 10, volatility: 0.72 })
  });

  const BEHAVIORS = Object.freeze({
    natural: Object.freeze({ id: 'natural', label: 'Natural', speed: 1, hesitation: 1, correction: 1 }),
    fast: Object.freeze({ id: 'fast', label: 'Fast', speed: 0.72, hesitation: 0.55, correction: 0.55 }),
    careful: Object.freeze({ id: 'careful', label: 'Careful', speed: 1.25, hesitation: 1.35, correction: 0.85 }),
    varied: Object.freeze({ id: 'varied', label: 'Highly varied', speed: 1.05, hesitation: 1.2, correction: 1.35 })
  });

  function hashString(text) {
    let hash = 2166136261 >>> 0;
    for (let i = 0; i < text.length; i++) {
      hash ^= text.charCodeAt(i);
      hash = Math.imul(hash, 16777619) >>> 0;
    }
    return hash || 1;
  }

  function mulberry32(seed) {
    let state = seed >>> 0;
    return () => {
      state |= 0;
      state = state + 0x6D2B79F5 | 0;
      let value = Math.imul(state ^ state >>> 15, 1 | state);
      value = value + Math.imul(value ^ value >>> 7, 61 | value) ^ value;
      return ((value ^ value >>> 14) >>> 0) / 4294967296;
    };
  }

  function createPersonality(seedText = '') {
    const random = mulberry32(hashString(seedText || `${Date.now()}-${Math.random()}`));
    return Object.freeze({
      seed: seedText,
      speedBias: 0.90 + random() * 0.22,
      hesitationBias: 0.82 + random() * 0.38,
      accuracyBias: (random() - 0.5) * 0.10,
      aggressionBias: (random() - 0.5) * 0.20,
      cursorDirectnessBias: (random() - 0.5) * 0.12,
      random
    });
  }

  function contextFromResult(state, result) {
    const candidates = Array.isArray(result?.candidates) ? result.candidates : [];
    const legal = Math.max(1, Number(result?.legalMoves) || candidates.length || 1);
    const scores = candidates.map((candidate) => Number(candidate?.score)).filter(Number.isFinite);
    const best = Number(scores[0] ?? result?.score ?? 0);
    const second = Number(scores[1] ?? best);
    const third = Number(scores[2] ?? second);
    const bestGap = Math.abs(best - second);
    const spread = scores.length > 1 ? Math.max(...scores) - Math.min(...scores) : 0;
    const closeness = Math.max(0, 1 - Math.min(1, bestGap / 180));
    const choiceDensity = Math.min(1, legal / 34);
    const forcingMoves = candidates.filter((candidate) => candidate?.capture || candidate?.check || candidate?.promotion).length;
    const forcingRatio = candidates.length ? forcingMoves / candidates.length : 0;
    const tacticalVolatility = Math.min(1, forcingRatio * 0.72 + Math.min(1, spread / 420) * 0.28);
    const ambiguity = Math.max(0, 1 - Math.min(1, Math.abs(second - third) / 160));
    const inCheck = Boolean(state?.inCheck);
    const remainingTimeMs = Number(state?.remainingTimeMs ?? state?.clockMs);
    const incrementMs = Math.max(0, Number(state?.incrementMs) || 0);
    const timePressure = remainingTimeMs > 0
      ? Math.max(0, Math.min(1, 1 - (remainingTimeMs + incrementMs * 8) / 45000))
      : 0;
    const forcedMove = legal === 1;
    const pieceCount = Number(state?.pieceCount || 32);
    const gamePhase = pieceCount <= 12 ? 'endgame' : pieceCount <= 24 ? 'middlegame' : 'opening';
    const phaseWeight = gamePhase === 'middlegame' ? 1 : gamePhase === 'endgame' ? 0.86 : 0.72;
    let complexity = choiceDensity * 0.28 + closeness * 0.30 + ambiguity * 0.14 + tacticalVolatility * 0.28;
    complexity *= phaseWeight;
    if (inCheck) complexity = Math.min(1, complexity + 0.10);
    if (forcedMove) complexity = 0.06;
    else if (legal <= 3) complexity *= 0.55;
    return {
      complexity: Math.max(0, Math.min(1, complexity)),
      candidateCloseness: closeness,
      candidateAmbiguity: ambiguity,
      tacticalVolatility,
      forcingMoves,
      scoreSpread: spread,
      bestGap,
      forcedMove,
      inCheck,
      legalMoves: legal,
      gamePhase,
      timePressure
    };
  }

  function createHumanState() {
    return {
      moveNumber: 0,
      fatigue: 0,
      confidence: 0,
      tilt: 0,
      lastScore: null,
      lastMove: '',
      lastPositionKey: '',
      familiarPositions: new Map(),
      recentMistakes: 0,
      calmMoves: 0,
      complexStreak: 0,
      telemetry: []
    };
  }

  function updateHumanState(humanState, observation = {}) {
    if (!humanState) return;
    humanState.moveNumber++;
    const complexity = Math.max(0, Math.min(1, Number(observation.complexity) || 0));
    const forcedMove = Boolean(observation.forcedMove);
    const calm = forcedMove || complexity < 0.28;
    humanState.complexStreak = complexity > 0.68 ? Math.min(12, (humanState.complexStreak || 0) + 1) : Math.max(0, (humanState.complexStreak || 0) - 1);
    humanState.calmMoves = calm ? Math.min(12, (humanState.calmMoves || 0) + 1) : 0;
    const strain = 0.004 + complexity * 0.009 + Math.min(0.018, humanState.complexStreak * 0.0015);
    const recovery = calm ? 0.012 + humanState.calmMoves * 0.0015 : 0;
    humanState.fatigue = Math.max(0, Math.min(1, humanState.fatigue + strain - recovery));
    humanState.tilt = Math.max(0, humanState.tilt * (calm ? 0.74 : 0.88));
    humanState.confidence *= 0.94;

    const score = Number(observation.score);
    if (Number.isFinite(score) && Number.isFinite(humanState.lastScore)) {
      const swing = score - humanState.lastScore;
      humanState.confidence = Math.max(-1, Math.min(1, humanState.confidence + Math.sign(swing) * Math.min(0.35, Math.abs(swing) / 500)));
      if (swing < -120) humanState.tilt = Math.min(1, humanState.tilt + Math.min(0.28, Math.abs(swing) / 900));
    }
    const mistakeType = String(observation.mistakeType || 'none');
    if (mistakeType !== 'none' || Number(observation.selectedRank) > 2) humanState.recentMistakes = Math.min(4, (humanState.recentMistakes || 0) + 1);
    else humanState.recentMistakes = Math.max(0, (humanState.recentMistakes || 0) - (calm ? 1 : 0.5));
    if (Number.isFinite(score)) humanState.lastScore = score;
    if (observation.positionKey && observation.move) {
      humanState.familiarPositions.set(observation.positionKey, observation.move);
      if (humanState.familiarPositions.size > 96) humanState.familiarPositions.delete(humanState.familiarPositions.keys().next().value);
    }
    humanState.lastMove = observation.move || humanState.lastMove;
    humanState.lastPositionKey = observation.positionKey || humanState.lastPositionKey;
  }

  function mistakeProfile(skill, context, humanState, random) {
    if (skill.id === 'maximum' || context.forcedMove) return { type: 'none', severity: 0 };
    const fatigue = humanState?.fatigue || 0;
    const tilt = humanState?.tilt || 0;
    const timePressure = context.timePressure || 0;
    const confidence = humanState?.confidence || 0;
    const recentMistakes = humanState?.recentMistakes || 0;
    const correction = Math.max(0.46, 1 - recentMistakes * 0.16);
    const confidenceRisk = confidence < 0 ? 1 + Math.abs(confidence) * 0.22 : 1 - confidence * 0.12;
    const risk = Math.min(0.48, skill.blunderRate * (0.65 + context.complexity * 1.8 + fatigue * 1.1 + tilt * 1.25 + timePressure * 1.6) * correction * confidenceRisk);
    if (random() >= risk) return { type: 'none', severity: 0 };
    const roll = random();
    if (skill.rating <= 1000) return { type: roll < 0.45 ? 'tactical-oversight' : roll < 0.75 ? 'material-greed' : 'premature-attack', severity: 0.75 };
    if (skill.rating <= 1600) return { type: roll < 0.45 ? 'shallow-calculation' : roll < 0.75 ? 'misjudged-exchange' : 'plan-error', severity: 0.52 };
    return { type: roll < 0.55 ? 'quiet-resource-miss' : roll < 0.82 ? 'plan-error' : 'risk-overestimate', severity: 0.32 };
  }


  function candidateLossLimit(skill, context, mistake) {
    if (skill.id === 'maximum') return 0;
    const baseBySkill = { beginner: 340, casual: 260, intermediate: 185, club: 125, advanced: 80, expert: 48 };
    let limit = baseBySkill[skill.id] || 125;
    limit *= 0.78 + context.complexity * 0.72;
    limit *= 1 + (context.timePressure || 0) * 0.55;
    if (context.inCheck) limit *= 0.76;
    if (mistake?.type !== 'none') limit *= 1 + (mistake.severity || 0) * 0.85;
    return Math.max(24, Math.round(limit));
  }

  function adjustedCandidateScore(candidate, style, personality, mistake = { type: 'none' }) {
    let score = Number(candidate.score) || 0;
    if (candidate.capture) score += style.captureBonus;
    if (candidate.check) score += style.checkBonus;
    if (candidate.promotion) score += style.promotionBonus;
    if (candidate.capture || candidate.check) score += personality.aggressionBias * 24;
    if (mistake.type === 'material-greed' && candidate.capture) score += 45;
    if (mistake.type === 'premature-attack' && candidate.check) score += 38;
    if (mistake.type === 'misjudged-exchange' && candidate.capture) score += 24;
    if (mistake.type === 'risk-overestimate' && (candidate.capture || candidate.check)) score += 20;
    if (mistake.type === 'quiet-resource-miss' && !candidate.capture && !candidate.check) score -= 18;
    return score;
  }

  function weightedPick(items, random) {
    const total = items.reduce((sum, item) => sum + item.weight, 0);
    if (!(total > 0)) return items[0]?.candidate || null;
    let pick = random() * total;
    for (const item of items) {
      pick -= item.weight;
      if (pick <= 0) return item.candidate;
    }
    return items.at(-1)?.candidate || null;
  }

  function chooseMove(result, state, settings, personality, humanState = null) {
    const skill = SKILLS[settings.skillLevel] || SKILLS.club;
    const style = STYLES[settings.playStyle] || STYLES.balanced;
    const candidates = (Array.isArray(result?.candidates) && result.candidates.length ? result.candidates : [{ move: result.move, score: result.score }])
      .filter((candidate) => candidate?.move)
      .slice(0, skill.candidateCount);
    if (!candidates.length) return { move: null, selected: null, context: contextFromResult(state, result) };
    if (skill.id === 'maximum' || candidates.length === 1) return { move: candidates[0].move, selected: candidates[0], context: contextFromResult(state, result) };

    const context = contextFromResult(state, result);
    const random = personality?.random || Math.random;
    const familiarMove = humanState?.familiarPositions?.get(state?.positionKey || '');
    if (familiarMove) {
      const familiar = candidates.find((candidate) => candidate.move === familiarMove);
      if (familiar) return { move: familiar.move, selected: familiar, context: { ...context, familiar: true }, mistake: { type: 'none', severity: 0 } };
    }
    const mistake = mistakeProfile(skill, context, humanState, random);
    const scored = candidates.map((candidate) => ({ candidate, score: adjustedCandidateScore(candidate, style, personality, mistake) }));
    scored.sort((a, b) => b.score - a.score);
    const bestScore = scored[0].score;
    const lossLimit = candidateLossLimit(skill, context, mistake);
    const plausible = scored.filter((item) => bestScore - item.score <= lossLimit);
    const effectiveTemperature = Math.max(5, skill.temperature * style.volatility * (1 - (personality.accuracyBias || 0)) * (0.75 + context.complexity * 0.65) * (1 + context.timePressure * 0.45));
    const blunderRisk = mistake.type === 'none' ? 0 : Math.min(0.4, skill.blunderRate * (1 + mistake.severity * 3));

    let pool = plausible.length ? plausible : scored.slice(0, 1);
    if (random() < blunderRisk && scored.length > 2) {
      const start = Math.max(1, Math.floor(scored.length * 0.45));
      const riskyPool = scored.slice(start).filter((item) => bestScore - item.score <= lossLimit * 1.35);
      if (riskyPool.length) pool = riskyPool;
    }
    const weighted = pool.map((item) => ({
      candidate: item.candidate,
      weight: Math.exp(-(bestScore - item.score) / effectiveTemperature)
    }));
    const selected = weightedPick(weighted, random) || scored[0].candidate;
    return { move: selected.move, selected, context, blunderRisk, effectiveTemperature, lossLimit, scoreLoss: Math.max(0, bestScore - adjustedCandidateScore(selected, style, personality, mistake)), mistake };
  }

  function searchOptions(settings) {
    const skill = SKILLS[settings.skillLevel] || SKILLS.club;
    if (skill.id === 'maximum') {
      return { maxDepth: settings.maxDepth, moveTimeMs: settings.moveTimeMs, candidateCount: 1 };
    }
    return {
      maxDepth: Math.min(settings.maxDepth, skill.maxDepth),
      moveTimeMs: Math.max(25, Math.round(settings.moveTimeMs * skill.timeScale)),
      candidateCount: skill.candidateCount
    };
  }

  function behaviorModifiers(settings, personality) {
    const skill = SKILLS[settings.skillLevel] || SKILLS.club;
    const behavior = BEHAVIORS[settings.humanBehavior] || BEHAVIORS.natural;
    return {
      speedScale: behavior.speed * personality.speedBias,
      hesitationScale: behavior.hesitation * personality.hesitationBias,
      correctionScale: behavior.correction * (1.08 - skill.cursorDirectness),
      directness: Math.max(0.5, Math.min(0.98, skill.cursorDirectness + personality.cursorDirectnessBias))
    };
  }

  globalThis.VelocitySkillModel = Object.freeze({
    SKILLS, STYLES, BEHAVIORS, createPersonality, createHumanState, updateHumanState,
    chooseMove, searchOptions, behaviorModifiers, contextFromResult, mistakeProfile, candidateLossLimit
  });
})();
