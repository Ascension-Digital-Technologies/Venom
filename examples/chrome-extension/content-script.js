// @venom: browser
(() => {
  'use strict';
  const CONTROLLER_KEY = '__VELOCITY_CHESS_CONTROLLER_V0110__';
  const previousController = globalThis[CONTROLLER_KEY];
  if (previousController && typeof previousController.dispose === 'function') {
    try { previousController.dispose(); } catch (_) {}
  }

  const REQUEST_EVENT = 'VELOCITY_CHESS_EXTENSION_REQUEST_V0110';
  const RESPONSE_EVENT = 'VELOCITY_CHESS_EXTENSION_RESPONSE_V0110';

  function isMissingReceiverError(error) {
    const message = error instanceof Error ? error.message : String(error || '');
    return /Could not establish connection|Receiving end does not exist/i.test(message);
  }

  async function sendBackgroundMessage(message, attempts = 5) {
    let lastError;
    for (let attempt = 0; attempt < attempts; attempt += 1) {
      try {
        return await chrome.runtime.sendMessage(message);
      } catch (error) {
        lastError = error;
        if (!isMissingReceiverError(error) || attempt + 1 >= attempts) throw error;
        await new Promise((resolve) => setTimeout(resolve, 75 * (attempt + 1)));
      }
    }
    throw lastError || new Error('Velocity background service worker is unavailable');
  }
  const humanizer = globalThis.VelocityHumanizer;
  const skillModel = globalThis.VelocitySkillModel;
  const mouseRecorder = globalThis.VelocityMouseRecorder;
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
    telemetry: [],
    lastStateReadAt: 0,
    lastObservedPosition: '',
    lastPositionChangeAt: 0,
    staleProbeCount: 0,
    proactiveRecoveries: 0
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
      varyMoveTiming: input.varyMoveTiming !== false,
      recordedMouseProfile: input.recordedMouseProfile && typeof input.recordedMouseProfile === 'object' ? input.recordedMouseProfile : null
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
    const state = await bridge('GET_STATE', {}, 3500);
    const now = Date.now();
    session.lastStateReadAt = now;
    if (state?.positionKey && state.positionKey !== session.lastObservedPosition) {
      session.lastObservedPosition = state.positionKey;
      session.lastPositionChangeAt = now;
      session.staleProbeCount = 0;
    } else if (state?.positionKey) {
      session.staleProbeCount++;
    }
    return state;
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

  const RECOVERABLE_ERROR = /(timed out|still animating|could not be determined|could not be interpreted|did not change|did not accept|unable to find|unable to locate|board was found|not currently visible|no visible chessboard|incomplete)/i;


  function shouldProactivelyRecover(state, playingSide) {
    if (!state || state.gameOver || !playingSide) return false;
    const health = state.boardHealth || {};
    if (health.rootConnected === false || Number(health.confidence || 0) < 0.55) return true;
    const stagnantMs = Date.now() - (session.lastPositionChangeAt || Date.now());
    const ourTurn = shouldPlay(state.turn, playingSide);
    // Only reset aggressively while it is our turn. During the opponent's turn,
    // a long unchanged position can simply mean they are thinking.
    return ourTurn && session.staleProbeCount >= 36 && stagnantMs >= 9000;
  }

  async function proactiveRecovery(generation, reason) {
    if (!session.running || generation !== session.generation) return false;
    session.proactiveRecoveries++;
    session.totalRecoveries++;
    session.lastRecoveryAt = Date.now();
    session.status = `Refreshing frozen board state · ${reason}`;
    await hardResetVisualState();
    await sleep(220);
    if (!session.running || generation !== session.generation) return false;
    await readState();
    session.lastError = '';
    session.staleProbeCount = 0;
    return true;
  }

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
        if (shouldProactivelyRecover(state, playingSide)) {
          await proactiveRecovery(generation, state.boardHealth?.rootConnected === false ? 'board root replaced' : 'position stopped updating');
          continue;
        }
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
        const analysisResponse = await sendBackgroundMessage({
          type: 'VELOCITY_ANALYZE',
          target: 'velocity-background',
          fen: state.fen,
          options: {
            maxDepth: searchOptions.maxDepth,
            moveTimeMs: searchOptions.moveTimeMs,
            candidateCount: searchOptions.candidateCount,
            hashBits: 18
          }
        });
        if (!analysisResponse?.ok) {
          throw new Error(analysisResponse?.error || 'Protected engine analysis failed');
        }
        const result = analysisResponse.result;
        const decision = skillModel.chooseMove(result, state, session.settings, session.personality, session.humanState);
        if (!decision.move) {
          session.running = false;
          session.status = 'No legal move';
          break;
        }

        const timingResult = { ...result, score: Number(decision.selected?.score ?? result.score) || 0 };
        const scoreShock = Number.isFinite(session.humanState?.lastScore) ? Math.min(1, Math.abs((Number(decision.selected?.score) || 0) - session.humanState.lastScore) / 420) : 0;
        const timingContext = { ...decision.context, familiar: Boolean(decision.context.familiar), surprise: scoreShock };
        const timing = humanizer.chooseMoveDelay(state, timingResult, session.settings, session.timingMemory, session.personality.random, timingContext);
        const modifiers = skillModel.behaviorModifiers(session.settings, session.personality);
        const humanState = session.humanState || {};
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
          scoreLoss: Math.max(0, Number(result.candidates?.[0]?.score ?? result.score) - Number(decision.selected?.score ?? result.score)),
          lossLimit: Number(decision.lossLimit || 0),
          timePressure: Number(decision.context.timePressure || 0),
          fatigue: session.humanState?.fatigue || 0,
          skill: session.settings.skillLevel,
          style: session.settings.playStyle
        };
        session.telemetry.push(telemetryEntry);
        if (session.telemetry.length > 200) session.telemetry.shift();
        skillModel.updateHumanState(session.humanState, {
          positionKey: state.positionKey, move: decision.move,
          score: telemetryEntry.score, complexity: decision.context.complexity,
          forcedMove: decision.context.forcedMove,
          mistakeType: telemetryEntry.mistakeType,
          selectedRank: telemetryEntry.selectedRank,
          thinkMs: telemetryEntry.thinkMs,
          timePressure: telemetryEntry.timePressure
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
    let recordedMouseProfile = null;
    if (mouseRecorder && typeof mouseRecorder.load === 'function') {
      try {
        recordedMouseProfile = await Promise.race([
          mouseRecorder.load(),
          new Promise((resolve) => setTimeout(() => resolve(null), 350))
        ]);
      } catch (_) {
        recordedMouseProfile = null;
      }
    }
    session.settings = normalizeSettings({ ...inputSettings, recordedMouseProfile });
    session.lastError = '';
    session.recoveryCount = 0;
    session.totalRecoveries = 0;
    session.lastRecoveryAt = 0;
    session.lastStateReadAt = 0;
    session.lastObservedPosition = '';
    session.lastPositionChangeAt = Date.now();
    session.staleProbeCount = 0;
    session.proactiveRecoveries = 0;
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
      proactiveRecoveries: session.proactiveRecoveries,
      lastStateReadAt: session.lastStateReadAt,
      lastPositionChangeAt: session.lastPositionChangeAt,
      staleProbeCount: session.staleProbeCount,
      humanState: session.humanState ? {
        moveNumber: session.humanState.moveNumber,
        fatigue: session.humanState.fatigue,
        confidence: session.humanState.confidence,
        tilt: session.humanState.tilt
      } : null,
      telemetryCount: session.telemetry.length,
      recentTelemetry: session.telemetry.slice(-8),
      mouseProfileQuality: humanizer?.assessRecordedProfile ? humanizer.assessRecordedProfile(session.settings?.recordedMouseProfile || null) : null
    };
  }


  async function startMouseRecording() {
    if (!mouseRecorder) return { ok: false, error: 'Mouse recorder was not loaded' };
    if (session.running) stopController();
    return mouseRecorder.start();
  }

  async function stopMouseRecording() {
    if (!mouseRecorder) return { ok: false, error: 'Mouse recorder was not loaded' };
    return mouseRecorder.stop();
  }

  async function clearMouseRecording() {
    if (!mouseRecorder) return { ok: false, error: 'Mouse recorder was not loaded' };
    return mouseRecorder.clear();
  }

  async function mouseRecordingStatus() {
    if (!mouseRecorder) return { ok: false, error: 'Mouse recorder was not loaded' };
    return mouseRecorder.status();
  }


  async function diagnostics() {
    const base = controllerStatus();
    let recorder = { savedSamples: 0, recording: false, quality: { score: 0, label: 'None' }, profile: null };
    if (mouseRecorder?.status) {
      try { recorder = await mouseRecorder.status(); } catch (_) {}
    }
    return {
      ...base,
      recorder,
      generatedAt: Date.now(),
      lastMove: session.telemetry.at(-1) || null
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
    startMouseRecording,
    stopMouseRecording,
    clearMouseRecording,
    mouseRecordingStatus,
    diagnostics,
    dispose: disposeController
  });
  globalThis[CONTROLLER_KEY] = controller;
})();
