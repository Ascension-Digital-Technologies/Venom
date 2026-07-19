(() => {
  'use strict';
  const CONTROLLER_KEY = '__VELOCITY_CHESS_CONTROLLER_V045__';
  const previousController = globalThis[CONTROLLER_KEY];
  if (previousController && typeof previousController.dispose === 'function') {
    try { previousController.dispose(); } catch (_) {}
  }

  const REQUEST_EVENT = 'VELOCITY_CHESS_EXTENSION_REQUEST_V045';
  const RESPONSE_EVENT = 'VELOCITY_CHESS_EXTENSION_RESPONSE_V045';
  const humanizer = globalThis.VelocityHumanizer;
  const skillModel = globalThis.VelocitySkillModel;
  if (!humanizer) throw new Error('Velocity humanization module was not loaded');
  if (!skillModel) throw new Error('Velocity skill model was not loaded');

  const session = {
    running: false,
    generation: 0,
    channel: '',
    settings: null,
    timingMemory: {},
    lastProcessedPosition: '',
    detectedSide: '',
    detectedSideSource: '',
    status: 'Idle',
    lastError: '',
    personality: null,
    recoveryCount: 0,
    totalRecoveries: 0,
    lastRecoveryAt: 0,
    humanState: null,
    lastDecisionAt: 0,
    telemetry: []
  };
  let requestCounter = 0;
  const pending = new Map();
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

  function bridge(type, payload = {}, timeoutMs = 5000) {
    const id = `${Date.now()}-${++requestCounter}`;
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        pending.delete(id);
        reject(new Error(`Page bridge timed out during ${type}`));
      }, timeoutMs);
      pending.set(id, { resolve, reject, timeout });
      window.dispatchEvent(new CustomEvent(REQUEST_EVENT, {
        detail: JSON.stringify({ channel: session.channel, id, type, payload })
      }));
    });
  }

  function handleBridgeResponse(event) {
    let response;
    try { response = JSON.parse(String(event.detail || '')); } catch { return; }
    if (!response || response.channel !== session.channel) return;
    const waiter = pending.get(response.id);
    if (!waiter) return;
    pending.delete(response.id);
    clearTimeout(waiter.timeout);
    if (response.ok) waiter.resolve(response.result);
    else waiter.reject(new Error(response.error || 'Unknown page bridge error'));
  }

  window.addEventListener(RESPONSE_EVENT, handleBridgeResponse);

  function normalizeSettings(input = {}) {
    const minDelayMs = Math.max(100, Math.min(10000, Number(input.minDelayMs) || 450));
    const maxDelayMs = Math.max(minDelayMs, Math.min(15000, Number(input.maxDelayMs) || 1800));
    const dragMinMs = Math.max(120, Math.min(1800, Number(input.dragMinMs) || 260));
    const dragMaxMs = Math.max(dragMinMs, Math.min(2200, Number(input.dragMaxMs) || 620));
    return {
      matchMode: ['1m', '3m', '5m', '10m', 'custom'].includes(input.matchMode) ? input.matchMode : '5m',
      side: ['auto', 'w', 'b', 'both'].includes(input.side) ? input.side : 'auto',
      skillLevel: Object.hasOwn(skillModel.SKILLS, input.skillLevel) ? input.skillLevel : 'club',
      playStyle: Object.hasOwn(skillModel.STYLES, input.playStyle) ? input.playStyle : 'balanced',
      humanBehavior: Object.hasOwn(skillModel.BEHAVIORS, input.humanBehavior) ? input.humanBehavior : 'natural',
      maxDepth: Math.max(1, Math.min(12, Number(input.maxDepth) || 4)),
      moveTimeMs: Math.max(25, Math.min(15000, Number(input.moveTimeMs) || 1000)),
      minDelayMs,
      maxDelayMs,
      dragMinMs,
      dragMaxMs,
      humanize: input.humanize !== false,
      varyMoveTiming: input.varyMoveTiming !== false
    };
  }

  function resolvePlayingSide(state) {
    if (session.settings.side !== 'auto') return session.settings.side;
    if (session.detectedSide === 'w' || session.detectedSide === 'b') return session.detectedSide;
    if (state.playerSide === 'w' || state.playerSide === 'b') {
      session.detectedSide = state.playerSide;
      session.detectedSideSource = state.playerSideSource || 'visual-board';
      return session.detectedSide;
    }
    return null;
  }

  function shouldPlay(turn, side) {
    return side === 'both' || side === turn;
  }

  async function readState() {
    return bridge('GET_STATE', {}, 3500);
  }

  async function waitForPositionChange(originalKey, generation, timeoutMs = 8000) {
    const deadline = Date.now() + timeoutMs;
    while (session.running && generation === session.generation && Date.now() < deadline) {
      await sleep(180);
      const state = await readState();
      if (state.positionKey !== originalKey) return state;
    }
    throw new Error('The visual chessboard did not change after the move');
  }

  const RECOVERABLE_ERROR = /(timed out|still animating|could not be determined|could not be interpreted|did not change|did not accept|unable to find|unable to locate|board was found)/i;

  async function hardResetVisualState() {
    await bridge('RESET_STATE', {}, 5000);
    session.lastProcessedPosition = '';
    session.detectedSide = '';
    session.detectedSideSource = '';
    session.timingMemory = {};
  }

  async function recoverFromTransientError(error, generation) {
    if (!session.running || generation !== session.generation) return false;
    const message = error instanceof Error ? error.message : String(error);
    if (!RECOVERABLE_ERROR.test(message) || session.recoveryCount >= 5) return false;
    session.recoveryCount++;
    session.totalRecoveries++;
    session.lastRecoveryAt = Date.now();
    session.status = `Recovering visual state (${session.recoveryCount}/5)…`;
    try {
      await hardResetVisualState();
      await sleep(180 + session.recoveryCount * 160);
      if (!session.running || generation !== session.generation) return false;
      await readState();
      session.lastError = '';
      return true;
    } catch (recoveryError) {
      session.lastError = recoveryError instanceof Error ? recoveryError.message : String(recoveryError);
      return session.recoveryCount < 5;
    }
  }

  async function run(generation) {
    while (session.running && generation === session.generation) {
      try {
        const state = await readState();
        session.recoveryCount = 0;
        const playingSide = resolvePlayingSide(state);
        if (!playingSide) {
          session.status = 'Detecting your side from the visual board…';
          await sleep(250);
          continue;
        }
        const sideLabel = playingSide === 'both' ? 'both sides' : playingSide === 'w' ? 'White' : 'Black';
        const autoLabel = session.settings.side === 'auto' ? `Auto-detected ${sideLabel}` : `Playing ${sideLabel}`;
        session.status = `${autoLabel} · ${state.turn === 'w' ? 'White' : 'Black'} to move · ${session.settings.matchMode}`;
        if (state.gameOver) {
          session.running = false;
          session.status = 'Game over';
          break;
        }
        if (!shouldPlay(state.turn, playingSide) || state.positionKey === session.lastProcessedPosition) {
          await sleep(250);
          continue;
        }

        session.status = 'Velocity is thinking…';
        const searchOptions = skillModel.searchOptions(session.settings);
        const response = await chrome.runtime.sendMessage({
          target: 'velocity-background',
          type: 'ANALYZE',
          payload: {
            fen: state.fen,
            options: {
              maxDepth: searchOptions.maxDepth,
              moveTimeMs: searchOptions.moveTimeMs,
              candidateCount: searchOptions.candidateCount,
              hashBits: 18
            }
          }
        });
        if (!response || response.ok !== true) {
          throw new Error(response?.error || 'Protected Velocity engine did not return a result');
        }
        const result = response.result;
        const decision = skillModel.chooseMove(result, state, session.settings, session.personality, session.humanState);
        if (!decision.move) {
          session.running = false;
          session.status = 'No legal move';
          break;
        }

        const timingResult = { ...result, score: Number(decision.selected?.score ?? result.score) || 0 };
        const timing = humanizer.chooseMoveDelay(state, timingResult, session.settings, session.timingMemory, session.personality.random);
        const modifiers = skillModel.behaviorModifiers(session.settings, session.personality);
        const humanState = session.humanState || {};
        const scoreShock = Number.isFinite(humanState.lastScore) ? Math.min(1, Math.abs((Number(decision.selected?.score) || 0) - humanState.lastScore) / 420) : 0;
        const fatigueScale = 1 + (humanState.fatigue || 0) * 0.16;
        const surpriseScale = 1 + scoreShock * 0.42;
        const confidenceScale = 1 - Math.max(-0.12, Math.min(0.1, (humanState.confidence || 0) * 0.08));
        const familiarityScale = decision.context.familiar ? 0.38 : 1;
        timing.waitMs = Math.max(0, Math.round(timing.waitMs * modifiers.speedScale * (0.82 + decision.context.complexity * 0.42) * fatigueScale * surpriseScale * confidenceScale * familiarityScale));
        const behavior = humanizer.createMoveBehavior(session.settings, session.timingMemory, session.personality.random);
        behavior.durationMs = Math.round(behavior.durationMs * modifiers.speedScale);
        behavior.approachMs = Math.round(behavior.approachMs * modifiers.speedScale);
        behavior.hoverMs = Math.round(behavior.hoverMs * modifiers.hesitationScale);
        behavior.holdMs = Math.round(behavior.holdMs * modifiers.hesitationScale);
        behavior.destinationPauseMs = Math.round(behavior.destinationPauseMs * modifiers.hesitationScale);
        behavior.microCorrections = Math.max(0, Math.round(behavior.microCorrections * (1 + modifiers.correctionScale * 2)));
        behavior.pathJitterPx *= Math.max(0.45, 1.15 - modifiers.directness);
        behavior.bendRatio *= Math.max(0.45, 1.2 - modifiers.directness * 0.55);
        if ((session.humanState?.fatigue || 0) > 0.45) {
          behavior.pathJitterPx *= 1 + session.humanState.fatigue * 0.32;
          if (session.personality.random() < session.humanState.fatigue * 0.12) behavior.microCorrections += 1;
        }
        if (decision.context.complexity > 0.72 && !decision.context.forcedMove && session.personality.random() < 0.055 * modifiers.hesitationScale) {
          behavior.hoverMs += Math.round(180 + session.personality.random() * 520);
          behavior.destinationPauseMs += Math.round(70 + session.personality.random() * 180);
        }
        session.status = `Playing ${decision.move} after ${timing.waitMs} ms · ${session.settings.skillLevel}`;
        await sleep(timing.waitMs);
        if (!session.running || generation !== session.generation) break;

        const latest = await readState();
        if (latest.positionKey !== state.positionKey || latest.turn !== state.turn) continue;

        session.lastProcessedPosition = state.positionKey;
        await bridge('PLAY_MOVE', { move: decision.move, behavior }, 12000);
        await waitForPositionChange(state.positionKey, generation);
        session.lastError = '';
        session.recoveryCount = 0;
        const telemetryEntry = {
          at: Date.now(), positionKey: state.positionKey, move: decision.move,
          bestMove: result.candidates?.[0]?.move || result.move,
          selectedRank: Math.max(1, (result.candidates || []).findIndex((candidate) => candidate.move === decision.move) + 1),
          score: Number(decision.selected?.score ?? result.score) || 0,
          bestScore: Number(result.candidates?.[0]?.score ?? result.score) || 0,
          thinkMs: timing.waitMs + Number(result.elapsedMs || 0),
          complexity: decision.context.complexity,
          mistakeType: decision.mistake?.type || 'none',
          fatigue: session.humanState?.fatigue || 0,
          skill: session.settings.skillLevel,
          style: session.settings.playStyle
        };
        session.telemetry.push(telemetryEntry);
        if (session.telemetry.length > 200) session.telemetry.shift();
        skillModel.updateHumanState(session.humanState, {
          positionKey: state.positionKey, move: decision.move,
          score: telemetryEntry.score, complexity: decision.context.complexity
        });
        const mistakeLabel = telemetryEntry.mistakeType === 'none' ? '' : ` · ${telemetryEntry.mistakeType}`;
        session.status = `Played ${decision.move} · ${session.settings.skillLevel}${mistakeLabel} · depth ${result.depth} · ${Number(result.nodes || 0).toLocaleString()} nodes`;
        await sleep(session.settings.humanize ? behavior.releaseMs : 120);
      } catch (error) {
        session.lastError = error instanceof Error ? error.message : String(error);
        if (await recoverFromTransientError(error, generation)) continue;
        session.status = `Paused by error: ${session.lastError}`;
        session.running = false;
      }
    }
  }

  async function startController(channel, inputSettings = {}) {
    session.running = false;
    session.generation++;
    session.channel = String(channel || '');
    session.settings = normalizeSettings(inputSettings);
    session.lastError = '';
    session.recoveryCount = 0;
    session.totalRecoveries = 0;
    session.lastRecoveryAt = 0;
    session.personality = skillModel.createPersonality(`${session.channel}:${Date.now()}`);
    session.humanState = skillModel.createHumanState();
    session.telemetry = [];
    session.status = 'Rebuilding state from the live board…';
    try {
      await hardResetVisualState();
      let bootstrapped = false;
      let bootstrapError = null;
      for (let attempt = 1; attempt <= 6; attempt++) {
        try {
          await readState();
          bootstrapped = true;
          break;
        } catch (error) {
          bootstrapError = error;
          session.status = `Rebuilding live position (${attempt}/6)…`;
          await sleep(120 + attempt * 120);
          await hardResetVisualState();
        }
      }
      if (!bootstrapped) throw bootstrapError || new Error('Unable to read a stable live board');
    } catch (error) {
      session.lastError = error instanceof Error ? error.message : String(error);
      session.status = `Unable to rebuild visual state: ${session.lastError}`;
      return { ok: false, error: session.lastError };
    }
    session.running = true;
    session.status = 'Starting…';
    const generation = session.generation;
    void run(generation);
    return { ok: true, recovered: true };
  }

  function stopController() {
    session.running = false;
    session.generation++;
    session.status = 'Stopped';
    return { ok: true };
  }

  function controllerStatus() {
    return {
      ok: true,
      running: session.running,
      status: session.status,
      error: session.lastError,
      settings: session.settings,
      detectedSide: session.detectedSide,
      detectedSideSource: session.detectedSideSource,
      skillLevel: session.settings?.skillLevel || '',
      playStyle: session.settings?.playStyle || '',
      recoveryCount: session.recoveryCount,
      totalRecoveries: session.totalRecoveries,
      lastRecoveryAt: session.lastRecoveryAt,
      humanState: session.humanState ? {
        moveNumber: session.humanState.moveNumber,
        fatigue: session.humanState.fatigue,
        confidence: session.humanState.confidence,
        tilt: session.humanState.tilt
      } : null,
      telemetryCount: session.telemetry.length,
      recentTelemetry: session.telemetry.slice(-8)
    };
  }

  function disposeController() {
    stopController();
    window.removeEventListener(RESPONSE_EVENT, handleBridgeResponse);
    for (const waiter of pending.values()) {
      clearTimeout(waiter.timeout);
      try { waiter.reject(new Error('Velocity page controller was replaced')); } catch (_) {}
    }
    pending.clear();
    if (globalThis[CONTROLLER_KEY] === controller) delete globalThis[CONTROLLER_KEY];
  }

  const controller = Object.freeze({
    start: startController,
    stop: stopController,
    status: controllerStatus,
    dispose: disposeController
  });
  globalThis[CONTROLLER_KEY] = controller;
})();
