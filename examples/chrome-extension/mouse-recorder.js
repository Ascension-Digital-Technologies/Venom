// @venom: browser
(() => {
  'use strict';
  if (globalThis.VelocityMouseRecorder) return;

  const STORAGE_KEY = 'velocityMouseProfileV1';
  const MAX_TEMPLATES = 48;
  const MAX_POINTS = 64;
  const state = { recording: false, strokes: [], active: null, listeners: false };
  const now = () => performance.now();
  const clamp = (v, min, max) => Math.max(min, Math.min(max, v));

  function simplify(points, limit = MAX_POINTS) {
    if (points.length <= limit) return points;
    const result = [];
    for (let i = 0; i < limit; i++) result.push(points[Math.round(i * (points.length - 1) / (limit - 1))]);
    return result;
  }

  function normalizeStroke(stroke) {
    const points = stroke.points;
    if (!points || points.length < 4) return null;
    const first = points[0];
    const last = points[points.length - 1];
    const dx = last.x - first.x;
    const dy = last.y - first.y;
    const distance = Math.hypot(dx, dy);
    const durationMs = Math.max(1, last.time - first.time);
    if (distance < 18 || durationMs < 25) return null;
    const ux = dx / distance;
    const uy = dy / distance;
    const nx = -uy;
    const ny = ux;
    const reduced = simplify(points);
    const template = reduced.map((point) => {
      const rx = point.x - first.x;
      const ry = point.y - first.y;
      return {
        p: clamp((rx * ux + ry * uy) / distance, -0.18, 1.18),
        n: clamp((rx * nx + ry * ny) / distance, -0.6, 0.6),
        t: clamp((point.time - first.time) / durationMs, 0, 1)
      };
    });
    template[0] = { p: 0, n: 0, t: 0 };
    template[template.length - 1] = { p: 1, n: 0, t: 1 };
    let pathLength = 0;
    for (let i = 1; i < points.length; i++) pathLength += Math.hypot(points[i].x - points[i - 1].x, points[i].y - points[i - 1].y);
    return {
      durationMs: Math.round(durationMs),
      distance: Math.round(distance),
      efficiency: clamp(distance / Math.max(distance, pathLength), 0.25, 1),
      template
    };
  }

  function pointerDown(event) {
    if (!state.recording || event.isTrusted === false) return;
    state.active = { pointerId: event.pointerId, points: [{ x: event.clientX, y: event.clientY, time: now() }] };
  }

  function pointerMove(event) {
    if (!state.recording || !state.active || state.active.pointerId !== event.pointerId || event.isTrusted === false) return;
    const last = state.active.points[state.active.points.length - 1];
    const time = now();
    if (time - last.time < 6 && Math.hypot(event.clientX - last.x, event.clientY - last.y) < 2) return;
    state.active.points.push({ x: event.clientX, y: event.clientY, time });
  }

  function pointerEnd(event) {
    if (!state.active || state.active.pointerId !== event.pointerId) return;
    state.active.points.push({ x: event.clientX, y: event.clientY, time: now() });
    const normalized = normalizeStroke(state.active);
    state.active = null;
    if (normalized) {
      state.strokes.push(normalized);
      if (state.strokes.length > MAX_TEMPLATES) state.strokes.shift();
    }
  }

  function attach() {
    if (state.listeners) return;
    state.listeners = true;
    addEventListener('pointerdown', pointerDown, true);
    addEventListener('pointermove', pointerMove, true);
    addEventListener('pointerup', pointerEnd, true);
    addEventListener('pointercancel', pointerEnd, true);
  }

  function detach() {
    if (!state.listeners) return;
    state.listeners = false;
    removeEventListener('pointerdown', pointerDown, true);
    removeEventListener('pointermove', pointerMove, true);
    removeEventListener('pointerup', pointerEnd, true);
    removeEventListener('pointercancel', pointerEnd, true);
  }

  function storageAvailable() {
    return Boolean(globalThis.chrome?.storage?.local);
  }

  function validateProfile(profile) {
    if (!profile || typeof profile !== 'object' || !Array.isArray(profile.templates)) return null;
    const templates = profile.templates.filter((item) =>
      item && typeof item === 'object' && Number.isFinite(Number(item.durationMs)) &&
      Array.isArray(item.points) && item.points.length >= 4 &&
      item.points.every((point) => point && Number.isFinite(Number(point.p)) && Number.isFinite(Number(point.n)) && Number.isFinite(Number(point.t)))
    ).slice(-MAX_TEMPLATES);
    if (!templates.length) return null;
    return {
      version: 1,
      recordedAt: Number(profile.recordedAt) || 0,
      sampleCount: templates.length,
      meanDurationMs: Math.max(1, Number(profile.meanDurationMs) || 420),
      meanDistance: Math.max(1, Number(profile.meanDistance) || 160),
      meanEfficiency: clamp(Number(profile.meanEfficiency) || 0.9, 0.25, 1),
      templates
    };
  }

  function average(values, fallback = 0) {
    return values.length ? values.reduce((sum, value) => sum + value, 0) / values.length : fallback;
  }

  function buildProfile(strokes) {
    const templates = strokes.slice(-MAX_TEMPLATES);
    return {
      version: 1,
      recordedAt: Date.now(),
      sampleCount: templates.length,
      meanDurationMs: Math.round(average(templates.map((item) => item.durationMs), 420)),
      meanDistance: Math.round(average(templates.map((item) => item.distance), 160)),
      meanEfficiency: Number(average(templates.map((item) => item.efficiency), 0.9).toFixed(4)),
      templates: templates.map((item) => ({ durationMs: item.durationMs, distance: item.distance, points: item.template }))
    };
  }


  function profileQuality(profile, targetDistance = null) {
    const valid = validateProfile(profile);
    if (!valid) return { score: 0, label: 'None', sampleScore: 0, coverageScore: 0, consistencyScore: 0, distanceConfidence: 0 };
    const distances = valid.templates.map((item) => Math.max(1, Number(item.distance) || valid.meanDistance));
    const durations = valid.templates.map((item) => Math.max(1, Number(item.durationMs) || valid.meanDurationMs));
    const sampleScore = clamp(valid.sampleCount / 24, 0, 1);
    const minDistance = Math.min(...distances);
    const maxDistance = Math.max(...distances);
    const spread = maxDistance / Math.max(1, minDistance);
    const coverageScore = clamp(Math.log2(Math.max(1, spread)) / 2.6 + Math.min(0.35, valid.sampleCount / 80), 0, 1);
    const meanDuration = average(durations, 1);
    const variance = average(durations.map((value) => ((value - meanDuration) / meanDuration) ** 2), 0);
    const consistencyScore = clamp(1 - variance * 2.4, 0, 1);
    let distanceConfidence = 1;
    if (Number.isFinite(Number(targetDistance))) {
      const target = Math.max(1, Number(targetDistance));
      const nearest = Math.min(...distances.map((value) => Math.abs(Math.log(target / value))));
      distanceConfidence = clamp(1 - nearest / 1.15, 0, 1);
    }
    const score = Math.round(100 * clamp(sampleScore * 0.38 + coverageScore * 0.28 + consistencyScore * 0.18 + distanceConfidence * 0.16, 0, 1));
    const label = score >= 82 ? 'Excellent' : score >= 68 ? 'Good' : score >= 48 ? 'Fair' : score >= 25 ? 'Weak' : 'Insufficient';
    return { score, label, sampleScore, coverageScore, consistencyScore, distanceConfidence, minDistance, maxDistance };
  }

  async function start() {
    attach();
    state.strokes = [];
    state.active = null;
    state.recording = true;
    return { ok: true, recording: true, sampleCount: 0 };
  }

  async function stop() {
    state.recording = false;
    state.active = null;
    detach();
    const profile = buildProfile(state.strokes);
    if (profile.sampleCount >= 3 && storageAvailable()) {
      try { await chrome.storage.local.set({ [STORAGE_KEY]: profile }); } catch (_) {}
    }
    return { ok: profile.sampleCount >= 3, recording: false, sampleCount: profile.sampleCount, profile };
  }

  async function load() {
    if (!storageAvailable()) return null;
    try {
      const result = await chrome.storage.local.get(STORAGE_KEY);
      return validateProfile(result?.[STORAGE_KEY]);
    } catch (_) {
      return null;
    }
  }

  async function clear() {
    state.recording = false;
    state.strokes = [];
    state.active = null;
    detach();
    if (storageAvailable()) {
      try { await chrome.storage.local.remove(STORAGE_KEY); } catch (_) {}
    }
    return { ok: true };
  }

  async function status() {
    const profile = await load();
    return { ok: true, recording: state.recording, currentSamples: state.strokes.length, savedSamples: profile?.sampleCount || 0, profile, quality: profileQuality(profile) };
  }

  globalThis.VelocityMouseRecorder = Object.freeze({ start, stop, load, clear, status, profileQuality });
})();
